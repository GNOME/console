/* kgx-file-closures.h
 *
 * Copyright 2025 Zander Brown
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

#include <gio/gio.h>

G_BEGIN_DECLS


G_GNUC_UNUSED
static char *
kgx_file_as_subtitle (G_GNUC_UNUSED GObject *object,
                      GFile                 *file,
                      const char            *window_title)
{
  g_autoptr (GFile) home = NULL;
  g_autoptr (GError) error = NULL;
  g_autofree char *path_raw = NULL;
  g_autofree char *path_utf8 = NULL;
  g_autofree char *path_rel_raw = NULL;
  g_autofree char *path_rel_utf8 = NULL;
  g_autofree char *short_home = NULL;
  const char *home_path = NULL;

  if (!file) {
    return NULL;
  }

  path_raw = g_file_get_path (file);
  if (G_UNLIKELY (!path_raw || g_strcmp0 (path_raw, window_title) == 0)) {
    return NULL;
  }

  path_utf8 = g_filename_to_utf8 (path_raw, -1, NULL, NULL, &error);
  if (G_UNLIKELY (error)) {
    g_debug ("path had unexpected encoding (%s)", error->message);
    return g_file_get_uri (file);
  }

  home_path = g_get_home_dir ();
  if (G_UNLIKELY (!g_str_has_prefix (path_raw, home_path))) {
    return g_steal_pointer (&path_utf8);
  }

  home = g_file_new_for_path (home_path);
  if (!g_file_equal (home, file)) {
    path_rel_raw = g_file_get_relative_path (home, file);

    /* In practice we should have discounted non-utf8 already, but better
     * safe than sorry and replacement chars beats a crash */
    path_rel_utf8 = g_filename_display_name (path_rel_raw);

    short_home = g_strdup_printf ("~/%s", path_rel_utf8);
  } else {
    short_home = g_strdup ("~");
  }

  if (G_LIKELY (g_strcmp0 (short_home, window_title) == 0)) {
    /* Avoid duplicating the title */
    return NULL;
  }

  return g_steal_pointer (&short_home);
}


G_GNUC_UNUSED
static char *
kgx_file_as_display (G_GNUC_UNUSED GObject *object,
                     GFile                 *file)
{
  g_autofree char *raw_path = NULL;

  if (!file) {
    return NULL;
  }

  raw_path = g_file_get_path (file);

  /* Probably impossible in KGX, but you never know */
  if (G_UNLIKELY (!raw_path)) {
    return NULL;
  }

  return g_filename_display_name (raw_path);
}


G_GNUC_UNUSED
static char *
kgx_file_as_display_or_uri (G_GNUC_UNUSED GObject *object,
                            GFile                 *file)
{
  g_autofree char *path_raw = NULL;
  g_autofree char *path_utf8 = NULL;
  g_autoptr (GError) error = NULL;

  if (!file) {
    return NULL;
  }

  path_raw = g_file_get_path (file);
  if (G_UNLIKELY (!path_raw)) {
    return g_file_get_uri (file);
  }

  path_utf8 = g_filename_to_utf8 (path_raw, -1, NULL, NULL, &error);
  if (G_UNLIKELY (error)) {
    g_debug ("path had unexpected encoding (%s)", error->message);
    return g_file_get_uri (file);
  }

  return g_steal_pointer (&path_utf8);
}


G_END_DECLS
