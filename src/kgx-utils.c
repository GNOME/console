/* kgx-utils.c
 *
 * Copyright 2019 Zander Brown
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

/**
 * SECTION:kgx-utils
 * @title: Utilities
 * @short_description: Various useful functions
 * 
 * Oddities that don't belong anywhere else
 * 
 * Since: 0.2.0
 */

#define G_LOG_DOMAIN "Kgx"

#include <glib/gi18n.h>

#include "kgx-config.h"
#include "kgx-utils.h"

/**
 * kgx_get_app_colour:
 * @info: the #GAppInfo to pick a colour for
 * @colour: (out caller-allocates): the picked #GdkRGBA
 * 
 * Looks at the icon for @info and picks a @colour to represent it
 * 
 * Returns: %TRUE on success
 * 
 * Stability: Private
 * 
 * Since: 0.2.0
 */
gboolean
kgx_get_app_colour (GAppInfo *info, GdkRGBA *colour)
{
  GIcon *icon;
  g_autoptr (GtkIconInfo) icon_info = NULL;
  g_autoptr (GdkPixbuf) pixbuf = NULL;
  g_autoptr (GError) error = NULL;
  int n_channels = 0;
  int width = 32;
  int height = 32;
  int rowstride;
  guchar *pixels;
  guchar *p;
  int red = 0;
  int blue = 0;
  int green = 0;
  int count = 0;

  g_return_val_if_fail (G_IS_APP_INFO (info), FALSE);

  if (colour == NULL) {
    return FALSE;
  }

  icon = g_app_info_get_icon (info);

  if (info == NULL) {
    return FALSE;
  }

  icon_info = gtk_icon_theme_lookup_by_gicon (gtk_icon_theme_get_default (),
                                              icon,
                                              32,
                                              GTK_ICON_LOOKUP_FORCE_SIZE);

  if (icon_info == NULL) {
    return FALSE;
  }

  pixbuf = gtk_icon_info_load_icon (icon_info, &error);

  if (error) {
    g_warning ("Can't load icon: %s", error->message);

    return FALSE;
  }

  g_return_val_if_fail (gdk_pixbuf_get_colorspace (pixbuf) == GDK_COLORSPACE_RGB, FALSE);
  g_return_val_if_fail (gdk_pixbuf_get_bits_per_sample (pixbuf) == 8, FALSE);

  n_channels = gdk_pixbuf_get_n_channels (pixbuf);

  if (n_channels == 4) {
    g_return_val_if_fail (gdk_pixbuf_get_has_alpha (pixbuf), FALSE);
  }

  height = gdk_pixbuf_get_height (pixbuf);
  width = gdk_pixbuf_get_width (pixbuf);
  rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  pixels = gdk_pixbuf_get_pixels (pixbuf);

  for (int x = 0; x < width; x++) {
    for (int y = 0; y < height; y++) {
      p = pixels + y * rowstride + x * n_channels;
      if (G_LIKELY (n_channels == 3 || (n_channels == 4 && p[3] > 0))) {
        red += p[0];
        green += p[1];
        blue += p[2];
        count++;
      }
    }
  }

  red /= count;
  green /= count;
  blue /= count;

  colour->alpha = 1.0;
  colour->red = red / 255.0;
  colour->green = green / 255.0;
  colour->blue = blue / 255.0;

  return TRUE;
}