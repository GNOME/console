/* kgx-page.h
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

#include "kgx-pages-tab.h"
#include "kgx-terminal.h"

G_BEGIN_DECLS

#define KGX_TYPE_PAGE (kgx_page_get_type())

/**
 * KgxPageClass:
 * 
 * Stability: Private
 */
struct _KgxPageClass
{
  /*< private >*/
  GtkBinClass parent;

  /*< public >*/
};

G_DECLARE_DERIVABLE_TYPE (KgxPage, kgx_page, KGX, PAGE, GtkBin)


void kgx_page_connect_tab      (KgxPage     *self,
                                KgxPagesTab *tab);
void kgx_page_connect_terminal (KgxPage     *self,
                                KgxTerminal *term);
void kgx_page_focus_terminal   (KgxPage     *self);
void kgx_page_search_forward   (KgxPage     *self);
void kgx_page_search_back      (KgxPage     *self);
void kgx_page_search           (KgxPage     *self,
                                const char  *search);

G_END_DECLS
