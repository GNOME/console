/* kgx-pages.h
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

#include "kgx-page.h"

G_BEGIN_DECLS

#define KGX_TYPE_PAGES (kgx_pages_get_type())

/**
 * KgxPagesClass:
 * 
 * Since: 0.3.0
 */
struct _KgxPagesClass
{
  /*< private >*/
  GtkOverlayClass parent;

  /*< public >*/
};

G_DECLARE_DERIVABLE_TYPE (KgxPages, kgx_pages, KGX, PAGES, GtkOverlay)

void kgx_pages_focus_terminal (KgxPages   *self);
void kgx_pages_search_forward (KgxPages   *self);
void kgx_pages_search_back    (KgxPages   *self);
void kgx_pages_search         (KgxPages   *self,
                               const char *search);
void kgx_pages_add_page       (KgxPages   *self,
                               KgxPage    *page);

G_END_DECLS
