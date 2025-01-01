/* kgx-shared-closures.h
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

#include <adwaita.h>

G_BEGIN_DECLS


G_GNUC_UNUSED
static AdwStyleManager *
kgx_style_manager_for_display (G_GNUC_UNUSED GObject *self,
                               GdkDisplay            *display)
{
  if (G_UNLIKELY (!display)) {
    return NULL;
  }

  return g_object_ref (adw_style_manager_get_for_display (display));
}


G_GNUC_UNUSED
static GtkSettings *
kgx_gtk_settings_for_display (G_GNUC_UNUSED GObject *self,
                              GdkDisplay            *display)
{
  if (G_UNLIKELY (!display)) {
    return NULL;
  }

  return g_object_ref (gtk_settings_get_for_display (display));
}

G_END_DECLS
