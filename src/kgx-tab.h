/* kgx-tab.h
 *
 * Copyright 2019-2024 Zander Brown
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

#include "kgx-process.h"
#include "kgx-train.h"

G_BEGIN_DECLS


#ifndef __GTK_DOC_IGNORE__
typedef struct _KgxPages KgxPages;
#endif

#define KGX_TYPE_TAB kgx_tab_get_type ()

G_DECLARE_DERIVABLE_TYPE (KgxTab, kgx_tab, KGX, TAB, AdwBin)


/**
 * KgxTabClass:
 * @start: start whatever it is that run in the tab
 * @start_finish: complete @start
 *
 * Stability: Private
 */
struct _KgxTabClass {
  AdwBinClass parent;

  void         (*start)            (KgxTab               *tab,
                                    GAsyncReadyCallback   callback,
                                    gpointer              callback_data);
  KgxTrain   * (*start_finish)     (KgxTab               *tab,
                                    GAsyncResult         *res,
                                    GError              **error);

  void (*died)         (KgxTab               *self,
                        GtkMessageType        type,
                        const char           *message,
                        gboolean              success);
  void (*bell)         (KgxTab               *self);
};


guint       kgx_tab_get_id           (KgxTab               *self);
void        kgx_tab_start            (KgxTab               *self,
                                      GAsyncReadyCallback   callback,
                                      gpointer              callback_data);
void        kgx_tab_start_finish     (KgxTab               *self,
                                      GAsyncResult         *res,
                                      GError              **error);
void        kgx_tab_died             (KgxTab               *self,
                                      GtkMessageType        type,
                                      const char           *message,
                                      gboolean              success);
void        kgx_tab_bell              (KgxTab               *self);
KgxPages   *kgx_tab_get_pages        (KgxTab               *self);
gboolean    kgx_tab_is_active        (KgxTab               *self);
GPtrArray  *kgx_tab_get_children     (KgxTab               *self);
void        kgx_tab_extra_drop       (KgxTab               *self,
                                      const GValue         *value);
void        kgx_tab_set_initial_title (KgxTab              *self,
                                       const char          *title,
                                       GFile               *path);
void        kgx_tab_mark_working      (KgxTab              *self);
void        kgx_tab_unmark_working    (KgxTab              *self);

G_END_DECLS
