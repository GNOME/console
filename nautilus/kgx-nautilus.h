/* kgx-nautilus.h
 *
 * Copyright 2021 Zander Brown
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

#include <nautilus-extension.h>
#include <gio/gdesktopappinfo.h>

G_BEGIN_DECLS

#define KGX_TYPE_NAUTILUS kgx_nautilus_get_type ()

struct _KgxNautilus {
  GObject   parent_instance;

  GAppInfo *kgx;
};

G_DECLARE_FINAL_TYPE (KgxNautilus, kgx_nautilus, KGX, NAUTILUS, GObject)

void         kgx_nautilus_open         (KgxNautilus *self,
                                        GtkWidget   *window,
                                        GFile       *file);

G_END_DECLS
