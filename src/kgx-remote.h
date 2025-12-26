/* kgx-remote.h
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
kgx_is_remote (const char *basename, GStrv argv)
{
  if (G_UNLIKELY (g_strcmp0 (basename, "ssh") == 0
                  || g_strcmp0 (basename, "telnet") == 0
                  || g_strcmp0 (basename, "mosh-client") == 0
                  || g_strcmp0 (basename, "mosh") == 0
                  || g_strcmp0 (basename, "et") == 0)) {
    return TRUE;
  }

  if (G_UNLIKELY (g_strcmp0 (basename, "waypipe") == 0)) {
    for (size_t i = 1; argv[i]; i++) {
      if (G_UNLIKELY (g_strcmp0 (argv[i], "ssh") == 0
                      || g_strcmp0 (argv[i], "telnet") == 0)) {
        return TRUE;
      }
    }
  }

  return FALSE;
}


G_END_DECLS
