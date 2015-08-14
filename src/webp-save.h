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

#ifndef __WEBP_SAVE_H__
#define __WEBP_SAVE_H__

#include <glib.h>

gboolean save_image(const gchar *filename,
                    gint32       drawable_ID,
                    gchar       *preset,
                    gboolean     lossless,
                    gfloat       quality,
                    gfloat       alpha_quality,
                    GError     **error);

#endif /* __WEBP_SAVE_H__ */
