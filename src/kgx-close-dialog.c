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
#include "kgx-close-dialog-row.h"

G_DEFINE_TYPE (KgxCloseDialog, kgx_close_dialog, GTK_TYPE_DIALOG)

static void
kgx_close_dialog_class_init (KgxCloseDialogClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               RES_PATH "kgx-close-dialog.ui");

  gtk_widget_class_bind_template_child (widget_class, KgxCloseDialog, list);
}

static void
separator_header (GtkListBoxRow *row,
                  GtkListBoxRow *before,
                  gpointer       data)
{
  GtkWidget *header;

  g_return_if_fail (GTK_IS_LIST_BOX_ROW (row));

  if (before == NULL) {
    gtk_list_box_row_set_header (row, NULL);

    return;
  } else if (gtk_list_box_row_get_header (row) != NULL) {
    return;
  }

  header = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_widget_show (header);

  gtk_list_box_row_set_header (row, header);
}

static void
kgx_close_dialog_init (KgxCloseDialog *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_list_box_set_header_func (GTK_LIST_BOX (self->list),
                                separator_header,
                                NULL,
                                NULL);
}

/**
 * kgx_close_dialog_add_command:
 * @self: the #KgxCloseDialog
 * @command: the command the row is for
 * 
 * Adds a row to the #GtkListBox
 * 
 * Since: 0.2.0
 */
void
kgx_close_dialog_add_command (KgxCloseDialog *self,
                              const char     *command)
{
  GtkWidget *row;

  g_return_if_fail (KGX_IS_CLOSE_DIALOG (self));

  row = g_object_new (KGX_TYPE_CLOSE_DIALOG_ROW,
                      "command", command,
                      NULL);

  gtk_container_add (GTK_CONTAINER (self->list), row);
}
