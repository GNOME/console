/* kgx-close-dialog.h
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

#include <gtk/gtk.h>

#include "kgx-enums.h"

G_BEGIN_DECLS

typedef enum {
  KGX_CONTEXT_WINDOW,
  KGX_CONTEXT_TAB,
} KgxCloseDialogContext;


typedef enum {
  KGX_CLOSE_ANYWAY,
  KGX_CLOSE_CANCELLED,
} KgxCloseDialogResult;


#define KGX_TYPE_CLOSE_DIALOG (kgx_close_dialog_get_type ())

G_DECLARE_FINAL_TYPE (KgxCloseDialog, kgx_close_dialog, KGX, CLOSE_DIALOG, GObject)

void                 kgx_close_dialog_run           (KgxCloseDialog       *restrict self,
                                                     GtkWidget            *restrict parent,
                                                     GCancellable         *restrict cancellable,
                                                     GAsyncReadyCallback            callback,
                                                     gpointer                       user_data);
KgxCloseDialogResult kgx_close_dialog_run_finish    (KgxCloseDialog       *restrict self,
                                                     GAsyncResult         *restrict res,
                                                     GError              **restrict error);

G_END_DECLS
