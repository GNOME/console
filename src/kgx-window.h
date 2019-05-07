/* kgx-window.h
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

G_BEGIN_DECLS

#define KGX_TYPE_WINDOW (kgx_window_get_type())

G_DECLARE_FINAL_TYPE (KgxWindow, kgx_window, KGX, WINDOW, GtkApplicationWindow)

const char *kgx_window_get_working_dir (KgxWindow *self);
void        kgx_window_push_root       (KgxWindow *self);
void        kgx_window_pop_root        (KgxWindow *self);
void        kgx_window_push_remote     (KgxWindow *self);
void        kgx_window_pop_remote      (KgxWindow *self);


G_END_DECLS
