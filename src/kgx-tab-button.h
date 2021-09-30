/* kgx-tab-button.h
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

#define KGX_TYPE_TAB_BUTTON (kgx_tab_button_get_type())

G_DECLARE_FINAL_TYPE (KgxTabButton, kgx_tab_button, KGX, TAB_BUTTON, GtkButton)

GtkWidget  *kgx_tab_button_new      (void);

HdyTabView *kgx_tab_button_get_view (KgxTabButton *self);
void        kgx_tab_button_set_view (KgxTabButton *self,
                                     HdyTabView   *view);

G_END_DECLS
