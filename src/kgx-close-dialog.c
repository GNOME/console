/* kgx-close-dialog.c
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

/**
 * SECTION:kgx-close-dialog
 * @title: KgxCloseDialog
 * @short_description: Confirmation dialog to close a terminal with children
 * 
 * The "are you sure?" dialog when a terminal is closed whilst commands are
 * still running within it
 * 
 * Since: 0.2.0
 */

#include <glib/gi18n.h>

#include "kgx-config.h"
#include "kgx-close-dialog.h"
#include "kgx-process.h"
#include <handy.h>

GtkWidget *
kgx_close_dialog_new (GPtrArray *commands)
{
  g_autoptr (GtkBuilder) builder = NULL;
  GtkWidget *dialog, *list;

  builder = gtk_builder_new_from_resource (RES_PATH "kgx-close-dialog.ui");

  dialog = GTK_WIDGET (gtk_builder_get_object (builder, "dialog"));
  list = GTK_WIDGET (gtk_builder_get_object (builder, "list"));

  for (int i = 0; i < commands->len; i++) {
    KgxProcess *process = g_ptr_array_index (commands, i);
    GtkWidget *row;

    row = g_object_new (HDY_TYPE_ACTION_ROW,
                        "visible", TRUE,
                        "can-focus", FALSE,
                        "title", kgx_process_get_exec (process),
                        NULL);

    gtk_container_add (GTK_CONTAINER (list), row);
  }

  return dialog;
}
