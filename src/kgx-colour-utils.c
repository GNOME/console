/* kgx-colour-utils.c
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

#include "kgx-config.h"

#include <glib/gi18n.h>

#include "kgx-colour-utils.h"


G_DEFINE_QUARK (kgx-colour-parse-error-quark, kgx_colour_parse_error)


static inline double
read_component (const char **const ptr)
{
  int first = g_unichar_xdigit_value (g_utf8_get_char (*ptr));
  int second = 0;

  *ptr = g_utf8_next_char (*ptr);
  second = g_unichar_xdigit_value (g_utf8_get_char (*ptr));
  *ptr = g_utf8_next_char (*ptr);

  return ((first << 4) + second) / 255.0;
}


void
kgx_colour_from_string (const char *const   string,
                        GdkRGBA    *const   colour,
                        GError            **error)
{
  const char *ptr = string;

  g_return_if_fail (string != NULL);
  g_return_if_fail (colour != NULL);
  g_return_if_fail (error != NULL);

  if (g_utf8_strlen (string, -1) != 6) {
    g_set_error_literal (error,
                         KGX_COLOUR_PARSE_ERROR,
                         KGX_COLOUR_PARSE_ERROR_WRONG_LENGTH,
                         _("Color string is wrong length"));
    return;
  }

  colour->red = read_component (&ptr);
  colour->green = read_component (&ptr);
  colour->blue = read_component (&ptr);
  colour->alpha = 0.0;
}
