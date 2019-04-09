/* kgx-window.c
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

#include <glib/gi18n.h>
#include <vte/vte.h>

#include "rgba.h"

#include "kgx.h"
#include "kgx-config.h"
#include "kgx-application.h"
#include "kgx-window.h"
#include "kgx-enums.h"

struct _KgxApplication
{
  GtkApplication parent_instance;

  KgxTheme       theme;
};

G_DEFINE_TYPE (KgxApplication, kgx_application, GTK_TYPE_APPLICATION)

enum {
	PROP_0,
	PROP_THEME,
	LAST_PROP
};

static GParamSpec *pspecs[LAST_PROP] = { NULL, };

void
kgx_application_set_theme (KgxApplication *self,
                           KgxTheme        theme)
{
  GtkSettings *settings;

  self->theme = theme;

  settings = gtk_settings_get_default ();

  switch (theme) {
    case KGX_THEME_NIGHT:
    case KGX_THEME_HACKER:
      g_object_set (G_OBJECT (settings),
                    "gtk-application-prefer-dark-theme", TRUE,
                    NULL);
      break;
    case KGX_THEME_DAY:
    default:
      g_object_set (G_OBJECT (settings),
                    "gtk-application-prefer-dark-theme", FALSE,
                    NULL);
      break;
  }

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_THEME]);
}

static void
kgx_application_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  KgxApplication *self = KGX_APPLICATION (object);

  switch (property_id) {
    case PROP_THEME:
      kgx_application_set_theme (self, g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
kgx_application_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  KgxApplication *self = KGX_APPLICATION (object);

  switch (property_id) {
    case PROP_THEME:
      g_value_set_enum (value, self->theme);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
kgx_application_activate (GApplication *app)
{
  GtkWindow *window;
  guint32 timestamp;

  timestamp = GDK_CURRENT_TIME;

  /* Get the current window or create one if necessary. */
  window = gtk_application_get_active_window (GTK_APPLICATION (app));
  if (window == NULL) {
    window = g_object_new (KGX_TYPE_WINDOW,
                           "application", app,
                           NULL);
  }

  gtk_window_present_with_time (window, timestamp);
}

static void
kgx_application_startup (GApplication *app)
{
  GSettings      *settings;
  GtkCssProvider *provider;
  const char *const new_window_accels[] = { "<shift><primary>n", NULL };

  g_type_ensure (VTE_TYPE_TERMINAL);

  G_APPLICATION_CLASS (kgx_application_parent_class)->startup (app);

  gtk_application_set_accels_for_action (GTK_APPLICATION (app),
                                         "win.new-window", new_window_accels);

  settings = g_settings_new ("org.gnome.zbrown.KingsCross");
  g_settings_bind (settings, "theme", app, "theme", G_SETTINGS_BIND_DEFAULT);

  provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_resource (provider, "/org/gnome/zbrown/KingsCross/styles.css");
  gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                             GTK_STYLE_PROVIDER (provider),
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

static void
kgx_application_class_init (KgxApplicationClass *klass)
{
  GObjectClass      *object_class = G_OBJECT_CLASS (klass);
  GApplicationClass *app_class    = G_APPLICATION_CLASS (klass);

  object_class->set_property = kgx_application_set_property;
  object_class->get_property = kgx_application_get_property;

  app_class->activate = kgx_application_activate;
  app_class->startup = kgx_application_startup;

  pspecs[PROP_THEME] =
    g_param_spec_enum ("theme", "Theme", "Terminal theme",
                       KGX_TYPE_THEME, KGX_THEME_NIGHT,
                       G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);
}

static void
kgx_application_init (KgxApplication *self)
{
}
