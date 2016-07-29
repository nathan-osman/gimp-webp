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

#ifndef __WEBP_SAVE_H__
#define __WEBP_SAVE_H__

#include <glib.h>

#include "config.h"

typedef struct {
    gchar   *preset;
    gboolean lossless;
    gfloat   quality;
    gfloat   alpha_quality;
#ifdef WEBP_0_5
    gboolean animation;
    gboolean loop;
#endif
} WebPSaveParams;

gboolean save_image(const gchar    *filename,
#ifdef WEBP_0_5
                    gint32          nLayers,
                    gint32         *allLayers,
#endif
                    gint32          drawable_ID,
                    WebPSaveParams *params,
                    GError        **error);

#endif /* __WEBP_SAVE_H__ */
