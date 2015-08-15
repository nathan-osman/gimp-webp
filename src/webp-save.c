/**
 * gimp-webp - WebP Plugin for the GIMP
 * Copyright (C) 2015  Nathan Osman
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

#include "webp-save.h"

WebPPreset webp_preset_by_name(gchar *name)
{
    if(!strcmp(name, "picture")) {
        return WEBP_PRESET_PICTURE;
    } else if(!strcmp(name, "photo")) {
        return WEBP_PRESET_PHOTO;
    } else if(!strcmp(name, "drawing")) {
        return WEBP_PRESET_DRAWING;
    } else if(!strcmp(name, "icon")) {
        return WEBP_PRESET_ICON;
    } else if(!strcmp(name, "text")) {
        return WEBP_PRESET_TEXT;
    } else {
        return WEBP_PRESET_DEFAULT;
    }
}

int webp_file_writer(const uint8_t     *data,
                     size_t             data_size,
                     const WebPPicture *picture)
{
    FILE *outfile;

    /* Obtain the FILE* and write the data to the file */
    outfile = (FILE*)picture->custom_ptr;
    return fwrite(data, sizeof(uint8_t), data_size, outfile) == data_size;
}

int webp_file_progress(int                percent,
                       const WebPPicture *picture)
{
    return gimp_progress_update(percent / 100.0);
}

const gchar *webp_error_string(WebPEncodingError error_code)
{
    switch(error_code) {
    case VP8_ENC_ERROR_OUT_OF_MEMORY:           return "out of memory";
    case VP8_ENC_ERROR_BITSTREAM_OUT_OF_MEMORY: return "not enough memory to flush bits";
    case VP8_ENC_ERROR_NULL_PARAMETER:          return "NULL parameter";
    case VP8_ENC_ERROR_INVALID_CONFIGURATION:   return "invalid configuration";
    case VP8_ENC_ERROR_BAD_DIMENSION:           return "bad image dimensions";
    case VP8_ENC_ERROR_PARTITION0_OVERFLOW:     return "partition is bigger than 512K";
    case VP8_ENC_ERROR_PARTITION_OVERFLOW:      return "partition is bigger than 16M";
    case VP8_ENC_ERROR_BAD_WRITE:               return "unable to flush bytes";
    case VP8_ENC_ERROR_FILE_TOO_BIG:            return "file is larger than 4GiB";
    case VP8_ENC_ERROR_USER_ABORT:              return "user aborted encoding";
    case VP8_ENC_ERROR_LAST:                    return "list terminator";
    default:                                    return "unknown error";
    }
}

gboolean save_image(const gchar *filename,
                    gint32       drawable_ID,
                    gchar       *preset,
                    gboolean     lossless,
                    gfloat       quality,
                    gfloat       alpha_quality,
                    GError     **error)
{
    gboolean      status   = FALSE;
    GimpDrawable *drawable = NULL;
    GimpImageType drawable_type;
    FILE         *outfile  = NULL;
    WebPConfig    config   = {0};
    WebPPicture   picture  = {0};
    GimpPixelRgn  region;
    guchar       *buffer   = NULL;

    /* The do...while() loop is a neat little trick that makes it easier to
     * jump to error handling code while still ensuring proper cleanup */

    do {

        /* Obtain the drawable and determine its type */
        drawable = gimp_drawable_get(drawable_ID);
        drawable_type = gimp_drawable_type(drawable_ID);

        /* Begin displaying export progress */
        gimp_progress_init_printf("Saving '%s'",
                                  gimp_filename_to_utf8(filename));

        /* Attempt to open the output file */
        if((outfile = g_fopen(filename, "wb")) == NULL) {
            g_set_error(error,
                        G_FILE_ERROR,
                        g_file_error_from_errno(errno),
                        "Unable to open '%s' for writing",
                        gimp_filename_to_utf8(filename));
            break;
        }

        /* Initialize the WebP configuration with a preset and fill in the
         * remaining values */
        WebPConfigPreset(&config,
                         webp_preset_by_name(preset),
                         quality);

        config.lossless      = lossless;
        config.method        = 6;  /* better quality */
        config.alpha_quality = alpha_quality;

        /* Prepare the WebP structure */
        WebPPictureInit(&picture);
        picture.use_argb      = 1;
        picture.width         = drawable->width;
        picture.height        = drawable->height;
        picture.writer        = webp_file_writer;
        picture.custom_ptr    = outfile;
        picture.progress_hook = webp_file_progress;

        /* Obtain the pixel region for the drawable */
        gimp_pixel_rgn_init(&region,
                            drawable,
                            0, 0,
                            drawable->width,
                            drawable->height,
                            FALSE, FALSE);

        /* Attempt to allocate a buffer of the appropriate size */
        buffer = (guchar *)malloc(drawable->width * drawable->height * drawable->bpp);
        if(!buffer) {
            break;
        }

        /* Read the region into the buffer */
        gimp_pixel_rgn_get_rect(&region,
                                buffer,
                                0, 0,
                                drawable->width,
                                drawable->height);

        /* Use the appropriate function to import the data from the buffer */
        if(drawable_type == GIMP_RGB_IMAGE) {
            WebPPictureImportRGB(&picture, buffer, drawable->width * drawable->bpp);
        } else {
            WebPPictureImportRGBA(&picture, buffer, drawable->width * drawable->bpp);
        }

        /* Perform the actual encode */
        if(!WebPEncode(&config, &picture)) {
            g_set_error(error,
                        G_FILE_ERROR,
                        picture.error_code,
                        "WebP error: '%s'",
                        webp_error_string(picture.error_code));
            break;
        }

        /* The cleanup stuff still needs to run but indicate that everything
         * completed successfully */
        status = TRUE;

    } while(0);

    /* Free any resources */

    if(drawable) {
        gimp_drawable_detach(drawable);
    }

    if(outfile) {
        fclose(outfile);
    }

    if(buffer) {
        free(buffer);
    }

    WebPPictureFree(&picture);

    /* Return the status */
    return status;
}
