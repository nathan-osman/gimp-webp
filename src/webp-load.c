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
#include <libgimp/gimp.h>
#include <stdio.h>
#include <stdlib.h>
#include <webp/decode.h>

#include "webp-load.h"

gint32 load_image(const gchar *filename,
                  GError      **error)
{
    gchar        *indata;
    gsize         indatalen;
    uint8_t      *outdata;
    int           width;
    int           height;
    gint32        image_ID;
    gint32        layer_ID;
    GimpDrawable *drawable;
    GimpPixelRgn  region;

    /* Attempt to read the file contents from disk */
    if(g_file_get_contents(filename,
                           &indata,
                           &indatalen,
                           error) == FALSE) {
        return -1;
    }

    /* TODO: decode the image in "chunks" or "tiles" */
    /* TODO: check if an alpha channel is present */

    /* Attempt to decode the data as a WebP image */
    outdata = WebPDecodeRGBA(indata, indatalen, &width, &height);

    /* Free the original compressed data */
    g_free(indata);

    /* Check to ensure the image data was loaded correctly */
    if(outdata == NULL) {
        return -1;
    }

    /* Create the new image and associated layer */
    image_ID = gimp_image_new(width, height, GIMP_RGB);
    layer_ID = gimp_layer_new(image_ID,
                              "Background",
                              width,
                              height,
                              GIMP_RGBA_IMAGE,
                              100,
                              GIMP_NORMAL_MODE);

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
                            outdata,
                            0, 0,
                            width, height);

    /* Flush the drawable and detach */
    gimp_drawable_flush(drawable);
    gimp_drawable_detach(drawable);

    /* Free the image data */
    free(outdata);

    /* Add the layer to the image and set the filename */
    gimp_image_insert_layer(image_ID, layer_ID, -1, 0);
    gimp_image_set_filename(image_ID, filename);

    return image_ID;
}
