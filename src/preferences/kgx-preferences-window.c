/* kgx-preferences-window.c
 *
 * Copyright 2023 Maximiliano Sandoval
 * Copyright 2023 Zander Brown
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

#include "kgx-font-picker.h"
#include "kgx-settings.h"
#include "kgx-utils.h"

#include "kgx-preferences-window.h"


struct _KgxPreferencesWindow {
  AdwPreferencesDialog  parent_instance;

  KgxSettings          *settings;
  GBindingGroup        *settings_binds;

  GtkWidget            *audible_bell;
  GtkWidget            *visual_bell;
  GtkWidget            *use_system_font;
  GtkWidget            *custom_font;
  GtkWidget            *unlimited_scrollback;
  GtkWidget            *scrollback;
};


G_DEFINE_TYPE (KgxPreferencesWindow, kgx_preferences_window, ADW_TYPE_PREFERENCES_DIALOG)


enum {
  PROP_0,
  PROP_SETTINGS,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };


static void
kgx_preferences_window_dispose (GObject *object)
{
  KgxPreferencesWindow *self = KGX_PREFERENCES_WINDOW (object);

  g_clear_object (&self->settings);

  G_OBJECT_CLASS (kgx_preferences_window_parent_class)->dispose (object);
}


static void
kgx_preferences_window_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  KgxPreferencesWindow *self = KGX_PREFERENCES_WINDOW (object);

  switch (property_id) {
    case PROP_SETTINGS:
      if (g_set_object (&self->settings, g_value_get_object (value))) {
        g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_SETTINGS]);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
kgx_preferences_window_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  KgxPreferencesWindow *self = KGX_PREFERENCES_WINDOW (object);

  switch (property_id) {
    case PROP_SETTINGS:
      g_value_set_object (value, self->settings);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static PangoAttrList *
font_as_attributes (GObject *object, PangoFontDescription *font)
{
  g_autoptr (PangoAttrList) list = pango_attr_list_new ();

  if (font) {
    pango_attr_list_insert (list, pango_attr_font_desc_new (font));
  }

  return g_steal_pointer (&list);
}


static char *
font_as_label (GObject              *object,
               PangoFontDescription *font)
{
  if (!font) {
    return g_strdup (_("No Font Set"));
  }

  return pango_font_description_to_string (font);
}


static void
font_selected (KgxFontPicker        *picker,
               PangoFontDescription *font,
               KgxPreferencesWindow *self)
{
  kgx_settings_set_custom_font (self->settings, font);

  adw_preferences_dialog_pop_subpage (ADW_PREFERENCES_DIALOG (self));
}


static void
select_font_activated (GtkWidget  *widget,
                       const char *action_name,
                       GVariant   *parameter)
{
  KgxPreferencesWindow *self = KGX_PREFERENCES_WINDOW (widget);
  g_autoptr (PangoFontDescription) initial_value = NULL;
  AdwNavigationPage *picker;

  initial_value = kgx_settings_dup_custom_font (self->settings);

  picker = g_object_connect (g_object_new (KGX_TYPE_FONT_PICKER,
                                           "initial-font", initial_value,
                                           NULL),
                             "object-signal::selected", G_CALLBACK (font_selected), self,
                             NULL);

  adw_preferences_dialog_push_subpage (ADW_PREFERENCES_DIALOG (self),
                                       picker);
}


static void
kgx_preferences_window_class_init (KgxPreferencesWindowClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = kgx_preferences_window_dispose;
  object_class->set_property = kgx_preferences_window_set_property;
  object_class->get_property = kgx_preferences_window_get_property;

  pspecs[PROP_SETTINGS] =
    g_param_spec_object ("settings", NULL, NULL,
                         KGX_TYPE_SETTINGS,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               KGX_APPLICATION_PATH "preferences/kgx-preferences-window.ui");

  gtk_widget_class_bind_template_child (widget_class, KgxPreferencesWindow, settings_binds);
  gtk_widget_class_bind_template_child (widget_class, KgxPreferencesWindow, audible_bell);
  gtk_widget_class_bind_template_child (widget_class, KgxPreferencesWindow, visual_bell);
  gtk_widget_class_bind_template_child (widget_class, KgxPreferencesWindow, use_system_font);
  gtk_widget_class_bind_template_child (widget_class, KgxPreferencesWindow, custom_font);
  gtk_widget_class_bind_template_child (widget_class, KgxPreferencesWindow, unlimited_scrollback);
  gtk_widget_class_bind_template_child (widget_class, KgxPreferencesWindow, scrollback);

  gtk_widget_class_bind_template_callback (widget_class, font_as_attributes);
  gtk_widget_class_bind_template_callback (widget_class, font_as_label);

  gtk_widget_class_install_action (widget_class,
                                   "prefs.select-font",
                                   NULL,
                                   select_font_activated);
}


struct _WatchData {
  KgxPreferencesWindow *window;
  GtkExpressionWatch   *watch;
};


KGX_DEFINE_DATA (WatchData, watch_data)


static inline void
watch_data_cleanup (WatchData *self)
{
  g_clear_weak_pointer (&self->window);
}


static void
notify_use_system (gpointer user_data)
{
  WatchData *data = user_data;
  g_auto (GValue) value = G_VALUE_INIT;

  if (G_UNLIKELY (!data->window)) {
    return;
  }

  if (G_LIKELY (gtk_expression_watch_evaluate (data->watch, &value))) {
    gtk_widget_action_set_enabled (GTK_WIDGET (data->window),
                                   "prefs.select-font",
                                   !g_value_get_boolean (&value));
  } else {
    gtk_widget_action_set_enabled (GTK_WIDGET (data->window),
                                   "prefs.select-font",
                                   FALSE);
  }
}


static void
kgx_preferences_window_init (KgxPreferencesWindow *self)
{
  g_autoptr (GtkExpression) expression = NULL;
  g_autoptr (WatchData) data = watch_data_alloc ();

  gtk_widget_init_template (GTK_WIDGET (self));

  g_set_weak_pointer (&data->window, self);

  expression =
    gtk_property_expression_new (KGX_TYPE_SETTINGS,
                                 gtk_property_expression_new (G_TYPE_BINDING_GROUP,
                                                              gtk_object_expression_new (G_OBJECT (self->settings_binds)),
                                                              "source"),
                                 "use-system-font");

  data->watch =
    gtk_expression_watch (expression, self, notify_use_system, data, watch_data_free);
  g_steal_pointer (&data); /* this is actually stolen by the watch */

  g_binding_group_bind (self->settings_binds, "audible-bell",
                        self->audible_bell, "active",
                        G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  g_binding_group_bind (self->settings_binds, "visual-bell",
                        self->visual_bell, "active",
                        G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  g_binding_group_bind (self->settings_binds, "use-system-font",
                        self->use_system_font, "active",
                        G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);

  g_binding_group_bind (self->settings_binds, "ignore-scrollback-limit",
                        self->unlimited_scrollback, "active",
                        G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  g_binding_group_bind (self->settings_binds, "scrollback-limit",
                        self->scrollback, "value",
                        G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);

}
