/* kgx-playbox.h
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

#include "kgx-config.h"

#include <glib.h>

G_BEGIN_DECLS


G_GNUC_UNUSED
static inline gboolean
kgx_is_playbox (const char *basename, GStrv argv)
{
  g_autofree char *argv_1_basename = NULL;

  /* If there isn't a second term, we can just bail out */
  if (!argv[1]) {
    return FALSE;
  }

  /* Look for `toolbox enter` */
  if (G_UNLIKELY (g_strcmp0 (basename, "toolbox") == 0
                  && g_strcmp0 (argv[1], "enter") == 0)) {
    return TRUE;
  }

  /* Now we look for distrobox */

  /* If there isn't a third term, it's not `distrobox enter` */
  if (!argv[1] || !argv[2]) {
    return FALSE;
  }

  argv_1_basename = g_path_get_basename (argv[1]);

  return G_UNLIKELY (g_strcmp0 (argv_1_basename, "distrobox") == 0
                     && g_strcmp0 (argv[2], "enter") == 0);
}


G_END_DECLS
