/**
 * gimp-webp - WebP Plugin for the GIMP
 * Copyright (C) 2016  Nathan Osman & Ben Touchette
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <glib/gstdio.h>
#include <libgimp/gimp.h>
#include <stdio.h>
#include <stdlib.h>
#include <webp/encode.h>
#include <webp/mux.h>

#include "webp-save.h"

#ifdef GIMP_2_9
#  include <gegl.h>
#endif

/* Determine which WebP preset to use given its name */
WebPPreset webp_preset_by_name(gchar *name)
{
    if (!strcmp(name, "picture")) {
        return WEBP_PRESET_PICTURE;
    } else if (!strcmp(name, "photo")) {
        return WEBP_PRESET_PHOTO;
    } else if (!strcmp(name, "drawing")) {
        return WEBP_PRESET_DRAWING;
    } else if (!strcmp(name, "icon")) {
        return WEBP_PRESET_ICON;
    } else if (!strcmp(name, "text")) {
        return WEBP_PRESET_TEXT;
    } else {
        return WEBP_PRESET_DEFAULT;
    }
}

/* Write the provided data to the file */
int webp_file_writer(const uint8_t     *data,
                     size_t             data_size,
                     const WebPPicture *picture)
{
    FILE *outfile;

    /* Obtain the FILE* and write the data to the file */
    outfile = (FILE*)picture->custom_ptr;
    return fwrite(data, sizeof(uint8_t), data_size, outfile) == data_size;
}

/* Update progress as data is written to the file */
int webp_file_progress(int                percent,
                       const WebPPicture *picture)
{
    return gimp_progress_update(percent / 100.0);
}

/* Convert error into a human-readable message */
const gchar *webp_error_string(WebPEncodingError error_code)
{
    switch(error_code) {
    case VP8_ENC_ERROR_OUT_OF_MEMORY:
        return "out of memory";
    case VP8_ENC_ERROR_BITSTREAM_OUT_OF_MEMORY:
        return "not enough memory to flush bits";
    case VP8_ENC_ERROR_NULL_PARAMETER:
        return "NULL parameter";
    case VP8_ENC_ERROR_INVALID_CONFIGURATION:
        return "invalid configuration";
    case VP8_ENC_ERROR_BAD_DIMENSION:
        return "bad image dimensions";
    case VP8_ENC_ERROR_PARTITION0_OVERFLOW:
        return "partition is bigger than 512K";
    case VP8_ENC_ERROR_PARTITION_OVERFLOW:
        return "partition is bigger than 16M";
    case VP8_ENC_ERROR_BAD_WRITE:
        return "unable to flush bytes";
    case VP8_ENC_ERROR_FILE_TOO_BIG:
        return "file is larger than 4GiB";
    case VP8_ENC_ERROR_USER_ABORT:
        return "user aborted encoding";
    case VP8_ENC_ERROR_LAST:
        return "list terminator";
    default:
        return "unknown error";
    }
}

/* Save a layer from an image */
gboolean save_layer(gint32             drawable_ID,
                    WebPWriterFunction writer,
                    void              *custom_ptr,
#ifdef WEBP_0_5
                    gboolean           animation,
                    WebPAnimEncoder   *enc,
                    int                frame_timestamp,
#endif
                    WebPSaveParams    *params,
                    GError           **error)
{
    gboolean          status   = FALSE;
    gint              bpp;
    gint              width;
    gint              height;
    GimpImageType     drawable_type;
    WebPConfig        config;
    WebPPicture       picture;
    guchar           *buffer   = NULL;
#ifdef GIMP_2_9
    GeglBuffer       *geglbuffer;
    GeglRectangle     extent;
#else
    GimpDrawable     *drawable = NULL;
    GimpPixelRgn      region;
#endif

    /* Retrieve the image data */
    bpp = gimp_drawable_bpp(drawable_ID);
    width = gimp_drawable_width(drawable_ID);
    height = gimp_drawable_height(drawable_ID);
    drawable_type = gimp_drawable_type(drawable_ID);

    /* Initialize the WebP configuration with a preset and fill in the
     * remaining values */
    WebPConfigPreset(&config,
                     webp_preset_by_name(params->preset),
                     params->quality);

    config.lossless      = params->lossless;
    config.method        = 6;  /* better quality */
    config.alpha_quality = params->alpha_quality;

    /* Prepare the WebP structure */
    WebPPictureInit(&picture);
    picture.use_argb      = 1;
    picture.width         = width;
    picture.height        = height;
    picture.writer        = writer;
    picture.custom_ptr    = custom_ptr;
    picture.progress_hook = webp_file_progress;

    do {
        /* Attempt to allocate a buffer of the appropriate size */
        buffer = (guchar *)g_malloc(bpp * width * height);
        if(!buffer) {
            g_set_error(error,
                        G_FILE_ERROR,
                        0,
                        "Unable to allocate buffer for layer");
            break;
        }

#ifdef GIMP_2_9
        /* Obtain the buffer and get its extent */
        geglbuffer = gimp_drawable_get_buffer(drawable_ID);
        extent = *gegl_buffer_get_extent(geglbuffer);

        /* Read the layer buffer into our buffer */
        gegl_buffer_get(geglbuffer, &extent, 1.0, NULL, buffer,
                        GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

        g_object_unref(geglbuffer);
#else
        /* Get the drawable */
        drawable = gimp_drawable_get(drawable_ID);

        /* Obtain the pixel region for the drawable */
        gimp_pixel_rgn_init(&region,
                            drawable,
                            0, 0,
                            width,
                            height,
                            FALSE, FALSE);

        /* Read the region into the buffer */
        gimp_pixel_rgn_get_rect(&region,
                                buffer,
                                0, 0,
                                width,
                                height);

        gimp_drawable_detach(drawable);
#endif

        /* Use the appropriate function to import the data from the buffer */
        if(drawable_type == GIMP_RGB_IMAGE) {
            WebPPictureImportRGB(&picture, buffer, width * bpp);
        } else {
            WebPPictureImportRGBA(&picture, buffer, width * bpp);
        }

#ifdef WEBP_0_5
        if (animation == TRUE) {

            if (!WebPAnimEncoderAdd(enc, &picture, frame_timestamp, &config)) {
                g_set_error(error,
                            G_FILE_ERROR,
                            picture.error_code,
                            "WebP error: '%s'",
                            webp_error_string(picture.error_code));
                break;
            }
        } else {
#endif
            if(!WebPEncode(&config, &picture)) {
                g_set_error(error,
                            G_FILE_ERROR,
                            picture.error_code,
                            "WebP error: '%s'",
                            webp_error_string(picture.error_code));
                break;
            }
#ifdef WEBP_0_5
        }
#endif

        /* Everything succeeded */
        status = TRUE;

    } while(0);

    /* Free the buffer */
    if (buffer) {
        free(buffer);
    }

    return status;
}

#ifdef WEBP_0_5
/* Save an animation to disk */
gboolean save_animation(gint32          nLayers,
                        gint32         *allLayers,
                        WebPSaveParams *params,
                        GError        **error)
{
    gboolean               status          = FALSE;
    gboolean               innerStatus     = FALSE;
    WebPAnimEncoderOptions enc_options;
    WebPAnimEncoder       *enc             = NULL;
    int                    frame_timestamp = 0;
    WebPData               webp_data       = {0};
    WebPMux               *mux;
    WebPMuxAnimParams      anim_params     = {0};

    /* Prepare for encoding an animation */
    WebPAnimEncoderOptionsInit(&enc_options);

    do {
        /* Create the encoder */
        enc = WebPAnimEncoderNew(gimp_drawable_width(drawable_ID),
                                 gimp_drawable_height(drawable_ID),
                                 &enc_options);

        /* Encode each layer */
        for (int i = 0; i < nLayers; i++) {
            if ((innerStatus = save_layer(allLayers[i],
                                          params,
                                          NULL,
                                          NULL,
                                          TRUE,
                                          enc,
                                          frame_timestamp,
                                          error)) == FALSE) {
                break;
            }
        }

        /* Check to make sure each layer was encoded correctly */
        if (innerStatus == FALSE) {
            break;
        }

        /* Add NULL frame */
        WebPAnimEncoderAdd(enc, NULL, frame_timestamp, NULL);

        /* Initialize the WebP image structure */
        WebPDataInit(&webp_data);

        /* Write the animation to the image */
        if (!WebPAnimEncoderAssemble(enc, &webp_data)) {
            g_set_error(error,
                        G_FILE_ERROR,
                        0,
                        "Encoding error: '%s'",
                        WebPAnimEncoderGetError(enc));
            break;
        }

        /* Create a Mux */
        mux = WebPMuxCreate(&webp_data, 1);

        /* Set animation parameters */
        anim_params.loop_count = params->loop == TRUE ? 0 : 1;
        WebPMuxSetAnimationParams(mux, &anim_params);

        /* Assemble the image */
        WebPMuxAssemble(mux, &webp_data);

        /* Write to disk */
        if (fwrite(webp_data.bytes, webp_data.size, 1, outfile) != 1) {
            break;
        }

        /* Everything succeeded */
        status = TRUE;

    } while(0);

    /* Free image data */
    WebPDataClear(&webp_data);

    /* Free the animation encoder */
    if (enc) {
        WebPAnimEncoderDelete(enc);
    }

    return status;
}
#endif

/* Save a WebP image to disk */
gboolean save_image(const gchar    *filename,
#ifdef WEBP_0_5
                    gint32          nLayers,
                    gint32         *allLayers,
#endif
                    gint32          drawable_ID,
                    WebPSaveParams *params,
                    GError        **error)
{
    gboolean status  = FALSE;
    FILE    *outfile = NULL;

#ifdef GIMP_2_9
    /* Initialize GEGL */
    gegl_init(NULL, NULL);
#endif

    /* Begin displaying export progress */
    gimp_progress_init_printf("Saving '%s'",
                              gimp_filename_to_utf8(filename));

    /* Attempt to open the output file */
    if((outfile = g_fopen(filename, "wb+")) == NULL) {
        g_set_error(error,
                    G_FILE_ERROR,
                    g_file_error_from_errno(errno),
                    "Unable to open '%s' for writing",
                    gimp_filename_to_utf8(filename));
        return FALSE;
    }

#ifdef WEBP_0_5
    if (params->animation == TRUE) {
        status = save_animation(nLayers
                                allLayers,
                                outfile,
                                params,
                                error);
    } else {
#endif
        status = save_layer(drawable_ID,
                            webp_file_writer,
                            outfile,
#ifdef WEBP_0_5
                            FALSE,
                            NULL,
                            0,
#endif
                            params,
                            error);
#ifdef WEBP_0_5
    }
#endif

    /* Close the file */
    if(outfile) {
        fclose(outfile);
    }

    return status;
}
