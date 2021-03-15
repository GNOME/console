/* kgx-close-dialog.h
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
#include "kgx-enums.h"

G_BEGIN_DECLS

typedef enum {
  KGX_CONTEXT_WINDOW,
  KGX_CONTEXT_TAB,
} KgxCloseDialogContext;

GtkWidget *kgx_close_dialog_new (KgxCloseDialogContext  context,
                                 GPtrArray             *commands);

G_END_DECLS
