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

/**
 * SECTION:kgx-application
 * @title: KgxApplication
 * @short_description: Application
 * 
 * The application, on the face of it nothing particularly interesting but
 * under the hood it contains a #GSource used to monitor the shells (and
 * there children) running in the open #KgxWindow s
 * 
 * Sooner or later this will implement the GNOME Terminal dbus interface
 */

#include <glib/gi18n.h>
#include <vte/vte.h>

#include "rgba.h"
#include "kgx-dbus.h"

#include "kgx-config.h"
#include "kgx-application.h"
#include "kgx-search-box.h"
#include "kgx-window.h"

G_DEFINE_TYPE (KgxApplication, kgx_application, GTK_TYPE_APPLICATION)

enum {
  PROP_0,
  PROP_THEME,
  PROP_FONT,
  LAST_PROP
};

static GParamSpec *pspecs[LAST_PROP] = { NULL, };

void
kgx_application_set_theme (KgxApplication *self,
                           KgxTheme        theme)
{
  self->theme = theme;

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
    case PROP_FONT:
      g_value_set_boxed (value, kgx_application_get_system_font (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
kgx_application_finalize (GObject *object)
{
  KgxApplication *self = KGX_APPLICATION (object);

  g_clear_object (&self->desktop_interface);

  g_ptr_array_unref (self->watching);
  g_ptr_array_unref (self->children);

  G_OBJECT_CLASS (kgx_application_parent_class)->finalize (object);
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

#if HAS_GTOP
static gboolean
watch_is_for_process (struct ProcessWatch *watch,
                      KgxProcess          *process)
{
  return kgx_process_get_pid (watch->process) == kgx_process_get_pid (process);
}

static gboolean
process_is_watched_by (KgxProcess *process,
                       struct ProcessWatch *watch)
{
  return kgx_process_get_pid (watch->process) == kgx_process_get_pid (process);
}

static gboolean
watch (gpointer data)
{
  KgxApplication *self = KGX_APPLICATION (data);
  g_autoptr (GPtrArray) plist = NULL;
  const char *exec;

  plist = kgx_process_get_list ();

  for (int i = 0; i < self->watching->len; i++) {
    struct ProcessWatch *watch = g_ptr_array_index (self->watching, i);

    for (int i = 0; i < plist->len; i++) {
      g_autoptr (KgxProcess) parent = NULL;
      KgxProcess *curr = g_ptr_array_index (plist, i);
      GPid pid;

      parent = kgx_process_get_parent (curr);
      pid = kgx_process_get_pid (curr);

      if (kgx_process_get_pid (parent) == kgx_process_get_pid (watch->process)) {
        exec = kgx_process_get_exec (curr);
        g_message ("Found child pid %i (of %i) @%i: %s",
                  pid,
                  kgx_process_get_pid (watch->process),
                  i,
                  exec);
        
        if (!g_ptr_array_find_with_equal_func (self->children, curr, (GEqualFunc) watch_is_for_process, NULL)) {
          struct ProcessWatch *child_watch = g_new(struct ProcessWatch, 1);

          child_watch->process = g_rc_box_acquire (curr);
          child_watch->window = watch->window;

          g_message ("New child, watch it!");

          g_ptr_array_add (self->children, child_watch);
        }

        if (g_strcmp0 (exec, "ssh") == 0) {
          kgx_window_push_remote (watch->window);
        }

        if (kgx_process_get_is_root (curr)) {
          kgx_window_push_root (watch->window);
        }
      }
    }
  }

  for (int i = 0; i < self->children->len; i++) {
    struct ProcessWatch *child_watch = g_ptr_array_index (self->children, i);

    g_message ("%i of %i", i, self->children->len);

    if (!g_ptr_array_find_with_equal_func (plist, child_watch, (GEqualFunc) process_is_watched_by, NULL)) {
      g_message ("Oh bye then %s", kgx_process_get_exec (child_watch->process));
      kgx_window_pop_remote (child_watch->window);
      kgx_window_pop_root (child_watch->window);
      g_ptr_array_remove_index (self->children, i);
      i--;
    }
  }

  return G_SOURCE_CONTINUE;
}
#endif

static void
kgx_application_startup (GApplication *app)
{
  GtkSettings    *gtk_settings;
  GSettings      *settings;
  GtkCssProvider *provider;
  #if HAS_GTOP
  guint           source;
  #endif
  const char *const new_window_accels[] = { "<shift><primary>n", NULL };

  g_type_ensure (KGX_TYPE_TERMINAL);
  g_type_ensure (KGX_TYPE_SEARCH_BOX);

  G_APPLICATION_CLASS (kgx_application_parent_class)->startup (app);

  gtk_settings = gtk_settings_get_default ();

  g_object_set (G_OBJECT (gtk_settings),
                "gtk-application-prefer-dark-theme", TRUE,
                NULL);

  gtk_application_set_accels_for_action (GTK_APPLICATION (app),
                                         "win.new-window", new_window_accels);

  settings = g_settings_new ("org.gnome.zbrown.KingsCross");
  g_settings_bind (settings, "theme", app, "theme", G_SETTINGS_BIND_DEFAULT);

  provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_resource (provider, "/org/gnome/zbrown/KingsCross/styles.css");
  gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                             GTK_STYLE_PROVIDER (provider),
                                             /* Is this stupid? Yes
                                              * Does it fix vte using the wrong
                                              * priority for fallback styles? Yes*/
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 1);

  #if HAS_GTOP
  source = g_timeout_add (500, watch, app);
  g_source_set_name_by_id (source, _("child watcher"));
  #endif
}

static gboolean
create_instance (KgxDBusFactory        *interface,
                 GDBusMethodInvocation *invocation,
                 const gchar           *greeting,
                 gpointer               user_data)
{
  kgx_dbus_factory_complete_create_instance (interface, invocation, "/org/gnome/Terminal/Factory0/");

  return TRUE;
}

static gboolean
kgx_application_dbus_register (GApplication    *app,
                               GDBusConnection *connection,
                               const gchar     *object_path,
                               GError         **error)
{
  KgxDBusFactory *interface = kgx_dbus_factory_skeleton_new ();

  g_signal_connect (interface,
                    "handle-create-instance",
                    G_CALLBACK (create_instance),
                    app);

  if (!g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (interface),
                                         connection,
                                         "/org/gnome/Terminal/Factory0",
                                         error))
    {
      /* handle error */
    }

  return TRUE;
}

static void
kgx_application_class_init (KgxApplicationClass *klass)
{
  GObjectClass      *object_class = G_OBJECT_CLASS (klass);
  GApplicationClass *app_class    = G_APPLICATION_CLASS (klass);

  object_class->set_property = kgx_application_set_property;
  object_class->get_property = kgx_application_get_property;
  object_class->finalize = kgx_application_finalize;

  app_class->activate = kgx_application_activate;
  app_class->startup = kgx_application_startup;
  app_class->dbus_register = kgx_application_dbus_register;

  pspecs[PROP_THEME] =
    g_param_spec_enum ("theme", "Theme", "Terminal theme",
                       KGX_TYPE_THEME, KGX_THEME_NIGHT,
                       G_PARAM_READWRITE);

  pspecs[PROP_FONT] =
    g_param_spec_boxed ("font", "Font", "Monospace font",
                         PANGO_TYPE_FONT_DESCRIPTION,
                         G_PARAM_READABLE);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);
}

static void
clear_watch (struct ProcessWatch *watch)
{
  kgx_process_unref (watch->process);

  g_free (watch);
}

static void
font_changed (GSettings      *settings,
              const char     *key,
              KgxApplication *self)
{
  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_FONT]);
}

static void
kgx_application_init (KgxApplication *self)
{
  self->desktop_interface = g_settings_new (DESKTOP_INTERFACE_SETTINGS_SCHEMA);

  g_signal_connect (self->desktop_interface,
                    "changed::" MONOSPACE_FONT_KEY_NAME,
                    G_CALLBACK (font_changed),
                    self);

  self->watching = g_ptr_array_new_with_free_func (g_free);
  self->children = g_ptr_array_new_with_free_func ((GDestroyNotify) clear_watch);
}

/**
 * kgx_application_add_watch:
 * @self: the #KgxApplication
 * @process: the shell process to watch
 * @window: the window the shell is running in
 * 
 * registers a new shell proccess with the pid watcher
 */
void
kgx_application_add_watch (KgxApplication *self,
                           KgxProcess     *process,
                           KgxWindow      *window)
{
  struct ProcessWatch *watch;

  g_return_if_fail (KGX_IS_APPLICATION (self));
  g_return_if_fail (KGX_IS_WINDOW (window));

  watch = g_new0 (struct ProcessWatch, 1);
  watch->process = g_rc_box_acquire (process);
  watch->window = window;

  g_return_if_fail (KGX_IS_WINDOW (watch->window));

  g_ptr_array_add (self->watching, watch);
}

/**
 * kgx_application_get_system_font:
 * @self: the #KgxApplication
 *
 * Creates a #PangoFontDescription for the system monospace font.
 *
 * Returns: (transfer full): a new #PangoFontDescription
 */
PangoFontDescription *
kgx_application_get_system_font (KgxApplication *self)
{
  // Taken from gnome-terminal
  g_autofree char *font = NULL;

  g_return_val_if_fail (KGX_IS_APPLICATION (self), NULL);

  font = g_settings_get_string (self->desktop_interface,
                                MONOSPACE_FONT_KEY_NAME);

  return pango_font_description_from_string (font);
}