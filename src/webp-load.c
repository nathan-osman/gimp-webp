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

#include <glib.h>
#include <glib/gstdio.h>
#include <libgimp/gimp.h>
#include <stdio.h>
#include <webp/decode.h>
#include <webp/mux.h>

#include "webp-load.h"

gint32 load_layer(gint32            image_id,
                  WebPMuxFrameInfo *info,
                  int               layer)
{
    uint8_t      *outdata;
    int           width;
    int           height;
    char          layer_name[40];
    int           layer_id;
    GimpDrawable *drawable;
    GimpPixelRgn  region;

    /* Decode the image */
    outdata = WebPDecodeRGBA(info->bitstream.bytes,
                             info->bitstream.size,
                             &width,
                             &height);
    if (!outdata) {
        return -1;
    }

    /* Create the layer */
    snprintf(layer_name, 40, "Layer %d", layer);
    layer_id = gimp_layer_new(image_id,
                              layer_name,
                              width,
                              height,
                              GIMP_RGBA_IMAGE,
                              100,
                              GIMP_NORMAL_MODE);

    /* Retrieve the drawable for the layer */
    drawable = gimp_drawable_get(layer_id);

    /* Get a pixel region from the layer */
    gimp_pixel_rgn_init(&region,
                        drawable,
                        0, 0,
                        width, height,
                        FALSE, FALSE);

    /* Copy the image data to the region */
    gimp_pixel_rgn_set_rect(&region,
                            outdata,
                            0, 0,
                            width, height);

    /* Flush the drawable and detach */
    gimp_drawable_flush(drawable);
    gimp_drawable_detach(drawable);

    /* Free the image data */
    free(outdata);

    return layer_id;
}

gint32 load_image(const gchar *filename,
                  GError     **error)
{
    gint32       ret       = -1;
    gchar       *indata    = 0;
    gsize        indatalen;
    WebPData     bitstream;
    WebPMux     *mux       = 0;
    WebPMuxError err;
    int          width;
    int          height;
    uint32_t     flag;
    int          nFrames;
    gint32       image_id;
    int          i;

    /* Attempt to read the file contents from disk */
    if (g_file_get_contents(filename,
                            &indata,
                            &indatalen,
                            error) == FALSE) {
        goto error;
    }
    bitstream.bytes = (const uint8_t *)indata;
    bitstream.size = indatalen;

    /* Create the mux object from the bitstream  */
    mux = WebPMuxCreate(&bitstream, 0);
    if (!mux) {
        goto error;
    }

    /* Retrieve image dimensions */
    err = WebPMuxGetCanvasSize(mux, &width, &height);
    if (err != WEBP_MUX_OK) {
        goto error;
    }

    /* Find out if the image includes transparency (RGB vs. RGBA) */
    err = WebPMuxGetFeatures(mux, &flag);
    if (err != WEBP_MUX_OK) {
        goto error;
    }

    /* Determine how many frames are in the image - if this image doesn't
       contain an animation, this value is set to 0 - however, the first
       "frame" can still be retrieved, so set the number to 1 */
    err = WebPMuxNumChunks(mux, WEBP_CHUNK_ANMF, &nFrames);
    if (err != WEBP_MUX_OK) {
        goto error;
    }
    nFrames = nFrames ? nFrames : 1;

    /* Create a new image with the dimensions */
    image_id = gimp_image_new(width, height, GIMP_RGB);
    gimp_image_set_filename(image_id, filename);

    /* Loop over each of the frames */
    for (i = 0; i < nFrames; ++i) {

        gint32           layer_id;
        WebPMuxFrameInfo info;

        /* Load the frame */
        err = WebPMuxGetFrame(mux, i, &info);
        if (err != WEBP_MUX_OK) {
            goto error;
        }

        /* Attempt to load the layer and immediately free the data */
        layer_id = load_layer(image_id, &info, i);
        WebPDataClear(&info.bitstream);

        /* Ensure there were no errors loading the layer */
        if (layer_id == -1) {
            goto error;
        }

        gimp_image_insert_layer(image_id, layer_id, -1, 0);
    }

    /* Assign the image to the return value */
    ret = image_id;

error:

    if (mux) {
        WebPMuxDelete(mux);
    }

    if (indata) {
        g_free(indata);
    }

    return ret;
}
