/* kgx-search-box.h
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

#pragma once

#include <gtk/gtk.h>
#include <vte/vte.h>

G_BEGIN_DECLS

#define KGX_TYPE_SEARCH_BOX (kgx_search_box_get_type())

/**
 * KgxSearchBox:
 * @parent: The #GtkSearchBar that contains the box
 * @entry: The #GtkSearchEntry we contain
 * @parent_width: Width of the @parent
 * 
 * Stability: Private
 */
struct _KgxSearchBox
{
  /*< private >*/
  GtkBox parent_instance;

  /*< public >*/
  GtkWidget *parent;
  GtkWidget *entry;

  int parent_width;
};

G_DECLARE_FINAL_TYPE (KgxSearchBox, kgx_search_box, KGX, SEARCH_BOX, GtkBox)

const char *kgx_search_box_get_search (KgxSearchBox *self);
void        kgx_search_box_focus      (KgxSearchBox *self);

G_END_DECLS
