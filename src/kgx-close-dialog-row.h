/* kgx-close-dialog-row.h
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
#include <vte/vte.h>

G_BEGIN_DECLS

#define KGX_TYPE_CLOSE_DIALOG_ROW (kgx_close_dialog_row_get_type())

/**
 * KgxCloseDialogRow:
 * @command: the kgx_process_get_exec() of the pid being represented
 * @icon: the #GIcon to represent this process
 * 
 * Stability: Private
 */
struct _KgxCloseDialogRow
{
  /*< private >*/
  GtkListBoxRow  parent_instance;

  /*< public >*/
  char          *command;
  GIcon         *icon;
};

G_DECLARE_FINAL_TYPE (KgxCloseDialogRow, kgx_close_dialog_row, KGX, CLOSE_DIALOG_ROW, GtkListBoxRow)

G_END_DECLS
