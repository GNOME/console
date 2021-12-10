/* kgx-term-app-window.h
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

#include <gtk/gtk.h>
#include <gio/gdesktopappinfo.h>

#include "kgx-window.h"

G_BEGIN_DECLS

#define KGX_TYPE_TERM_APP_WINDOW kgx_term_app_window_get_type ()

/**
 * KgxTermAppWindow:
 * @entry: the #GDesktopAppInfo the window is for
 * @cancellable: the #GCancellable for operations specific to this window
 *
 * Stability: Private
 */
struct _KgxTermAppWindow {
  /*< private >*/
  KgxWindow        parent_instance;

  /*< public >*/
  GDesktopAppInfo *entry;
  GCancellable    *cancellable;
};

G_DECLARE_FINAL_TYPE (KgxTermAppWindow, kgx_term_app_window, KGX, TERM_APP_WINDOW, KgxWindow)


G_END_DECLS
