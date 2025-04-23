/* kgx-spad.c
 *
 * Copyright 2025 Zander Brown
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

#include <gtk/gtk.h>
#include <vte/vte.h>

#include "kgx-enums.h"
#include "kgx-about.h"
#include "kgx-utils.h"
#include "kgx-despatcher.h"
#include "kgx-shared-closures.h"

#include "kgx-spad.h"

#define COPY_ICON "edit-copy-symbolic"
#define COPIED_ICON "emblem-ok-symbolic"


struct _KgxSpad {
  AdwDialog     parent_instance;

  KgxSpadFlags  spad_flags;
  char         *error_body;
  char         *error_content;
  const char   *copy_icon_name;

  guint         copy_timeout;
};


G_DEFINE_FINAL_TYPE (KgxSpad, kgx_spad, ADW_TYPE_DIALOG)


enum {
  PROP_0,
  PROP_SPAD_FLAGS,
  PROP_ERROR_BODY,
  PROP_ERROR_CONTENT,
  PROP_COPY_ICON_NAME,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };


static void
kgx_spad_dispose (GObject *object)
{
  KgxSpad *self = KGX_SPAD (object);

  g_clear_pointer (&self->error_body, g_free);
  g_clear_pointer (&self->error_content, g_free);

  g_clear_handle_id (&self->copy_timeout, g_source_remove);

  G_OBJECT_CLASS (kgx_spad_parent_class)->dispose (object);
}


static void
kgx_spad_get_property (GObject    *object,
                       guint       property_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
  KgxSpad *self = KGX_SPAD (object);

  switch (property_id) {
    case PROP_SPAD_FLAGS:
      g_value_set_flags (value, self->spad_flags);
      break;
    case PROP_ERROR_BODY:
      g_value_set_string (value, self->error_body);
      break;
    case PROP_ERROR_CONTENT:
      g_value_set_string (value, self->error_content);
      break;
    case PROP_COPY_ICON_NAME:
      g_value_set_static_string (value, self->copy_icon_name);
      break;
    KGX_INVALID_PROP (object, property_id, pspec);
  }
}


static void
kgx_spad_set_property (GObject      *object,
                       guint         property_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
  KgxSpad *self = KGX_SPAD (object);

  switch (property_id) {
    case PROP_SPAD_FLAGS:
      kgx_set_flags_prop (object, pspec, &self->spad_flags, value);
      break;
    case PROP_ERROR_BODY:
      kgx_set_str_prop (object, pspec, &self->error_body, value);
      break;
    case PROP_ERROR_CONTENT:
      kgx_set_str_prop (object, pspec, &self->error_content, value);
      break;
    KGX_INVALID_PROP (object, property_id, pspec);
  }
}


static char *
add_sys_info (G_GNUC_UNUSED GObject *self,
              gboolean               show_sys_info,
              GtkRoot               *root,
              const char            *text)
{
  g_autoptr (GString) buf = g_string_new (text);

  if (!show_sys_info) {
    return g_string_free_and_steal (g_steal_pointer (&buf));
  }

  if (kgx_str_non_empty (text)) {
    g_string_append (buf, "\n\n——————————\n");
  }

  kgx_about_append_sys_info (buf, root);

  return g_string_free_and_steal (g_steal_pointer (&buf));
}


static void
clear_copy_icon (gpointer user_data)
{
  KgxSpad *self = user_data;

  self->copy_icon_name = COPY_ICON;
  self->copy_timeout = 0;

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_COPY_ICON_NAME]);
}


static void
copy_message (GtkWidget                *widget,
              G_GNUC_UNUSED const char *action_name,
              GVariant                 *target)
{
  KgxSpad *self = KGX_SPAD (widget);
  GdkClipboard *clipboard = gtk_widget_get_clipboard (widget);

  gdk_clipboard_set_text (clipboard, g_variant_get_string (target, NULL));
  self->copy_icon_name = COPIED_ICON;

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_COPY_ICON_NAME]);

  g_clear_handle_id (&self->copy_timeout, g_source_remove);
  self->copy_timeout = g_timeout_add_once (500, clear_copy_icon, self);
}


static void
did_open (GObject *source, GAsyncResult *res, gpointer user_data)
{
  g_autoptr (GtkWidget) parent = user_data;
  g_autoptr (AdwDialog) alert = NULL;
  g_autoptr (GError) error = NULL;

  kgx_despatcher_open_finish (KGX_DESPATCHER (source), res, &error);

  if (!error) {
    return;
  }

  g_warning ("Couldn't open uri: %s\n", error->message);

  alert = adw_alert_dialog_new (_("Unable To Open"), NULL);

  adw_alert_dialog_format_body_markup (ADW_ALERT_DIALOG (alert),
                                       _("Opening ‘<a href=\"%s\">%s</a>’ failed:\n%s"),
                                       KGX_ISSUE_URL,
                                       KGX_ISSUE_URL,
                                       error->message);
  adw_alert_dialog_add_response (ADW_ALERT_DIALOG (alert),
                                 "close",
                                 _("_Close"));

  adw_dialog_present (g_steal_pointer (&alert), parent);
}


static void
open_issues (GtkWidget                *widget,
             G_GNUC_UNUSED const char *action_name,
             G_GNUC_UNUSED GVariant   *target)
{
  g_autoptr (KgxDespatcher) despatcher =
    g_object_new (KGX_TYPE_DESPATCHER, NULL);

  kgx_despatcher_open (despatcher,
                       KGX_ISSUE_URL,
                       GTK_WINDOW (gtk_widget_get_root (widget)),
                       NULL,
                       did_open,
                       g_object_ref (widget));
}


static void
kgx_spad_class_init (KgxSpadClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = kgx_spad_dispose;
  object_class->get_property = kgx_spad_get_property;
  object_class->set_property = kgx_spad_set_property;

  pspecs[PROP_SPAD_FLAGS] =
    g_param_spec_flags ("spad-flags", NULL, NULL,
                        KGX_TYPE_SPAD_FLAGS,
                        KGX_SPAD_NONE,
                        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_ERROR_BODY] =
    g_param_spec_string ("error-body", NULL, NULL,
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_ERROR_CONTENT] =
    g_param_spec_string ("error-content", NULL, NULL,
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_COPY_ICON_NAME] =
    g_param_spec_string ("copy-icon-name", NULL, NULL,
                        COPY_ICON,
                        G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               KGX_APPLICATION_PATH "kgx-spad.ui");

  gtk_widget_class_bind_template_callback (widget_class, add_sys_info);

  gtk_widget_class_bind_template_callback (widget_class, kgx_flags_includes);
  gtk_widget_class_bind_template_callback (widget_class, kgx_text_or_fallback);
  gtk_widget_class_bind_template_callback (widget_class, kgx_text_as_variant);
  gtk_widget_class_bind_template_callback (widget_class, kgx_text_non_empty);

  gtk_widget_class_install_action (widget_class,
                                   "spad.copy-message",
                                   "s",
                                   copy_message);
  gtk_widget_class_install_action (widget_class,
                                   "spad.open-issues",
                                   NULL,
                                   open_issues);
}


static void
kgx_spad_init (KgxSpad *self)
{
  self->copy_icon_name = COPY_ICON;

  gtk_widget_init_template (GTK_WIDGET (self));
}


GVariant *
kgx_spad_build_bundle (const char   *title,
                       KgxSpadFlags  flags,
                       const char   *error_body,
                       const char   *error_content,
                       GError       *error)
{
  g_autoptr (GVariantDict) dict = g_variant_dict_new (NULL);

  g_return_val_if_fail (error_body != NULL, NULL);

  if (title) {
    g_variant_dict_insert (dict, "title", "s", title);
  }

  g_variant_dict_insert (dict, "flags", "u", flags);
  g_variant_dict_insert (dict, "error-body", "s", error_body);

  if (error_content) {
    g_variant_dict_insert (dict, "error-content", "s", error_content);
  }

  if (error) {
    g_variant_dict_insert (dict,
                           "error",
                           "(uis)",
                           error->domain,
                           error->code,
                           error->message);
  }

  return g_variant_ref_sink (g_variant_dict_end (dict));
}


AdwDialog *
kgx_spad_new_from_bundle (GVariant *bundle)
{
  g_auto (GVariantDict) dict = G_VARIANT_DICT_INIT (bundle);
  g_autoptr (GString) str = g_string_new (NULL);
  const char *title = NULL;
  KgxSpadFlags flags;
  const char *error_body = NULL;
  const char *error_content = NULL;
  const char *error_message = NULL;
  GQuark domain;
  int code;

  if (!g_variant_dict_lookup (&dict, "title", "&s", &title) ||
      !kgx_str_non_empty (title)) {
    title = _("Error Details");
  }

  g_variant_dict_lookup (&dict, "flags", "u", &flags);
  g_variant_dict_lookup (&dict, "error-body", "&s", &error_body);

  if (g_variant_dict_lookup (&dict, "error-content", "&s", &error_content)) {
    g_string_append (str, error_content);
  }

  if (g_variant_dict_lookup (&dict, "error", "(ui&s)", &domain, &code, &error_message)) {
    if (kgx_str_non_empty (error_content)) {
      g_string_append_c (str, '\n');
      g_string_append_c (str, '\n');
    }

    g_string_append_printf (str, "%s (%i):\n%s",
                            g_quark_to_string (domain),
                            code,
                            error_message);
  }

  if (flags == KGX_SPAD_NONE &&
      !kgx_str_non_empty (error_content) &&
      !kgx_str_non_empty (error_message)) {
    AdwDialog *alert = g_object_new (ADW_TYPE_ALERT_DIALOG,
                                     "close-response", "close",
                                     "heading", title,
                                     "body-use-markup", TRUE,
                                     "body", error_body,
                                     NULL);

    adw_alert_dialog_add_responses (ADW_ALERT_DIALOG (alert),
                                    "close", _("_Close"),
                                    NULL);

    return alert;
  }

  return g_object_new (KGX_TYPE_SPAD,
                       "title", title,
                       "spad-flags", flags,
                       "error-body", error_body,
                       "error-content", str->str,
                       NULL);
}
