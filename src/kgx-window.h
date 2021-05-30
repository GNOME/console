/* kgx-window.h
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

G_BEGIN_DECLS

#define KGX_WINDOW_STYLE_ROOT "root"
#define KGX_WINDOW_STYLE_REMOTE "remote"
#define KGX_WINDOW_STYLE_RINGING "bell"


#define KGX_TYPE_WINDOW (kgx_window_get_type ())

struct _KgxWindowClass {
  AdwApplicationWindowClass parent;
};

G_DECLARE_DERIVABLE_TYPE (KgxWindow, kgx_window, KGX, WINDOW, AdwApplicationWindow)

GFile      *kgx_window_get_working_dir (KgxWindow    *self);
void        kgx_window_add_tab         (KgxWindow    *self,
                                        KgxTab       *tab);

G_END_DECLS
