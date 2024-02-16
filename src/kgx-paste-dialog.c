/* kgx-paste-dialog.c
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

#include "kgx-config.h"

#include <glib/gi18n.h>

#include <adwaita.h>

#include "kgx-utils.h"
#include "kgx-paste-dialog.h"


struct _KgxPasteDialog {
  GObject           parent;

  char             *content;
  GObject          *transient_for;

  AdwMessageDialog *dialog;
};


G_DEFINE_FINAL_TYPE (KgxPasteDialog, kgx_paste_dialog, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_CONTENT,
  PROP_TRANSIENT_FOR,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };


static void
lost_parent (gpointer data, GObject *dead)
{
  KgxPasteDialog *self = data;

  self->transient_for = NULL;
  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_TRANSIENT_FOR]);
}


static void
kgx_paste_dialog_dispose (GObject *object)
{
  KgxPasteDialog *self = KGX_PASTE_DIALOG (object);

  g_clear_pointer (&self->content, g_free);

  if (self->dialog) {
    gtk_window_destroy (GTK_WINDOW (self->dialog));
  }
  g_clear_weak_pointer (&self->dialog);

  if (self->transient_for) {
    g_object_weak_unref (self->transient_for, lost_parent, self);
    self->transient_for = NULL;
  }

  G_OBJECT_CLASS (kgx_paste_dialog_parent_class)->dispose (object);
}


static void
update_dialog (KgxPasteDialog *self)
{
  g_autofree char *content = kgx_str_constrained_dup (self->content, 1500);

  adw_message_dialog_format_body (ADW_MESSAGE_DIALOG (self->dialog),
                                  /* Translators: %s is the (possibly truncated) command being pasted */
                                  _("Make sure you know what the command does:\n%s"),
                                  content);
}


static void
kgx_paste_dialog_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  KgxPasteDialog *self = KGX_PASTE_DIALOG (object);

  switch (property_id) {
    case PROP_CONTENT:
      g_clear_pointer (&self->content, g_free);
      self->content = g_value_dup_string (value);
      update_dialog (self);
      break;
    case PROP_TRANSIENT_FOR:
      if (self->transient_for) {
        g_object_weak_unref (self->transient_for, lost_parent, self);
      }
      self->transient_for = g_value_get_object (value);
      if (G_LIKELY (self->transient_for)) {
        g_object_weak_ref (self->transient_for, lost_parent, self);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
kgx_paste_dialog_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  KgxPasteDialog *self = KGX_PASTE_DIALOG (object);

  switch (property_id) {
    case PROP_CONTENT:
      g_value_set_string (value, self->content);
      break;
    case PROP_TRANSIENT_FOR:
      g_value_set_object (value, self->transient_for);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
kgx_paste_dialog_class_init (KgxPasteDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS   (klass);

  object_class->dispose = kgx_paste_dialog_dispose;
  object_class->set_property = kgx_paste_dialog_set_property;
  object_class->get_property = kgx_paste_dialog_get_property;

  pspecs[PROP_CONTENT] =
    g_param_spec_string ("content", NULL, NULL,
                         "",
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_TRANSIENT_FOR] =
    g_param_spec_object ("transient-for", NULL, NULL,
                         GTK_TYPE_WINDOW,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);
}


static void
kgx_paste_dialog_init (KgxPasteDialog *self)
{
  AdwMessageDialog *dialog;

  dialog = g_object_new (ADW_TYPE_MESSAGE_DIALOG,
                         "heading", _("You are pasting a command that runs as an administrator"),
                         "close-response", "cancel",
                         NULL);
  g_set_weak_pointer (&self->dialog, dialog);
  g_object_bind_property (self, "transient-for",
                          self->dialog, "transient-for",
                          G_BINDING_DEFAULT);

  adw_message_dialog_add_responses (ADW_MESSAGE_DIALOG (self->dialog),
                                    "cancel", _("_Cancel"),
                                    "paste", _("_Paste"),
                                    NULL);
  adw_message_dialog_set_response_appearance (ADW_MESSAGE_DIALOG (self->dialog),
                                              "paste",
                                              ADW_RESPONSE_DESTRUCTIVE);

  gtk_widget_add_css_class (GTK_WIDGET (self->dialog), "numeric");
}


static void
got_choice (GObject *source, GAsyncResult *res, gpointer user_data)
{
  g_autoptr (GTask) task = user_data;
  const char *choice;

  choice = adw_message_dialog_choose_finish (ADW_MESSAGE_DIALOG (source), res);

  if (!g_strcmp0 (choice, "paste")) {
    g_task_return_int (task, KGX_PASTE_ANYWAY);
  } else {
    g_task_return_int (task, KGX_PASTE_CANCELLED);
  }
}


void
kgx_paste_dialog_run (KgxPasteDialog      *restrict self,
                      GCancellable        *restrict cancellable,
                      GAsyncReadyCallback           callback,
                      gpointer                      user_data)
{
  g_autoptr (GTask) task = g_task_new (self, cancellable, callback, user_data);

  adw_message_dialog_choose (self->dialog,
                             cancellable,
                             got_choice,
                             g_steal_pointer (&task));
}


KgxPasteDialogResult
kgx_paste_dialog_run_finish (KgxPasteDialog            *restrict       self,
                             GAsyncResult              *restrict       res,
                             const char     *restrict  *restrict const content,
                             GError                   **restrict       error)
{
  KgxPasteDialogResult result;

  g_return_val_if_fail (KGX_IS_PASTE_DIALOG (self), KGX_PASTE_CANCELLED);
  g_return_val_if_fail (g_task_is_valid (res, self), KGX_PASTE_CANCELLED);

  result = g_task_propagate_int (G_TASK (res), error);

  if (G_UNLIKELY (error && *error)) {
    *content = NULL;
  } else {
    *content = self->content;
  }

  return result;
}
