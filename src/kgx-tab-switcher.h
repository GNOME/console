/* kgx-tab-switcher.h
 *
 * Copyright 2021 Purism SPC
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

#include <handy.h>

G_BEGIN_DECLS

#define KGX_TYPE_TAB_SWITCHER (kgx_tab_switcher_get_type())

G_DECLARE_FINAL_TYPE (KgxTabSwitcher, kgx_tab_switcher, KGX, TAB_SWITCHER, GtkBin)

GtkWidget   *kgx_tab_switcher_new       (void);

HdyTabView *kgx_tab_switcher_get_view   (KgxTabSwitcher *self);
void        kgx_tab_switcher_set_view   (KgxTabSwitcher *self,
                                         HdyTabView     *view);

void        kgx_tab_switcher_open       (KgxTabSwitcher *self);
void        kgx_tab_switcher_close      (KgxTabSwitcher *self);

gboolean    kgx_tab_switcher_get_narrow (KgxTabSwitcher *self);

G_END_DECLS
