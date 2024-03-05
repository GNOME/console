/* kgx-paste-dialog.h
 *
 * Copyright 2024 Zander Brown
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

#include "kgx-enums.h"

G_BEGIN_DECLS

typedef enum {
  KGX_PASTE_ANYWAY,
  KGX_PASTE_CANCELLED,
} KgxPasteDialogResult;


#define KGX_TYPE_PASTE_DIALOG (kgx_paste_dialog_get_type ())

G_DECLARE_FINAL_TYPE (KgxPasteDialog, kgx_paste_dialog, KGX, PASTE_DIALOG, GObject)

void                 kgx_paste_dialog_run           (KgxPasteDialog                 *restrict       self,
                                                     GtkWidget                      *restrict       parent,
                                                     GCancellable                   *restrict       cancellable,
                                                     GAsyncReadyCallback                            callback,
                                                     gpointer                                       user_data);
KgxPasteDialogResult kgx_paste_dialog_run_finish    (KgxPasteDialog                 *restrict       self,
                                                     GAsyncResult                   *restrict       res,
                                                     const char          *restrict  *restrict const content,
                                                     GError                        **restrict       error);

G_END_DECLS
