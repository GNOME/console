/* kgx-shared-closures.h
 *
 * Copyright 2024-2025 Zander Brown
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

#include "kgx-utils.h"

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


G_GNUC_UNUSED
static char *
kgx_text_or_fallback (G_GNUC_UNUSED GObject *self,
                      const char            *text,
                      const char            *fallback)
{
  if (!kgx_str_non_empty (text)) {
    return g_strdup (fallback);
  }

  return g_strdup (text);
}


G_GNUC_UNUSED
static GObject *
kgx_object_or_fallback (G_GNUC_UNUSED GObject *self,
                        GObject               *object,
                        GObject               *fallback)
{
  if (G_UNLIKELY (!object)) {
    return fallback ? g_object_ref (fallback) : NULL;
  }

  return g_object_ref (object);
}


G_GNUC_UNUSED
static gboolean
kgx_enum_is (G_GNUC_UNUSED GObject *self,
             int                    a,
             int                    b)
{
  return a == b;
}


G_GNUC_UNUSED
static gboolean
kgx_flags_includes (G_GNUC_UNUSED GObject *self,
                    guint                  a,
                    guint                  b)
{
  return (a & b) == b;
}


G_END_DECLS
