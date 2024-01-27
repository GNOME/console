/* kgx-close-dialog.c
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

#include "kgx-config.h"

#include <glib/gi18n.h>

#include <adwaita.h>

#include "kgx-process.h"
#include "kgx-close-dialog.h"


struct _KgxCloseDialog {
  GObject                parent;

  KgxCloseDialogContext  context;
  GPtrArray             *commands;

  AdwAlertDialog        *dialog;
  GtkWidget             *list;
};


G_DEFINE_FINAL_TYPE (KgxCloseDialog, kgx_close_dialog, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_CONTEXT,
  PROP_COMMANDS,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };


static void
kgx_close_dialog_dispose (GObject *object)
{
  KgxCloseDialog *self = KGX_CLOSE_DIALOG (object);

  g_clear_pointer (&self->commands, g_ptr_array_unref);
  g_clear_weak_pointer (&self->dialog);

  G_OBJECT_CLASS (kgx_close_dialog_parent_class)->dispose (object);
}


static void
update_dialog (KgxCloseDialog *self)
{
  size_t n_commands = G_LIKELY (self->commands) ? self->commands->len : 0;

  gtk_list_box_remove_all (GTK_LIST_BOX (self->list));

  switch (self->context) {
    case KGX_CONTEXT_WINDOW:
      g_object_set (self->dialog,
                    "heading", _("Close Window?"),
                    "body", g_dngettext (GETTEXT_PACKAGE,
                                         "A command is still running, closing this window will kill it and may lead to unexpected outcomes",
                                         "Some commands are still running, closing this window will kill them and may lead to unexpected outcomes",
                                         n_commands),
                    NULL);
      break;
    case KGX_CONTEXT_TAB:
      g_object_set (self->dialog,
                    "heading", _("Close Tab?"),
                    "body", g_dngettext (GETTEXT_PACKAGE,
                                         "A command is still running, closing this tab will kill it and may lead to unexpected outcomes",
                                         "Some commands are still running, closing this tab will kill them and may lead to unexpected outcomes",
                                         n_commands),
                    NULL);
      break;
    default:
      g_return_if_reached ();
  }

  for (size_t i = 0; i < n_commands; i++) {
    g_autofree char *title = NULL;
    g_autofree char *subtitle = NULL;

    kgx_process_get_title (g_ptr_array_index (self->commands, i),
                           &title,
                           &subtitle);

    gtk_list_box_append (GTK_LIST_BOX (self->list),
                         g_object_new (ADW_TYPE_ACTION_ROW,
                                       "title", title,
                                       "subtitle", subtitle,
                                       NULL));
  }
}


static void
kgx_close_dialog_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  KgxCloseDialog *self = KGX_CLOSE_DIALOG (object);

  switch (property_id) {
    case PROP_CONTEXT:
      self->context = g_value_get_enum (value);
      update_dialog (self);
      break;
    case PROP_COMMANDS:
      g_clear_pointer (&self->commands, g_ptr_array_unref);
      self->commands = g_value_dup_boxed (value);
      update_dialog (self);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
kgx_close_dialog_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  KgxCloseDialog *self = KGX_CLOSE_DIALOG (object);

  switch (property_id) {
    case PROP_CONTEXT:
      g_value_set_enum (value, self->context);
      break;
    case PROP_COMMANDS:
      g_value_set_boxed (value, self->commands);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
kgx_close_dialog_class_init (KgxCloseDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS   (klass);

  object_class->dispose = kgx_close_dialog_dispose;
  object_class->set_property = kgx_close_dialog_set_property;
  object_class->get_property = kgx_close_dialog_get_property;

  pspecs[PROP_CONTEXT] =
    g_param_spec_enum ("context", NULL, NULL,
                       KGX_TYPE_CLOSE_DIALOG_CONTEXT,
                       KGX_CONTEXT_TAB,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_COMMANDS] =
    g_param_spec_boxed ("commands", NULL, NULL,
                        G_TYPE_PTR_ARRAY,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);
}


static void
kgx_close_dialog_init (KgxCloseDialog *self)
{
  AdwAlertDialog *dialog;

  self->list = g_object_new (GTK_TYPE_LIST_BOX,
                             "can-focus", FALSE,
                             "selection-mode", GTK_SELECTION_NONE,
                             NULL);
  gtk_widget_add_css_class (self->list, "boxed-list");
  gtk_widget_add_css_class (self->list, "process-list");
  gtk_accessible_update_property (GTK_ACCESSIBLE (self->list),
                                  /* Translators: Screen reader label for the list of running commands */
                                  GTK_ACCESSIBLE_PROPERTY_LABEL, _("Process list"),
                                  -1);

  dialog = g_object_new (ADW_TYPE_ALERT_DIALOG,
                         "close-response", "cancel",
                         "extra-child", self->list,
                         NULL);
  g_set_weak_pointer (&self->dialog, dialog);

  adw_alert_dialog_add_responses (self->dialog,
                                  /* Translators: This action dismisses the dialogue, leaving the tab/window open */
                                  "cancel", _("_Cancel"),
                                  /* Translators: This action accepts the ‘unexpected outcomes’ and closes the tab/window */
                                  "close", _("C_lose"),
                                  NULL);
  adw_alert_dialog_set_response_appearance (self->dialog,
                                            "close",
                                            ADW_RESPONSE_DESTRUCTIVE);
}


static void
got_choice (GObject *source, GAsyncResult *res, gpointer user_data)
{
  g_autoptr (GTask) task = user_data;
  const char *choice;

  choice = adw_alert_dialog_choose_finish (ADW_ALERT_DIALOG (source), res);

  if (!g_strcmp0 (choice, "close")) {
    g_task_return_int (task, KGX_CLOSE_ANYWAY);
  } else {
    g_task_return_int (task, KGX_CLOSE_CANCELLED);
  }
}


void
kgx_close_dialog_run (KgxCloseDialog      *restrict self,
                      GtkWidget           *restrict parent,
                      GCancellable        *restrict cancellable,
                      GAsyncReadyCallback           callback,
                      gpointer                      user_data)
{
  g_autoptr (GTask) task = g_task_new (self, cancellable, callback, user_data);

  adw_alert_dialog_choose (self->dialog,
                           parent,
                           cancellable,
                           got_choice,
                           g_steal_pointer (&task));
}


KgxCloseDialogResult
kgx_close_dialog_run_finish (KgxCloseDialog  *restrict self,
                             GAsyncResult    *restrict res,
                             GError         **restrict error)
{
  g_return_val_if_fail (KGX_IS_CLOSE_DIALOG (self), KGX_CLOSE_CANCELLED);
  g_return_val_if_fail (g_task_is_valid (res, self), KGX_CLOSE_CANCELLED);

  return g_task_propagate_int (G_TASK (res), error);
}
