/**
 * gimp-webp - WebP Plugin for the GIMP
 * Copyright (C) 2015  Nathan Osman
 * Copyright (C) 2016  Ben Touchette
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
#include <libgimp/gimpui.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <glib/gprintf.h>
#include <gegl.h>
#include <webp/decode.h>
#include <webp/demux.h>
#include <webp/mux.h>
#include "webp-load.h"

void create_layer(gint32 image_ID, uint8_t *layer_data, gint32 position,
                  gchar *name, gint width, gint height, gint32 offsetx,
                  gint32 offsety, gboolean has_alpha)
{
  gint32          layer_ID;
  GeglBuffer     *geglbuffer;
  GeglRectangle   extent;
  GeglColor       color;
  guchar         *buff;
  gint32          alpha;

  layer_ID = gimp_layer_new(image_ID, name,
                            width, height,
                            GIMP_RGBA_IMAGE,
                            100,
                            GIMP_NORMAL_MODE);

  gimp_image_insert_layer(image_ID, layer_ID, -1, position);

  if (offsetx > 0 || offsety > 0)
    {
      gimp_layer_set_offsets(layer_ID, offsetx, offsety);
    }

  /* Retrieve the buffer for the layer */
  geglbuffer = gimp_drawable_get_buffer(layer_ID);

  /* Copy the image data to the region */
  gegl_rectangle_set (&extent, 0, 0, width, height);
  gegl_buffer_set (geglbuffer, &extent, 0, NULL, layer_data, GEGL_AUTO_ROWSTRIDE);

  /* Flush the drawable and detach */
  gegl_buffer_flush (geglbuffer);

  g_object_unref(geglbuffer);
}

gint32 load_image(const gchar *filename,
                  gboolean    interactive,
                  GError      **error)
{
  gchar                *indata;
  gsize                 indatalen;
  uint8_t              *outdata;
  gint                  width;
  gint                  height;
  gint32                image_ID;
  gint32                layer_ID;
  gint                  has_alpha;
  GFile                *file;
  GimpMetadata         *metadata;
  WebPData              bitstream;
  WebPMux              *mux;
  WebPBitstreamFeatures features;
  WebPDecoderConfig     config;
  uint32_t              flag;
  int                   animation = FALSE;
  int                   icc = FALSE;
  int                   exif = FALSE;
  int                   xmp = FALSE;
  int                   alpha = FALSE;
  int                   frames = 0;
  GimpRGB               bgcolor;
  guchar                red, green, blue;

  /* Attempt to read the file contents from disk */
  if(g_file_get_contents(filename,
                         &indata,
                         &indatalen,
                         error) == FALSE)
    {
      return -1;
    }

  g_print("Loading WebP file %s\n", filename);

  /* Validate WebP data */
  if (!WebPGetInfo(indata, indatalen, &width, &height))
    {
      g_print("Invalid WebP file\n");
      return -1;
    }

  gegl_init(NULL, NULL);

  WebPData wp_data;
  wp_data.bytes = (uint8_t*)indata;
  wp_data.size = indatalen;
  mux = WebPMuxCreate(&wp_data, 1);
  if (mux == NULL)
    {
      return -1;
    }

  WebPMuxGetFeatures(mux, &flag);
  if (flag == 0)
    {
      animation = FALSE;
      icc = FALSE;
      exif = FALSE;
      xmp = FALSE;
      alpha = FALSE;
      frames = 0;
    }
  else
    {
      if (flag & ANIMATION_FLAG)
        {
          animation = TRUE;
        }
      if (flag & ICCP_FLAG)
        {
          icc = TRUE;
        }
      if (flag & EXIF_FLAG)
        {
          exif = TRUE;
        }
      if (flag & XMP_FLAG)
        {
          xmp = TRUE;
        }
      if (flag & ALPHA_FLAG)
        {
          alpha = TRUE;
        }
      g_print("\n");
    }

  /* TODO: decode the image in "chunks" or "tiles" */
  /* TODO: check if an alpha channel is present */

  /* Create the new image and associated layer */
  image_ID = gimp_image_new(width, height, GIMP_RGB);

  if (animation == FALSE)
    {
      /* Attempt to decode the data as a WebP image */
      outdata = WebPDecodeRGBA(indata, indatalen, &width, &height);

      /* Free the original compressed data */
      g_free(indata);

      /* Check to ensure the image data was loaded correctly */
      if(outdata == NULL)
        {
          return -1;
        }

      gchar name[255];
      sprintf((void*)name, "Background");
      create_layer(image_ID, outdata, 0, (gchar*)name, width, height, 0, 0, FALSE);

      /* Free the image data */
      free(outdata);
    }
  else
    {
      const WebPChunkId id = WEBP_CHUNK_ANMF;
      const char* const type_str = "frame";
      int nFrames;

      WebPMuxAnimParams params;
      WebPMuxGetAnimationParams(mux, &params);
      WebPMuxNumChunks(mux, id, &frames);

#ifdef __BACKGROUND_COLOR__
      blue = (params.bgcolor & 0xFF000000) >> 24;
      green = (params.bgcolor & 0xFF0000) >> 16;
      red = (params.bgcolor & 0xFF00) >> 8;
      gimp_rgb_set_uchar (&bgcolor, red, green, blue);
      gimp_context_set_background (&bgcolor);
#endif

      /* Attempt to decode the data as a WebP animation image */
      for (int loop = 0; loop < frames; loop++)
        {
          WebPMuxFrameInfo thisframe;
          int i = loop;
          if (WebPMuxGetFrame(mux, i, &thisframe) == WEBP_MUX_OK)
            {
              WebPGetFeatures(thisframe.bitstream.bytes, thisframe.bitstream.size, &features);
              outdata = WebPDecodeRGBA(thisframe.bitstream.bytes,
                                       thisframe.bitstream.size,
                                       &width, &height);

              if(outdata == NULL)
                {
                  return -1;
                }

              gchar name[255];
              sprintf((void*)name, "Frame %d", (loop+1));
              create_layer(image_ID, (uint8_t*)outdata, 0,
                           (gchar*)name, width, height,
                           thisframe.x_offset,
                           thisframe.y_offset,
                           features.has_alpha);

              /* Free the image data */
              free(outdata);
            }
          WebPDataClear(&thisframe.bitstream);
        }

      WebPDataClear(&wp_data);
    }

  if (icc == TRUE)
    {
      Babl             *type;
      WebPData          icc_profile;
      GimpColorProfile *profile;
      gint              image = 0;
      GimpPrecision     image_precision;
      gboolean          profile_linear = FALSE;
      WebPMuxGetChunk(mux, "ICCP", &icc_profile);
      profile = gimp_color_profile_new_from_icc_profile(
                  icc_profile.bytes, icc_profile.size, NULL);
      if (profile)
        {
          gimp_image_set_color_profile (image_ID, profile);
          g_object_unref (profile);
        }
    }

  if (exif == TRUE || xmp == TRUE)
    {
      if (exif == TRUE)
        {
          WebPData exif;
          WebPMuxGetChunk(mux, "EXIF", &exif);
        }

      if (xmp == TRUE)
        {
          WebPData xmp;
          WebPMuxGetChunk(mux, "XMP ", &xmp);
        }

      file = g_file_new_for_path (filename);
      metadata = gimp_image_metadata_load_prepare (image_ID, "image/webp",
                                                   file, NULL);
      if (metadata)
        {
          gimp_image_metadata_load_finish (image_ID, "image/webp",
                                           metadata, GIMP_METADATA_LOAD_ALL,
                                           interactive);
          g_object_unref (metadata);
        }

      g_object_unref (file);
    }

  gimp_image_set_filename(image_ID, filename);

  return image_ID;
}
