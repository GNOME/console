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

  // Workings of GDK_RGBA prevent this being static
  GdkRGBA palette[16] = {
    GDK_RGBA ("241f31"), // Black
    GDK_RGBA ("c01c28"), // Red
    GDK_RGBA ("2ec27e"), // Green
    GDK_RGBA ("f5c211"), // Yellow
    GDK_RGBA ("1c71d8"), // Blue
    GDK_RGBA ("813d9c"), // Magenta
    GDK_RGBA ("0ab9dc"), // Cyan
    GDK_RGBA ("c0bfbc"), // White
    GDK_RGBA ("5e5c64"), // Bright Black
    GDK_RGBA ("ed333b"), // Bright Red
    GDK_RGBA ("57e389"), // Bright Green
    GDK_RGBA ("f8e45c"), // Bright Yellow
    GDK_RGBA ("62a0ea"), // Bright Blue
    GDK_RGBA ("c061cb"), // Bright Magenta
    GDK_RGBA ("4fd2fd"), // Bright Cyan
    GDK_RGBA ("f6f5f4"), // Bright White
  };

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

  vte_terminal_set_colors (VTE_TERMINAL (self->terminal), &fg, &bg, palette, 16);

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
new_activated (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       data)
{
  GtkWindow *window;
  GtkApplication *app;
  guint32 timestamp;

  /* Slightly "wrong" but hopefully by taking the time before
   * we spend non-zero time initing the window it's far enough in the
   * past for shell to do-the-right-thing
   */
  timestamp = GDK_CURRENT_TIME;

  app = gtk_window_get_application (GTK_WINDOW (data));

  window = g_object_new (KGX_TYPE_WINDOW,
                        "application", app,
                        NULL);

  gtk_window_present_with_time (window, timestamp);
}

static void
got_text (GtkClipboard *clipboard,
          const gchar *text,
          gpointer data)
{
  GtkWidget *term = KGX_WINDOW (data)->terminal;

  // TODO: Check for sudo

  // HACK: Technically a race condition here
  vte_terminal_paste_clipboard (VTE_TERMINAL (term));
}

static void
paste_activated (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       data)
{
  GtkClipboard *cb;

  cb = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);

  gtk_clipboard_request_text (cb, got_text, data);
}

static void
select_all_activated (GSimpleAction *action,
                      GVariant      *parameter,
                      gpointer       data)
{
  GtkWidget *term = KGX_WINDOW (data)->terminal;

  vte_terminal_select_all (VTE_TERMINAL (term));
}

static void
show_in_files_activated (GSimpleAction *action,
                         GVariant      *parameter,
                         gpointer       data)
{
  GDBusProxy      *proxy;
  GVariant        *retval;
  GVariantBuilder *builder;
  GError          *error = NULL;
  GtkWidget       *term = KGX_WINDOW (data)->terminal;
  const gchar     *uri = NULL;
  const gchar     *method;

  uri = vte_terminal_get_current_file_uri (VTE_TERMINAL (term));
  method = "ShowItems";

  if (uri == NULL) {
    uri = vte_terminal_get_current_directory_uri (VTE_TERMINAL (term));
    method = "ShowFolders";
  }

  if (uri == NULL) {
    g_warning (_("win.show-in-files: no file"));
    return;
  }

  proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                         G_DBUS_PROXY_FLAGS_NONE,
                                         NULL,
                                         "org.freedesktop.FileManager1",
                                         "/org/freedesktop/FileManager1",
                                         "org.freedesktop.FileManager1",
                                         NULL, &error);

  if (!proxy) {
    g_warning (_("win.show-in-files: dbus connect failed %s"), error->message);
    return;
  }

  builder = g_variant_builder_new (G_VARIANT_TYPE ("as"));
  g_variant_builder_add (builder, "s", uri);

  retval = g_dbus_proxy_call_sync (proxy,
                                   method,
                                   g_variant_new ("(ass)",
                                                  builder,
                                                  ""),
                                   G_DBUS_CALL_FLAGS_NONE,
                                   -1, NULL, &error);

  g_variant_builder_unref (builder);
  g_object_unref (proxy);

  if (!retval) {
    g_warning (_("win.show-in-files: dbus call failed %s"), error->message);
    return;
  }

  g_variant_unref (retval);
}

static void
about_activated (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       data)
{
  const gchar *authors[] = { "Zander Brown", NULL };

  gtk_show_about_dialog (GTK_WINDOW (data),
                         "authors", authors,
                         "comments", _("Terminal Emulator"),
                         "copyright", g_strdup_printf (_("Copyright © %Id Zander Brown"), 2019),
                         "license-type", GTK_LICENSE_GPL_3_0,
                         "logo-icon-name", "org.gnome.zbrown.KingsCross",
                         "proogram-name", _("King's Cross"),
                         "version", PACKAGE_VERSION,
                         NULL);
}

static GActionEntry win_entries[] =
{
  { "new-window", new_activated, NULL, NULL, NULL },
  { "open-link", new_activated, NULL, NULL, NULL },
  { "copy-link", new_activated, NULL, NULL, NULL },
  { "copy", new_activated, NULL, NULL, NULL },
  { "paste", paste_activated, NULL, NULL, NULL },
  { "select-all", select_all_activated, NULL, NULL, NULL },
  { "show-in-files", show_in_files_activated, NULL, NULL, NULL },
  { "about", about_activated, NULL, NULL, NULL },
};

static void
context_menu (KgxWindow *self, GtkWidget *term, GdkEventButton *event)
{
  GtkWidget *menu;
  GtkApplication *app;
  GMenu *model;
  GdkRectangle rect;

  app = gtk_window_get_application (GTK_WINDOW (self));
  model = gtk_application_get_menu_by_id (app, "context-menu");

  rect = (GdkRectangle) { event->x, event->y, 1, 1 };

  menu = gtk_popover_new_from_model (term, G_MENU_MODEL (model));
  gtk_popover_set_pointing_to (GTK_POPOVER (menu), &rect);
  gtk_popover_popup (GTK_POPOVER (menu));
}

static gboolean
button_press_event (GtkWidget *widget, GdkEventButton *event, KgxWindow *self)
{
  if (gdk_event_triggers_context_menu ((GdkEvent *) event) &&
      event->type == GDK_BUTTON_PRESS)
    {
      context_menu (self, widget, event);
      return TRUE;
    }

  return FALSE;
}

static gboolean
popup_menu (GtkWidget *widget, KgxWindow *self)
{
  context_menu (self, widget, NULL);
  return TRUE;
}

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

  g_signal_connect (self->terminal, "button-press-event", G_CALLBACK (button_press_event), self);
  g_signal_connect (self->terminal, "popup-menu", G_CALLBACK (popup_menu), self);

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
