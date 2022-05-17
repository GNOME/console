/* kgx-util.c
 *
 * Copyright 2022 Zander Brown
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

#include "kgx-util.h"

/**
 * kgx_util_transform_uris_to_quoted_fuse_paths:
 * @uris:
 *
 * Transforms those URIs in @uris to shell-quoted paths that point to
 * GIO fuse paths.
 */
void
kgx_util_transform_uris_to_quoted_fuse_paths (GStrv uris)
{
  guint i;

  if (!uris) {
    return;
  }

  for (i = 0; uris[i]; ++i) {
    g_autoptr (GFile) file = NULL;
    g_autofree char  *path = NULL;

    file = g_file_new_for_uri (uris[i]);

    path = g_file_get_path (file);
    if (path) {
      char *quoted;

      quoted = g_shell_quote (path);
      g_free (uris[i]);

      uris[i] = quoted;
    }
  }
}


char *
kgx_util_concat_uris (GStrv  uris,
                      gsize *length)
{
  GString *string;
  gsize    len;
  guint    i;

  len = 0;
  for (i = 0; uris[i]; ++i) {
    len += strlen (uris[i]) + 1;
  }

  if (length) {
    *length = len;
  }

  string = g_string_sized_new (len + 1);
  for (i = 0; uris[i]; ++i) {
    g_string_append (string, uris[i]);
    g_string_append_c (string, ' ');
  }

  return g_string_free (string, FALSE);
}
