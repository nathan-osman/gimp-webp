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

#include <glib.h>
#include <glib/gstdio.h>
#include <libgimp/gimp.h>
#include <stdio.h>
#include <webp/decode.h>
#include <webp/mux.h>

#include "config.h"
#include "webp-load.h"

#ifdef GIMP_2_9
#  include <gegl.h>
#endif

/* Create a layer with the provided image data and add it to the image */
gboolean create_layer(gint32   image_ID,
                      uint8_t *layer_data,
                      gint32   position,
                      gchar   *name,
                      gint     width,
                      gint     height,
                      gint32   offsetx,
                      gint32   offsety)
{
    gint32        layer_ID;
#ifdef GIMP_2_9
    GeglBuffer   *geglbuffer;
    GeglRectangle extent;
#else
    GimpDrawable *drawable;
    GimpPixelRgn  region;
#endif

    layer_ID = gimp_layer_new(image_ID,
                              name,
                              width, height,
	                          GIMP_RGBA_IMAGE,
	                          100,
                              GIMP_NORMAL_MODE);

#ifdef GIMP_2_9
	/* Retrieve the buffer for the layer */
	geglbuffer = gimp_drawable_get_buffer(layer_ID);

	/* Copy the image data to the region */
    gegl_rectangle_set(&extent, 0, 0, width, height);
    gegl_buffer_set(geglbuffer, &extent, 0, NULL, layer_data, GEGL_AUTO_ROWSTRIDE);

	/* Flush the drawable and detach */
    gegl_buffer_flush(geglbuffer);

	g_object_unref(geglbuffer);
#else
    /* Retrieve the drawable for the layer */
    drawable = gimp_drawable_get(layer_ID);

    /* Get a pixel region from the layer */
    gimp_pixel_rgn_init(&region,
                        drawable,
                        0, 0,
                        width, height,
                        FALSE, FALSE);

    /* Copy the image data to the region */
    gimp_pixel_rgn_set_rect(&region,
                            layer_data,
                            0, 0,
                            width, height);

    /* Flush the drawable and detach */
    gimp_drawable_flush(drawable);
    gimp_drawable_detach(drawable);
#endif

    /* Add the new layer to the image */
    gimp_image_insert_layer(image_ID, layer_ID, -1, position);

    /* If layer offsets were provided, use them to position the image */
    if (offsetx || offsety) {
        gimp_layer_set_offsets(layer_ID, offsetx, offsety);
    }

    /* TODO: fix this */
    return TRUE;
}

gboolean load_image(const gchar *filename,
                    gint32      *image_ID,
                    GError     **error)
{
    gboolean              status      = FALSE;
    gchar                *indata      = NULL;
    gsize                 indatalen;
    gint                  width;
    gint                  height;
    WebPMux              *mux         = NULL;
    WebPData              wp_data;
    uint32_t              flags;
    uint8_t              *outdata     = NULL;

#ifdef GIMP_2_9
    /* Initialize GEGL */
    gegl_init(NULL, NULL);
#endif

    do {

        /* Attempt to read the file contents from disk */
        if (g_file_get_contents(filename,
                                &indata,
                                &indatalen,
                                error) == FALSE) {
            break;
        }

        /* Validate WebP data, grabbing the width and height */
        if (!WebPGetInfo(indata, indatalen, &width, &height)) {
            break;
        }

        /* Create a WebPMux from the contents of the file */
        wp_data.bytes = (uint8_t*)indata;
        wp_data.size = indatalen;

        mux = WebPMuxCreate(&wp_data, 1);
        if (mux == NULL) {
            break;
        }

        /* Retrieve the features present */
        if (WebPMuxGetFeatures(mux, &flags) != WEBP_MUX_OK) {
            break;
        }

        /* TODO: decode the image in "chunks" or "tiles" */
        /* TODO: check if an alpha channel is present */

        /* Create the new image and associated layer */
        *image_ID = gimp_image_new(width, height, GIMP_RGB);

#ifdef WEBP_0_5
        if (flags & ANIMATION_FLAG) {
            int frames;

            /* Retrieve the number of frames */
            WebPMuxNumChunks(mux, WEBP_CHUNK_ANMF, &frames);

            /* Loop over each of the frames */
            for (int i = 0; i < frames; ++i) {
                WebPMuxFrameInfo frame = {0};

                /* Retrieve the data for the frame */
                if (WebPMuxGetFrame(mux, i, &frame) != WEBP_MUX_OK) {
                    goto error;
                }

                /* Decode the frame */
                outdata = WebPDecodeRGBA(frame.bitstream.bytes,
                                         frame.bitstream.size,
                                         &width, &height);

                /* Free the compressed data */
                WebPDataClear(&frame.bitstream);

                if (!outdata) {
                    goto error;
                }

                /* Create a layer for the frame */
                char name[255];
                snprintf(name, 255, "Frame %d", (i + 1));

                if (create_layer(*image_ID,
                                 outdata,
                                 0,
                                 (gchar*)name,
                                 width, height,
                                 frame.x_offset,
                                 frame.y_offset) == FALSE) {
                    goto error;
                }
            }

            /* If all is well, jump *over* the error label - otherwise
               leave the loop and begin cleaning things up */

            goto success;

        error:
            break;

        success: ;

        } else {
#endif

            /* Attempt to decode the data as a WebP image */
            outdata = WebPDecodeRGBA(indata, indatalen, &width, &height);
            if (!outdata) {
                break;
            }

            /* Create a single layer */
            status = create_layer(*image_ID,
                                  outdata,
                                  0,
                                  "Background",
                                  width, height,
                                  0, 0);

#ifdef WEBP_0_5
        }

#ifdef GIMP_2_9
        /* Load a color profile if one was provided */
        if (flags & ICCP_FLAG) {
            WebPData          icc_profile;
            GimpColorProfile *profile;

            /* Load the ICC profile from the file */
            WebPMuxGetChunk(mux, "ICCP", &icc_profile);

            /* Have Gimp load the color profile */
            profile = gimp_color_profile_new_from_icc_profile(
                        icc_profile.bytes, icc_profile.size, NULL);
            if (profile) {
                gimp_image_set_color_profile(image_ID, profile);
                g_object_unref(profile);
            }
        }
#endif
#endif
        /* Set the filename for the image */
        gimp_image_set_filename(*image_ID, filename);

    } while(0);

    /* Delete the mux object */
    if (mux) {
        WebPMuxDelete(mux);
    }

    /* Free the data read from disk */
    if (indata) {
        g_free(indata);
    }

    return status;
}
