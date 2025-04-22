/* kgx-icon-closures.h
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

#include "kgx-train.h"

#include "kgx-utils.h"

G_BEGIN_DECLS


G_GNUC_UNUSED
static GIcon *
kgx_status_as_icon (G_GNUC_UNUSED GObject *self,
                    KgxStatus              status)
{
  if (status & KGX_REMOTE) {
    return g_themed_icon_new ("status-remote-symbolic");
  } else if (status & KGX_PRIVILEGED) {
    return g_themed_icon_new ("status-privileged-symbolic");
  }

  return NULL;
}


G_GNUC_UNUSED
static GIcon *
kgx_ringing_as_icon (G_GNUC_UNUSED GObject *self,
                     gboolean               ringing)
{
  if (ringing) {
    return g_themed_icon_new ("bell-outline-symbolic");
  }

  return NULL;
}


G_END_DECLS
