/* kgx-close-dialog-row.c
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
 * SECTION:kgx-close-dialog-row
 * @title: KgxCloseDialogRow
 * @short_description: A row in a #KgxCloseDialog
 * 
 * A simple row with a #GtkLabel to represent a running process
 * 
 * Since: 0.2.0
 */

#include <glib/gi18n.h>

#include "kgx-config.h"
#include "kgx-close-dialog-row.h"

G_DEFINE_TYPE (KgxCloseDialogRow, kgx_close_dialog_row, GTK_TYPE_LIST_BOX_ROW)

enum {
  PROP_0,
  PROP_COMMAND,
  PROP_ICON,
  LAST_PROP
};

static GParamSpec *pspecs[LAST_PROP] = { NULL, };


static void
kgx_close_dialog_row_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  KgxCloseDialogRow *self = KGX_CLOSE_DIALOG_ROW (object);

  switch (property_id) {
    case PROP_COMMAND:
      g_clear_pointer (&self->command, g_free);
      self->command = g_value_dup_string (value);
      break;
    case PROP_ICON:
      g_clear_object (&self->icon);
      self->icon = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
kgx_close_dialog_row_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  KgxCloseDialogRow *self = KGX_CLOSE_DIALOG_ROW (object);

  switch (property_id) {
    case PROP_COMMAND:
      g_value_set_string (value, self->command);
      break;
    case PROP_ICON:
      g_value_set_object (value, self->icon);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
kgx_close_dialog_row_finalize (GObject *object)
{
  KgxCloseDialogRow *self = KGX_CLOSE_DIALOG_ROW (object);

  g_clear_pointer (&self->command, g_free);
  g_clear_object (&self->icon);

  G_OBJECT_CLASS (kgx_close_dialog_row_parent_class)->finalize (object);
}

static void
kgx_close_dialog_row_class_init (KgxCloseDialogRowClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->set_property = kgx_close_dialog_row_set_property;
  object_class->get_property = kgx_close_dialog_row_get_property;
  object_class->finalize     = kgx_close_dialog_row_finalize;

  /**
   * KgxCloseDialogRow:command:
   * 
   * The command this row is supposed to represent, see kgx_process_get_exec()
   * 
   * Stability: Private
   * 
   * Since: 0.2.0
   */
  pspecs[PROP_COMMAND] =
    g_param_spec_string ("command", "Command", "Command row represents",
                         NULL,
                         G_PARAM_READWRITE);

  /**
   * KgxCloseDialogRow:icon:
   * 
   * A #GIcon to represent the command, most of the time this will just be
   * a placeholder but occasionally we might get lucky
   * 
   * Stability: Private
   * 
   * Since: 0.2.0
   */
  pspecs[PROP_ICON] =
    g_param_spec_object ("icon", "Icon", "Command app icon",
                         G_TYPE_ICON,
                         G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               RES_PATH "kgx-close-dialog-row.ui");
}

static void
kgx_close_dialog_row_init (KgxCloseDialogRow *self)
{
  self->icon = g_themed_icon_new ("application-x-executable-symbolic");

  gtk_widget_init_template (GTK_WIDGET (self));
}
