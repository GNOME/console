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
#include "handy.h"

#include "kgx-terminal.h"
#include "kgx-process.h"
#include "kgx-enums.h"
#include "kgx-pages.h"

G_BEGIN_DECLS

#define KGX_WINDOW_STYLE_ROOT "root"
#define KGX_WINDOW_STYLE_REMOTE "remote"

/**
 * KgxZoom:
 * @KGX_ZOOM_IN: Make text bigger
 * @KGX_ZOOM_OUT: Shrink text
 *
 * Indicates the zoom direction the zoom action was triggered for
 *
 * See #KgxPage:zoom, #KgxPages:zoom
 *
 * Stability: Private
 */
typedef enum /*< enum,prefix=KGX >*/
{
  KGX_ZOOM_IN = 0,  /*< nick=in >*/
  KGX_ZOOM_OUT = 1, /*< nick=out >*/
} KgxZoom;


#define KGX_TYPE_WINDOW (kgx_window_get_type())

typedef struct _KgxWindowClass KgxWindowClass;
struct _KgxWindowClass {
  HdyApplicationWindowClass parent;
};

G_DECLARE_DERIVABLE_TYPE (KgxWindow, kgx_window, KGX, WINDOW, HdyApplicationWindow)

GFile      *kgx_window_get_working_dir (KgxWindow    *self);
void        kgx_window_show_status     (KgxWindow    *self,
                                        const char   *status);
KgxPages   *kgx_window_get_pages       (KgxWindow    *self);

void        kgx_window_get_size        (KgxWindow    *self,
                                        int          *width,
                                        int          *height);

G_END_DECLS
