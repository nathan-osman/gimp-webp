/*=======================================================================
              WebP load / save plugin for the GIMP
                 Copyright 2012 - Nathan Osman

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
=======================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <libgimp/gimp.h>
#include <webp/encode.h>

#include "export-dialog.h"
#include "write-webp.h"

/* Attempts to create a WebP file from the specified drawable using
   the supplied quality and flags. */
int write_webp(const gchar * filename, gint drawable_id, float quality, int flags)
{
    GimpDrawable * drawable;
    gint bpp;
    GimpPixelRgn rgn;
    long int data_size;
    void * image_data;
    size_t output_size;
    uint8_t * raw_data;
    FILE * file;

    /* Use the drawable ID to get the drawable. */
    drawable = gimp_drawable_get(drawable_id);

    /* Get the number of bits-per-pixel (BPP). */
    bpp = gimp_drawable_bpp(drawable_id);

    /* Get the entire contents of the drawable. */
    gimp_pixel_rgn_init(&rgn,
                        drawable,
                        0, 0,
                        drawable->width,
                        drawable->height,
                        FALSE, FALSE);

    /* Determine exactly how much space we need to allocate for
       the contents of the image. Then allocate that amount. */
    data_size = drawable->width * drawable->height * bpp;
    image_data = malloc(data_size);

    if(!image_data)
    {
        gimp_drawable_detach(drawable);
        return 0;
    }

    /* Now copy the image data to our array. */
    gimp_pixel_rgn_get_rect(&rgn,
                            (guchar *)image_data,
                            0, 0,
                            drawable->width,
                            drawable->height);

    /* Now that we have the actual image data, encode it. Based on
       the flags provided, we create a lossy or lossless image. */
    if(flags & WEBP_OPTIONS_LOSSLESS)
    {
        output_size = WebPEncodeLosslessRGB((const uint8_t *)image_data,
                                            drawable->width,
                                            drawable->height,
                                            drawable->width * 3,
                                            &raw_data);
    }
    else
    {
        output_size = WebPEncodeRGB((const uint8_t *)image_data,
                                    drawable->width,
                                    drawable->height,
                                    drawable->width * 3,
                                    quality,
                                    &raw_data);
    }

    /* Free the uncompressed image data. */
    free(image_data);

    /* Detach the drawable. (I'm not 100% sure what this does.) */
    gimp_drawable_detach(drawable);

    /* Make sure that there was no error during the encoding process. */
    if(!output_size)
    {
        free(raw_data);
        return 0;
    }

    /* Open the file we are writing to. */
    file = fopen(filename, "wb");
    if(!file)
    {
        free(raw_data);
        return 0;
    }

    /* Write the actual data to the file, free the data, and close the file. */
    fwrite(raw_data, output_size, 1, file);
    free(raw_data);
    fclose(file);

    return 1;
}
