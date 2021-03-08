/* kgx-nautilus-menu-item.h
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

#include "kgx-nautilus.h"

G_BEGIN_DECLS

#define KGX_TYPE_NAUTILUS_MENU_ITEM kgx_nautilus_menu_item_get_type ()

struct _KgxNautilusMenuItem {
  NautilusMenuItem  parent_instance;

  KgxNautilus      *extension;
  GtkWidget        *window;
  GFile            *file;
};

G_DECLARE_FINAL_TYPE (KgxNautilusMenuItem, kgx_nautilus_menu_item, KGX, NAUTILUS_MENU_ITEM, NautilusMenuItem)

void              kgx_nautilus_menu_item_register (GTypeModule *module);
NautilusMenuItem *kgx_nautilus_menu_item_new      (KgxNautilus *extension,
                                                   GtkWidget   *window,
                                                   GFile       *file,
                                                   gboolean     is_back);

G_END_DECLS
