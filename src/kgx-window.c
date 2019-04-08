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

#include <vte/vte.h>

#include "kgx.h"
#include "kgx-config.h"
#include "kgx-window.h"
#include "kgx-enums.h"

struct _KgxWindow
{
  GtkApplicationWindow  parent_instance;

  KgxTheme              theme;

  /* Template widgets */
  GtkWidget            *header_bar;
  GtkWidget            *terminal;
};

G_DEFINE_TYPE (KgxWindow, kgx_window, GTK_TYPE_APPLICATION_WINDOW)

enum {
	PROP_0,
	PROP_THEME,
	LAST_PROP
};

static GParamSpec *pspecs[LAST_PROP] = { NULL, };

void
kgx_window_set_theme (KgxWindow *self,
                      KgxTheme   theme)
{
  GtkSettings *settings;
  GdkRGBA fg;
  GdkRGBA bg;

  self->theme = theme;

  settings = gtk_settings_get_default ();

  switch (theme) {
    case KGX_THEME_NIGHT:
      g_object_set (G_OBJECT (settings),
                    "gtk-application-prefer-dark-theme", TRUE,
                    NULL);
      bg = (GdkRGBA) { 0.1, 0.1, 0.1, 0.96};
      fg = (GdkRGBA) { 1.0, 1.0, 1.0, 1.0};
      break;
    case KGX_THEME_HACKER:
      g_object_set (G_OBJECT (settings),
                    "gtk-application-prefer-dark-theme", TRUE,
                    NULL);
      bg = (GdkRGBA) { 0.1, 0.1, 0.1, 0.96};
      fg = (GdkRGBA) { 0.1, 1.0, 0.1, 1.0};
      break;
    case KGX_THEME_DAY:
    default:
      g_object_set (G_OBJECT (settings),
                    "gtk-application-prefer-dark-theme", FALSE,
                    NULL);
      bg = (GdkRGBA) { 1.0, 1.0, 1.0, 0.96};
      fg = (GdkRGBA) { 0.0, 0.0, 0.0, 1.0};
      break;
  }

  vte_terminal_set_color_foreground (VTE_TERMINAL (self->terminal), &fg);
  vte_terminal_set_color_background (VTE_TERMINAL (self->terminal), &bg);

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_THEME]);
}

static void
kgx_window_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  KgxWindow *self = KGX_WINDOW (object);

  switch (property_id) {
    case PROP_THEME:
      kgx_window_set_theme (self, g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
kgx_window_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  KgxWindow *self = KGX_WINDOW (object);

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
kgx_window_class_init (KgxWindowClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->set_property = kgx_window_set_property;
  object_class->get_property = kgx_window_get_property;

  pspecs[PROP_THEME] =
    g_param_spec_enum ("theme", "Theme", "Terminal theme",
                       KGX_TYPE_THEME, KGX_THEME_HACKER,
                       G_PARAM_READWRITE);

	g_object_class_install_properties (object_class, LAST_PROP, pspecs);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/zbrown/KingsCross/kgx-window.ui");
  gtk_widget_class_bind_template_child (widget_class, KgxWindow, header_bar);
  gtk_widget_class_bind_template_child (widget_class, KgxWindow, terminal);
}

static void
about_activated (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       data)
{
  gtk_show_about_dialog (GTK_WINDOW (data), NULL);
}

static GActionEntry win_entries[] =
{
  { "about", about_activated, NULL, NULL, NULL },
};


static void
kgx_window_init (KgxWindow *self)
{
  GPropertyAction *act;
  GSettings *settings;
  GtkCssProvider *provider;
  gchar *shell[2] = {NULL, NULL};

  gtk_widget_init_template (GTK_WIDGET (self));

  g_action_map_add_action_entries (G_ACTION_MAP (self),
                                   win_entries,
                                   G_N_ELEMENTS (win_entries),
                                   self);

  self->theme = KGX_THEME_HACKER;

  settings = g_settings_new ("org.gnome.zbrown.KingsCross");
  g_settings_bind (settings, "theme", self, "theme", G_SETTINGS_BIND_DEFAULT);

  act = g_property_action_new ("theme", G_OBJECT (self), "theme");
  g_action_map_add_action (G_ACTION_MAP (self), G_ACTION (act));

  provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_resource (provider, "/org/gnome/zbrown/KingsCross/styles.css");
  gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                             GTK_STYLE_PROVIDER (provider),
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);


  shell[0] = vte_get_user_shell ();
  if (shell[0] == NULL) {
    shell[0] = "/bin/sh";
    g_warning ("Defaulting to /bin/sh");
  }

  vte_terminal_spawn_async (VTE_TERMINAL (self->terminal),
                            VTE_PTY_DEFAULT,
                            NULL,
                            shell,
                            NULL,
                            G_SPAWN_DEFAULT,
                            NULL,
                            NULL,
                            NULL,
                            -1,
                            NULL,
                            NULL,
                            NULL);
}
