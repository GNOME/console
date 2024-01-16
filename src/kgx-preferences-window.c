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

#include "kgx-settings.h"
#include "kgx-font-picker.h"
#include "kgx-preferences-window.h"


struct _KgxPreferencesWindow {
  AdwPreferencesDialog  parent_instance;

  KgxSettings          *settings;
  GBindingGroup        *settings_binds;

  GtkWidget            *audible_bell;
  GtkWidget            *visual_bell;
  GtkWidget            *use_system_font;
  GtkWidget            *custom_font;
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

  gtk_window_destroy (GTK_WINDOW (picker));
}


static void
select_font_activated (GtkWidget  *widget,
                       const char *action_name,
                       GVariant   *parameter)
{
  KgxPreferencesWindow *self = KGX_PREFERENCES_WINDOW (widget);
  GtkWindow *root = GTK_WINDOW (gtk_widget_get_root (widget));
  g_autoptr (PangoFontDescription) initial_value = NULL;

  initial_value = kgx_settings_get_custom_font (self->settings);

  gtk_window_present (g_object_connect (g_object_new (KGX_TYPE_FONT_PICKER,
                                                      "transient-for", root,
                                                      "initial-font", initial_value,
                                                      NULL),
                                        "object-signal::selected", G_CALLBACK (font_selected), self,
                                        NULL));
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
                                               KGX_APPLICATION_PATH "kgx-preferences-window.ui");

  gtk_widget_class_bind_template_child (widget_class, KgxPreferencesWindow, settings_binds);
  gtk_widget_class_bind_template_child (widget_class, KgxPreferencesWindow, audible_bell);
  gtk_widget_class_bind_template_child (widget_class, KgxPreferencesWindow, visual_bell);
  gtk_widget_class_bind_template_child (widget_class, KgxPreferencesWindow, use_system_font);
  gtk_widget_class_bind_template_child (widget_class, KgxPreferencesWindow, custom_font);

  gtk_widget_class_bind_template_callback (widget_class, font_as_attributes);
  gtk_widget_class_bind_template_callback (widget_class, font_as_label);

  gtk_widget_class_install_action (widget_class,
                                   "prefs.select-font",
                                   NULL,
                                   select_font_activated);
}


static void
kgx_preferences_window_init (KgxPreferencesWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  g_binding_group_bind (self->settings_binds, "audible-bell",
                        self->audible_bell, "active",
                        G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  g_binding_group_bind (self->settings_binds, "visual-bell",
                        self->visual_bell, "active",
                        G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  g_binding_group_bind (self->settings_binds, "use-system-font",
                        self->use_system_font, "active",
                        G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
}
