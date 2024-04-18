/* kgx-pages.h
 *
 * Copyright 2019-2023 Zander Brown
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

#include "kgx-tab.h"
#include "kgx-train.h"

G_BEGIN_DECLS

#define KGX_TYPE_PAGES (kgx_pages_get_type())

/**
 * KgxPagesClass:
 */
struct _KgxPagesClass {
  AdwBinClass parent;
};


G_DECLARE_DERIVABLE_TYPE (KgxPages, kgx_pages, KGX, PAGES, AdwBin)


void        kgx_pages_add_page       (KgxPages   *self,
                                      KgxTab     *page);
int         kgx_pages_count          (KgxPages   *self);
GPtrArray  *kgx_pages_get_children   (KgxPages   *self);
void        kgx_pages_focus_page     (KgxPages   *self,
                                      KgxTab     *page);
KgxStatus   kgx_pages_current_status (KgxPages   *self);
gboolean    kgx_pages_is_ringing          (KgxPages   *self);
void        kgx_pages_close_page          (KgxPages  *self);
void        kgx_pages_detach_page         (KgxPages  *self);
AdwTabPage *kgx_pages_get_selected_page   (KgxPages  *self);

G_END_DECLS
