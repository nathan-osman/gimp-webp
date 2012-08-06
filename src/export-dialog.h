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

#ifndef EXPORT_DIALOG_H
#define EXPORT_DIALOG_H

#include <webp/encode.h>

/* Displays the export dialog and returns 1 if the user
   accepted the export and 0 if the user cancelled the
   process. The chosen parameters are entered into the
   specified config. */
int export_dialog(WebPConfig *);

#endif // EXPORT_DIALOG_H
