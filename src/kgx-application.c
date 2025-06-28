/* kgx-application.c
 *
 * Copyright 2019-2024 Zander Brown
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

#include "kgx-about.h"
#include "kgx-depot.h"
#include "kgx-drop-target.h"
#include "kgx-pages.h"
#include "kgx-resources.h"
#include "kgx-settings.h"
#include "kgx-simple-tab.h"
#include "kgx-terminal.h"
#include "kgx-train.h"
#include "kgx-utils.h"
#include "kgx-watcher.h"
#include "kgx-window.h"

#include "kgx-application.h"

#define ARGV_ARGUMENT "extracted-argv"


struct _KgxApplication {
  AdwApplication            parent_instance;

  GTree                    *pages;
  KgxSettings              *settings;
  KgxWatcher               *watcher;
  KgxDepot                 *depot;

  /* this is simply a hashset of pointers, NOT references */
  GHashTable               *active_windows;

  GStrv                     argv;
};


G_DEFINE_TYPE (KgxApplication, kgx_application, ADW_TYPE_APPLICATION)


enum {
  PROP_0,
  PROP_IN_BACKGROUND,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };


static void
kgx_application_dispose (GObject *object)
{
  KgxApplication *self = KGX_APPLICATION (object);

  g_clear_pointer (&self->pages, g_tree_unref);
  g_clear_object (&self->settings);
  g_clear_object (&self->watcher);
  g_clear_object (&self->depot);

  g_clear_pointer (&self->active_windows, g_hash_table_unref);

  g_clear_pointer (&self->argv, g_strfreev);

  G_OBJECT_CLASS (kgx_application_parent_class)->dispose (object);
}


static void
kgx_application_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  KgxApplication *self = KGX_APPLICATION (object);

  switch (property_id) {
    case PROP_IN_BACKGROUND:
      g_value_set_boolean (value,
                           g_hash_table_size (self->active_windows) < 1);
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

  /* Get the current window or create one if necessary. */
  window = gtk_application_get_active_window (GTK_APPLICATION (app));
  if (window == NULL) {
    kgx_application_add_terminal (KGX_APPLICATION (app),
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL);

    return;
  }

  gtk_window_present (window);
}


static gboolean
theme_to_colour_scheme (GBinding     *binding,
                        const GValue *from_value,
                        GValue       *to_value,
                        gpointer      user_data)
{
  switch (g_value_get_enum (from_value)) {
    case KGX_THEME_AUTO:
      g_value_set_enum (to_value, ADW_COLOR_SCHEME_PREFER_LIGHT);
      break;
    case KGX_THEME_DAY:
      g_value_set_enum (to_value, ADW_COLOR_SCHEME_FORCE_LIGHT);
      break;
    case KGX_THEME_NIGHT:
    case KGX_THEME_HACKER:
    default:
      g_value_set_enum (to_value, ADW_COLOR_SCHEME_FORCE_DARK);
      break;
  }

  return TRUE;
}


static void
kgx_application_startup (GApplication *app)
{
  KgxApplication *self = KGX_APPLICATION (app);
  AdwStyleManager *style_manager;

  g_resources_register (kgx_get_resource ());

  G_APPLICATION_CLASS (kgx_application_parent_class)->startup (app);

  style_manager = adw_style_manager_get_default ();
  g_object_bind_property_full (self->settings, "theme",
                               style_manager, "color-scheme",
                               G_BINDING_SYNC_CREATE,
                               theme_to_colour_scheme, NULL, NULL, NULL);

  gtk_application_set_accels_for_action (GTK_APPLICATION (app),
                                         "win.new-window",
                                         (const char *[]) { "<shift><primary>n", "New", NULL });
  gtk_application_set_accels_for_action (GTK_APPLICATION (app),
                                         "win.new-tab",
                                         (const char *[]) { "<shift><primary>t", "<shift>New", NULL });
  gtk_application_set_accels_for_action (GTK_APPLICATION (app),
                                         "win.close-tab",
                                         (const char *[]) { "<shift><primary>w", NULL });
  gtk_application_set_accels_for_action (GTK_APPLICATION (app),
                                         "term.copy",
                                         (const char *[]) { "<shift><primary>c", "Copy", NULL });
  gtk_application_set_accels_for_action (GTK_APPLICATION (app),
                                         "term.paste",
                                         (const char *[]) { "<shift><primary>v", "Paste", NULL });
  gtk_application_set_accels_for_action (GTK_APPLICATION (app),
                                         "win.find",
                                         (const char *[]) { "<shift><primary>f", "Find", NULL });
  gtk_application_set_accels_for_action (GTK_APPLICATION (app),
                                         "app.zoom-in",
                                         (const char *[]) { "<primary>plus", "ZoomIn", NULL });
  gtk_application_set_accels_for_action (GTK_APPLICATION (app),
                                         "app.zoom-out",
                                         (const char *[]) { "<primary>minus", "ZoomOut", NULL });
  gtk_application_set_accels_for_action (GTK_APPLICATION (app),
                                         "app.zoom-normal",
                                         (const char *[]) { "<primary>0", NULL });
  gtk_application_set_accels_for_action (GTK_APPLICATION (app),
                                         "win.show-tabs",
                                         (const char *[]) { "<shift><primary>o", NULL });
  gtk_application_set_accels_for_action (GTK_APPLICATION (app),
                                         "win.show-tabs-desktop",
                                         (const char *[]) { "<shift><primary>o", NULL });
  gtk_application_set_accels_for_action (GTK_APPLICATION (app),
                                         "win.fullscreen",
                                         (const char *[]) { "<shift><primary>F11", NULL });
  gtk_application_set_accels_for_action (GTK_APPLICATION (app),
                                         "win.unfullscreen",
                                         (const char *[]) { "<shift><primary>F11", NULL });
}


static void
kgx_application_open (GApplication  *app,
                      GFile        **files,
                      int            n_files,
                      const char    *hint)
{
  KgxWindow *window = NULL;
  KgxTab *last_tab = NULL;

  for (int i = 0; i < n_files; i++) {
    if (G_UNLIKELY (last_tab && !window)) {
      window = KGX_WINDOW (gtk_widget_get_root (GTK_WIDGET (last_tab)));
    }

    last_tab = kgx_application_add_terminal (KGX_APPLICATION (app),
                                             window,
                                             files[i],
                                             NULL,
                                             NULL);
  }
}


static gboolean
kgx_application_local_command_line (GApplication   *app,
                                    char         ***arguments,
                                    int            *exit_status)
{
  g_autoptr (GError) error = NULL;
  KgxApplication *self = KGX_APPLICATION (app);

  kgx_filter_arguments (arguments, &self->argv, &error);

  if (error) {
    g_printerr ("%s\n", error->message);
    *exit_status = EXIT_FAILURE;
    return TRUE;
  }

  return G_APPLICATION_CLASS (kgx_application_parent_class)->local_command_line (app, arguments, exit_status);
}


static int
kgx_application_command_line (GApplication            *app,
                              GApplicationCommandLine *cli)
{
  KgxApplication *self = KGX_APPLICATION (app);
  GVariantDict *options = NULL;
  g_autofree const char **argv = NULL;
  g_autofree const char **paths = NULL;
  const char *command = NULL;
  const char *working_dir = NULL;
  const char *title = NULL;
  const char *const *shell = NULL;
  gint64 scrollback;
  gboolean tab;
  KgxWindow *window = NULL;

  options = g_application_command_line_get_options_dict (cli);

  g_variant_dict_lookup (options, "working-directory", "^&ay", &working_dir);
  g_variant_dict_lookup (options, "title", "&s", &title);
  g_variant_dict_lookup (options, "command", "^&ay", &command);
  g_variant_dict_lookup (options, ARGV_ARGUMENT, "^a&ay", &argv);
  g_variant_dict_lookup (options, G_OPTION_REMAINING, "^a&ay", &paths);

  if (g_variant_dict_lookup (options, "set-shell", "^as", &shell) && shell) {
    kgx_settings_set_custom_shell (self->settings, shell);

    return EXIT_SUCCESS;
  }

  if (g_variant_dict_lookup (options, "set-scrollback", "x", &scrollback)) {
    kgx_settings_set_scrollback_limit (self->settings, scrollback);

    return EXIT_SUCCESS;
  }

  if (working_dir && paths) {
    g_application_command_line_printerr (cli,
                                         _("Cannot use both --working-directory and positional parameters"));
    return EXIT_FAILURE;
  }

  if (g_variant_dict_lookup (options, "tab", "b", &tab) && tab) {
    window = KGX_WINDOW (gtk_application_get_active_window (GTK_APPLICATION (self)));
  }

  if (paths) {
    KgxTab *last_tab = NULL;

    for (size_t i = 0; paths[i]; i++) {
      g_autoptr (GFile) path =
        g_application_command_line_create_file_for_arg (cli, paths[i]);

      if (G_UNLIKELY (last_tab && !window)) {
        window = KGX_WINDOW (gtk_widget_get_root (GTK_WIDGET (last_tab)));
      }

      last_tab = kgx_application_add_terminal (KGX_APPLICATION (app),
                                              window,
                                              path,
                                              NULL,
                                              NULL);
    }
  } else {
    const char *cwd = NULL;
    g_autoptr (GFile) path = NULL;

    if (working_dir != NULL) {
      path = g_application_command_line_create_file_for_arg (cli, working_dir);
    }

    cwd = g_application_command_line_get_cwd (cli);

    if (path == NULL && cwd) {
      path = g_file_new_for_path (cwd);
    }

    kgx_application_add_terminal (self, window, path, (GStrv) argv, title);
  }

  return EXIT_SUCCESS;
}


static inline void
print_colour (size_t colour, size_t n)
{
  g_print ("\e[0;%02" G_GSIZE_FORMAT "m #%02" G_GSIZE_FORMAT " \e[0m", colour, n);
}


static inline void
print_colour_row (size_t base, bool second_row)
{
  for (size_t i = 0; i < 8; i++) {
    print_colour (base + i, i + (second_row ? 8 : 0));
  }
  g_print ("\n");
}


static int
kgx_application_handle_local_options (GApplication *app,
                                      GVariantDict *options)
{
  KgxApplication *self = KGX_APPLICATION (app);
  gboolean version = FALSE;
  gboolean about = FALSE;
  gboolean colour_table = FALSE;

  if (g_variant_dict_lookup (options, "version", "b", &version) && version) {
    kgx_about_print_version ();

    return EXIT_SUCCESS;
  }

  if (g_variant_dict_lookup (options, "about", "b", &about) && about) {
    kgx_about_print_logo ();

    return EXIT_SUCCESS;
  }

  if (g_variant_dict_lookup (options, "colour-table", "b", &colour_table) &&
      colour_table) {
    g_print ("Colour Table\n");

    print_colour_row (30, false);
    print_colour_row (90, true);
    print_colour_row (40, false);
    print_colour_row (100, true);

    return EXIT_SUCCESS;
  }

  if (self->argv) {
    GVariant *value =
      g_variant_new_bytestring_array ((const char * const*) self->argv, -1);

    g_variant_dict_insert_value (options, ARGV_ARGUMENT, value);

    g_clear_pointer (&self->argv, g_strfreev);
  }

  return G_APPLICATION_CLASS (kgx_application_parent_class)->handle_local_options (app, options);
}


static void
active_changed (GObject *object, GParamSpec *pspec, gpointer user_data)
{
  KgxApplication *self = KGX_APPLICATION (user_data);

  if (gtk_window_is_active (GTK_WINDOW (object))) {
    g_hash_table_add (self->active_windows, object);
  } else {
    g_hash_table_remove (self->active_windows, object);
  }

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_IN_BACKGROUND]);
}


static void
kgx_application_window_added (GtkApplication *app,
                              GtkWindow      *window)
{
  KgxApplication *self = KGX_APPLICATION (app);

  g_signal_connect (window,
                    "notify::is-active", G_CALLBACK (active_changed),
                    self);
  active_changed (G_OBJECT (window), NULL, self);

  GTK_APPLICATION_CLASS (kgx_application_parent_class)->window_added (app,
                                                                      window);
}


static void
kgx_application_window_removed (GtkApplication *app,
                                GtkWindow      *window)
{
  KgxApplication *self = KGX_APPLICATION (app);

  g_signal_handlers_disconnect_by_data (window, self);

  g_hash_table_remove (self->active_windows, window);
  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_IN_BACKGROUND]);

  GTK_APPLICATION_CLASS (kgx_application_parent_class)->window_removed (app,
                                                                        window);
}


static void
kgx_application_class_init (KgxApplicationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GApplicationClass *app_class = G_APPLICATION_CLASS (klass);
  GtkApplicationClass *gtk_app_class = GTK_APPLICATION_CLASS (klass);

  object_class->dispose = kgx_application_dispose;
  object_class->get_property = kgx_application_get_property;

  app_class->activate = kgx_application_activate;
  app_class->startup = kgx_application_startup;
  app_class->open = kgx_application_open;
  app_class->local_command_line = kgx_application_local_command_line;
  app_class->command_line = kgx_application_command_line;
  app_class->handle_local_options = kgx_application_handle_local_options;

  gtk_app_class->window_added = kgx_application_window_added;
  gtk_app_class->window_removed = kgx_application_window_removed;

  pspecs[PROP_IN_BACKGROUND] = g_param_spec_boolean ("in-background", NULL, NULL,
                                                     FALSE,
                                                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);

  g_type_ensure (KGX_TYPE_WINDOW);
}


static GOptionEntry entries[] = {
  {
    "version",
    0,
    0,
    G_OPTION_ARG_NONE,
    NULL,
    NULL,
    NULL
  },
  {
    "about",
    0,
    0,
    G_OPTION_ARG_NONE,
    NULL,
    NULL,
    NULL
  },
  {
    "tab",
    0,
    0,
    G_OPTION_ARG_NONE,
    NULL,
    NULL,
    NULL
  },
  {
    "command",
    'e',
    0,
    G_OPTION_ARG_FILENAME,
    NULL,
    N_("Execute the argument to this option inside the terminal"),
    N_("COMMAND")
  },
  {
    "working-directory",
    0,
    0,
    G_OPTION_ARG_FILENAME,
    NULL,
    N_("Set the working directory"),
    // Translators: Placeholder of for a given directory
    N_("DIRNAME")
  },
  {
    "wait",
    0,
    0,
    G_OPTION_ARG_NONE,
    NULL,
    N_("Wait until the child exits (TODO)"),
    NULL
  },
  {
    "title",
    'T',
    0,
    G_OPTION_ARG_STRING,
    NULL,
    N_("Set the initial window title"),
    N_("TITLE")
  },
  {
    "set-shell",
    0,
    0,
    G_OPTION_ARG_STRING_ARRAY,
    NULL,
    N_("ADVANCED: Set the shell to launch"),
    N_("SHELL")
  },
  {
    "set-scrollback",
    0,
    0,
    G_OPTION_ARG_INT64,
    NULL,
    N_("ADVANCED: Set the scrollback length"),
    N_("LINES")
  },
  {
    G_OPTION_REMAINING,
    0,
    G_OPTION_FLAG_HIDDEN,
    G_OPTION_ARG_FILENAME_ARRAY,
    NULL,
    NULL,
    NULL,
  },
  {
    "colour-table",
    0,
    G_OPTION_FLAG_HIDDEN,
    G_OPTION_ARG_NONE,
    NULL,
    NULL,
    NULL,
  },
  { NULL }
};


static void
new_window_activated (GSimpleAction *action,
                      GVariant      *parameter,
                      gpointer       data)
{
  KgxApplication *self = KGX_APPLICATION (data);

  kgx_application_add_terminal (self, NULL, NULL, NULL, NULL);
}


static void
new_tab_activated (GSimpleAction *action,
                   GVariant      *parameter,
                   gpointer       data)
{
  KgxApplication *self = KGX_APPLICATION (data);
  g_autoptr (GFile) dir = NULL;
  GtkWindow *window;

  window = gtk_application_get_active_window (GTK_APPLICATION (self));
  if (window) {
    dir = kgx_window_get_working_dir (KGX_WINDOW (window));

    kgx_application_add_terminal (self, KGX_WINDOW (window), dir, NULL, NULL);
  } else {
    kgx_application_add_terminal (self, NULL, NULL, NULL, NULL);
  }
}


static void
focus_activated (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       data)
{
  KgxApplication *self = KGX_APPLICATION (data);
  GtkRoot *root;
  KgxPages *pages;
  KgxTab *page;

  page = kgx_application_lookup_page (self, g_variant_get_uint32 (parameter));
  pages = kgx_tab_get_pages (page);
  kgx_pages_focus_page (pages, page);
  root = gtk_widget_get_root (GTK_WIDGET (pages));

  gtk_window_present (GTK_WINDOW (root));
}


static void
zoom_out_activated (GSimpleAction *action,
                    GVariant      *parameter,
                    gpointer       data)
{
  KgxApplication *self = KGX_APPLICATION (data);

  kgx_settings_decrease_scale (self->settings);
}


static void
zoom_normal_activated (GSimpleAction *action,
                       GVariant      *parameter,
                       gpointer       data)
{
  KgxApplication *self = KGX_APPLICATION (data);

  kgx_settings_reset_scale (self->settings);
}


static void
zoom_in_activated (GSimpleAction *action,
                   GVariant      *parameter,
                   gpointer       data)
{
  KgxApplication *self = KGX_APPLICATION (data);

  kgx_settings_increase_scale (self->settings);
}


static GActionEntry app_entries[] = {
  { "new-window", new_window_activated, NULL, NULL, NULL },
  { "new-tab", new_tab_activated, NULL, NULL, NULL },
  { "focus-page", focus_activated, "u", NULL, NULL },
  { "zoom-out", zoom_out_activated, NULL, NULL, NULL },
  { "zoom-normal", zoom_normal_activated, NULL, NULL, NULL },
  { "zoom-in", zoom_in_activated, NULL, NULL, NULL },
};


static gboolean
scale_to_can_reset (GBinding     *binding,
                    const GValue *from_value,
                    GValue       *to_value,
                    gpointer      user_data)
{
  double scale = g_value_get_double (from_value);

  g_value_set_boolean (to_value,
                       fabs (scale - KGX_FONT_SCALE_DEFAULT) > 0.05);

  return TRUE;
}


static void
kgx_application_init (KgxApplication *self)
{
  GAction *action;
  g_autoptr (GPropertyAction) theme_action = NULL;
  g_autofree char *version = kgx_about_dup_version_string ();
  /* Translators: %s is the version string, KGX is a codename and should be left as-is */
  g_autofree char *summary = g_strdup_printf (_("KGX %s — Terminal Emulator"), version);

  g_application_add_main_option_entries (G_APPLICATION (self), entries);
  g_application_set_option_context_description (G_APPLICATION (self),
                                                KGX_HOMEPAGE_URL);
  g_application_set_option_context_parameter_string (G_APPLICATION (self),
                                                     _("[-e|-- COMMAND [ARGUMENT...]]"));
  g_application_set_option_context_summary (G_APPLICATION (self), summary);

  g_action_map_add_action_entries (G_ACTION_MAP (self),
                                   app_entries,
                                   G_N_ELEMENTS (app_entries),
                                   self);

  self->active_windows = g_hash_table_new (g_direct_hash, g_direct_equal);

  self->settings = g_object_new (KGX_TYPE_SETTINGS, NULL);

  action = g_action_map_lookup_action (G_ACTION_MAP (self), "zoom-out");
  g_object_bind_property (self->settings, "scale-can-decrease",
                          action, "enabled",
                          G_BINDING_SYNC_CREATE);

  action = g_action_map_lookup_action (G_ACTION_MAP (self), "zoom-normal");
  g_object_bind_property_full (self->settings, "font-scale",
                               action, "enabled",
                               G_BINDING_SYNC_CREATE,
                               scale_to_can_reset, NULL, NULL, NULL);

  action = g_action_map_lookup_action (G_ACTION_MAP (self), "zoom-in");
  g_object_bind_property (self->settings, "scale-can-increase",
                          action, "enabled",
                          G_BINDING_SYNC_CREATE);

  theme_action = g_property_action_new ("theme", self->settings, "theme");
  g_action_map_add_action (G_ACTION_MAP (self), G_ACTION (theme_action));

  self->watcher = g_object_new (KGX_TYPE_WATCHER, NULL);
  g_object_bind_property (self, "in-background",
                          self->watcher, "in-background",
                          G_BINDING_SYNC_CREATE);

  self->depot = g_object_new (KGX_TYPE_DEPOT,
                              "watcher", self->watcher,
                              "train-type", KGX_TYPE_TRAIN,
                              NULL);

  self->pages = g_tree_new_full (kgx_pid_cmp, NULL, NULL, NULL);
}


struct _PageDiedData {
  KgxApplication *app;
  guint tab_id;
};


KGX_DEFINE_DATA (PageDiedData, page_died_data)


static void
page_died_data_cleanup (PageDiedData *self)
{
  g_clear_weak_pointer (&self->app);
}


static void
page_died (gpointer user_data, GObject *dead_object)
{
  PageDiedData *data = user_data;

  if (G_LIKELY (data->app)) {
    g_tree_remove (data->app->pages, GINT_TO_POINTER (data->tab_id));
  }

  page_died_data_free (data);
}


/**
 * kgx_application_add_page:
 * @self: the instance to look for @id in
 * @page: the page to add
 *
 * Register a new #KgxTab with @self
 */
void
kgx_application_add_page (KgxApplication *self,
                          KgxTab         *page)
{
  g_autoptr (PageDiedData) data = page_died_data_alloc ();

  g_return_if_fail (KGX_IS_APPLICATION (self));
  g_return_if_fail (KGX_IS_TAB (page));

  g_set_weak_pointer (&data->app, self);
  data->tab_id = kgx_tab_get_id (page);

  g_tree_insert (self->pages, GINT_TO_POINTER (data->tab_id), page);
  g_object_weak_ref (G_OBJECT (page), page_died, g_steal_pointer (&data));
}


/**
 * kgx_application_lookup_page:
 * @self: the instance to look for @id in
 * @id: the page id to look for
 *
 * Try and find a #KgxTab with @id in @self
 *
 * Returns: (transfer none) (nullable): the found #KgxTab or %NULL
 */
KgxTab *
kgx_application_lookup_page (KgxApplication *self,
                             guint           id)
{
  g_return_val_if_fail (KGX_IS_APPLICATION (self), NULL);

  return g_tree_lookup (self->pages, GUINT_TO_POINTER (id));
}


static void
started (GObject      *src,
         GAsyncResult *res,
         gpointer      app)
{
  g_autoptr (GError) error = NULL;

  kgx_tab_start_finish (KGX_TAB (src), res, &error);

  if (error) {
    g_warning ("Failed to start %s: %s",
               G_OBJECT_TYPE_NAME (src),
               error->message);
  }
}


KgxTab *
kgx_application_add_terminal (KgxApplication *self,
                              KgxWindow      *existing_window,
                              GFile          *working_directory,
                              GStrv           argv,
                              const char     *title)
{
  g_autofree char *directory = NULL;
  GtkWindow *window;
  KgxTab *tab;

  if (working_directory) {
    directory = g_file_get_path (working_directory);
  }

  tab = g_object_new (KGX_TYPE_SIMPLE_TAB,
                      "application", self,
                      "settings", self->settings,
                      "depot", self->depot,
                      "initial-work-dir", directory,
                      "command", argv,
                      "tab-title", title,
                      "close-on-quit", argv == NULL,
                      NULL);
  kgx_tab_start (tab, started, self);

  if (existing_window) {
    window = GTK_WINDOW (existing_window);
  } else {
    GtkWindow *active_window;
    int width = -1, height = -1;
    gboolean maximised = FALSE;

    if (kgx_settings_get_restore_size (self->settings)) {
      active_window = gtk_application_get_active_window (GTK_APPLICATION (self));
      if (active_window) {
        gtk_window_get_default_size (active_window, &width, &height);
      } else {
        kgx_settings_get_size (self->settings, &width, &height, &maximised);
      }
    }

    g_debug ("app: new window (%i×%i)", width, height);

    window = g_object_new (KGX_TYPE_WINDOW,
                           "application", self,
                           "settings", self->settings,
                           "default-width", width,
                           "default-height", height,
                           "maximized", maximised,
                           NULL);
  }

  kgx_window_add_tab (KGX_WINDOW (window), tab);

  gtk_window_present (window);

  return KGX_TAB (tab);
}
