/* kgx-preferences-window.c
 *
 * Copyright 2023 Maximiliano Sandoval
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

#include "kgx-settings.h"
#include "kgx-preferences-window.h"


struct _KgxPreferencesWindow {
  AdwPreferencesWindow  parent_instance;

  KgxSettings          *settings;
  GBindingGroup        *settings_binds;

  GtkWidget            *audible_bell;
};


G_DEFINE_TYPE (KgxPreferencesWindow, kgx_preferences_window, ADW_TYPE_PREFERENCES_WINDOW)


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
}


static void
kgx_preferences_window_init (KgxPreferencesWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  g_binding_group_bind (self->settings_binds, "audible-bell",
                        self->audible_bell, "active",
                        G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
}

