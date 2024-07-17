/* kgx-colour-utils.h
 *
 * Copyright 2024 Zander Brown
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

#pragma once

#include "kgx-config.h"

#include <glib/gi18n.h>

#include <gdk/gdk.h>

G_BEGIN_DECLS


static inline char *
kgx_colour_to_string (GdkRGBA *colour)
{
  /* intentionally not using gdk_rgba_to_string here */
  return g_strdup_printf ("%02X%02X%02X",
                          (unsigned int) (colour->red * 255),
                          (unsigned int) (colour->green * 255),
                          (unsigned int) (colour->blue * 255));
}


typedef enum /*< enum,prefix=KGX >*/ {
  KGX_COLOUR_PARSE_ERROR_WRONG_LENGTH,
} KgxColourParseError;


#define KGX_COLOUR_PARSE_ERROR (kgx_colour_parse_error_quark ())


GQuark          kgx_colour_parse_error_quark   (void);
void            kgx_colour_from_string         (const char *const   string,
                                                GdkRGBA    *const   colour,
                                                GError            **error);


G_END_DECLS
