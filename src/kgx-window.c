/* kgx-window.c
 *
 * Copyright 2019-2023 Zander Brown
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
#include <math.h>
#include <adwaita.h>

#include "rgba.h"

#include "kgx-window.h"
#include "kgx-application.h"
#include "kgx-close-dialog.h"
#include "kgx-pages.h"
#include "kgx-preferences-window.h"
#include "kgx-theme-switcher.h"
#include "kgx-watcher.h"


/**
 * KgxWindow:
 * @close_anyway: ignore running children and close without prompt
 * @pages: the #KgxPages of #KgxPage current in the window
 *
 * The main #AdwApplicationWindow that acts as the terminal
 */
typedef struct _KgxWindowPrivate KgxWindowPrivate;
struct _KgxWindowPrivate {
  KgxSettings          *settings;
  GBindingGroup        *settings_binds;

  gboolean              search_enabled;

  gboolean              close_anyway;

  /* Template widgets */
  GtkWidget            *theme_switcher;
  GtkWidget            *tab_bar;
  GtkWidget            *tab_overview;
  GtkWidget            *pages;
};


G_DEFINE_TYPE_WITH_PRIVATE (KgxWindow, kgx_window, ADW_TYPE_APPLICATION_WINDOW)


enum {
  PROP_0,
  PROP_SETTINGS,
  PROP_SEARCH_MODE_ENABLED,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };


static void
kgx_window_dispose (GObject *object)
{
  KgxWindow *self = KGX_WINDOW (object);
  KgxWindowPrivate *priv = kgx_window_get_instance_private (self);

  g_clear_object (&priv->settings);

  G_OBJECT_CLASS (kgx_window_parent_class)->dispose (object);
}


static void
kgx_window_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  KgxWindow *self = KGX_WINDOW (object);
  KgxWindowPrivate *priv = kgx_window_get_instance_private (self);

  switch (property_id) {
    case PROP_SETTINGS:
      g_set_object (&priv->settings, g_value_get_object (value));
      break;
    case PROP_SEARCH_MODE_ENABLED:
      priv->search_enabled = g_value_get_boolean (value);
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
  KgxWindowPrivate *priv = kgx_window_get_instance_private (self);

  switch (property_id) {
    case PROP_SETTINGS:
      g_value_set_object (value, priv->settings);
      break;
    case PROP_SEARCH_MODE_ENABLED:
      g_value_set_boolean (value, priv->search_enabled);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
close_response (KgxWindow *self)
{
  KgxWindowPrivate *priv = kgx_window_get_instance_private (self);

  priv->close_anyway = TRUE;

  gtk_window_destroy (GTK_WINDOW (self));
}


static gboolean
kgx_window_close_request (GtkWindow *window)
{
  KgxWindow *self = KGX_WINDOW (window);
  KgxWindowPrivate *priv = kgx_window_get_instance_private (self);
  GtkWidget *dlg;
  g_autoptr (GPtrArray) children = NULL;

  children = kgx_pages_get_children (KGX_PAGES (priv->pages));

  if (children->len < 1 || priv->close_anyway) {
    if (gtk_window_is_active (GTK_WINDOW (self))) {
      int width, height;
      gtk_window_get_default_size (GTK_WINDOW (self), &width, &height);
      kgx_settings_set_custom_size (priv->settings, width, height);
    }

    return FALSE; /* Aka no, I don’t want to block closing */
  }

  dlg = kgx_close_dialog_new (KGX_CONTEXT_WINDOW, children);

  gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (self));

  g_signal_connect_swapped (dlg, "response::close", G_CALLBACK (close_response), self);

  gtk_window_present (GTK_WINDOW (dlg));

  return TRUE; /* Block the close */
}


static void
active_changed (GObject *object, GParamSpec *pspec, gpointer data)
{
  if (gtk_window_is_active (GTK_WINDOW (object))) {
    kgx_watcher_push_active (kgx_watcher_get_default ());
  } else {
    kgx_watcher_pop_active (kgx_watcher_get_default ());
  }
}


static void
zoom (KgxPages  *pages,
      KgxZoom    dir,
      KgxWindow *self)
{
  GAction *action = NULL;
  GtkApplication *app = gtk_window_get_application (GTK_WINDOW (self));

  switch (dir) {
    case KGX_ZOOM_IN:
      action = g_action_map_lookup_action (G_ACTION_MAP (app), "zoom-in");
      break;
    case KGX_ZOOM_OUT:
      action = g_action_map_lookup_action (G_ACTION_MAP (app), "zoom-out");
      break;
    default:
      g_return_if_reached ();
  }

  g_action_activate (action, NULL);
}


static KgxPages *
create_tearoff_host (KgxPages *pages, KgxWindow *self)
{
  KgxWindowPrivate *priv = kgx_window_get_instance_private (self);
  GtkApplication *application = gtk_window_get_application (GTK_WINDOW (self));
  KgxWindow *new_window;
  KgxWindowPrivate *new_priv;
  int width, height;

  gtk_window_get_default_size (GTK_WINDOW (self), &width, &height);

  new_window = g_object_new (KGX_TYPE_WINDOW,
                             "application", application,
                             "settings", priv->settings,
                             "default-width", width,
                             "default-height", height,
                             NULL);
  gtk_window_present (GTK_WINDOW (new_window));

  new_priv = kgx_window_get_instance_private (new_window);

  return KGX_PAGES (new_priv->pages);
}


static void
maybe_close_window (KgxWindow *self)
{
  KgxWindowPrivate *priv = kgx_window_get_instance_private (self);

  if (adw_tab_overview_get_open (ADW_TAB_OVERVIEW (priv->tab_overview))) {
    return;
  }

  gtk_window_close (GTK_WINDOW (self));
}


static void
status_changed (GObject *object, GParamSpec *pspec, gpointer data)
{
  KgxWindow *self = KGX_WINDOW (object);
  KgxWindowPrivate *priv = kgx_window_get_instance_private (self);
  KgxStatus status;

  status = kgx_pages_current_status (KGX_PAGES (priv->pages));

  if (status & KGX_REMOTE) {
    gtk_widget_add_css_class (GTK_WIDGET (self), KGX_WINDOW_STYLE_REMOTE);
  } else {
    gtk_widget_remove_css_class (GTK_WIDGET (self), KGX_WINDOW_STYLE_REMOTE);
  }

  if (status & KGX_PRIVILEGED) {
    gtk_widget_add_css_class (GTK_WIDGET (self), KGX_WINDOW_STYLE_ROOT);
  } else {
    gtk_widget_remove_css_class (GTK_WIDGET (self), KGX_WINDOW_STYLE_ROOT);
  }
}


static void
ringing_changed (GObject *object, GParamSpec *pspec, gpointer data)
{
  KgxWindow *self = KGX_WINDOW (object);
  KgxWindowPrivate *priv = kgx_window_get_instance_private (self);

  if (kgx_pages_is_ringing (KGX_PAGES (priv->pages))) {
    gtk_widget_add_css_class (GTK_WIDGET (self), KGX_WINDOW_STYLE_RINGING);
  } else {
    gtk_widget_remove_css_class (GTK_WIDGET (self), KGX_WINDOW_STYLE_RINGING);
  }
}


static void
extra_drag_drop (AdwTabBar        *bar,
                 AdwTabPage       *page,
                 GValue           *value,
                 KgxWindow        *self)
{
  KgxTab *tab = KGX_TAB (adw_tab_page_get_child (page));

  kgx_tab_extra_drop (tab, value);
}


static AdwTabPage *
create_tab_cb (KgxWindow *self)
{
  KgxWindowPrivate *priv = kgx_window_get_instance_private (self);

  gtk_widget_activate_action (GTK_WIDGET (self), "win.new-tab", NULL);

  return kgx_pages_get_selected_page (KGX_PAGES (priv->pages));
}


static void
breakpoint_applied (KgxWindow *self)
{
  gtk_widget_action_set_enabled (GTK_WIDGET (self),
                                 "win.show-tabs-desktop",
                                 FALSE);
}


static void
breakpoint_unapplied (KgxWindow *self)
{
  KgxWindowPrivate *priv = kgx_window_get_instance_private (self);

  gtk_widget_action_set_enabled (GTK_WIDGET (self),
                                 "win.show-tabs-desktop",
                                 TRUE);

  if (kgx_pages_count (KGX_PAGES (priv->pages)) > 0) {
    adw_tab_overview_set_open (ADW_TAB_OVERVIEW (priv->tab_overview), FALSE);
  }
}


static char *
title_or_fallback (GObject *object, const char *title)
{
  if (G_UNLIKELY (!title || !title[0])) {
    return g_strdup (KGX_DISPLAY_NAME);
  }

  return g_strdup (title);
}


static char *
path_as_subtitle (GObject *object, GFile *file)
{
  g_autofree char *path = NULL;
  const char *home = NULL;

  if (!file) {
    return NULL;
  }

  path = g_file_get_path (file);
  if (G_UNLIKELY (!path)) {
    return NULL;
  }

  home = g_get_home_dir ();
  if (G_LIKELY (g_str_has_prefix (path, home))) {
    g_autofree char *short_home = g_strdup_printf ("~%s",
                                                   path + strlen (home));

    return g_steal_pointer (&short_home);
  }

  return g_steal_pointer (&path);
}


static char *
scale_as_label (GObject *object, double scale)
{
  return g_strdup_printf ("%i%%", (int) round (scale * 100));
}


static void
close_tab_activated (GtkWidget  *widget,
                     const char *action_name,
                     GVariant   *parameter)
{
  KgxWindow *self = KGX_WINDOW (widget);
  KgxWindowPrivate *priv = kgx_window_get_instance_private (self);

  kgx_pages_close_page (KGX_PAGES (priv->pages));
}


static void
detach_tab_activated (GtkWidget  *widget,
                      const char *action_name,
                      GVariant   *parameter)
{
  KgxWindow *self = KGX_WINDOW (widget);
  KgxWindowPrivate *priv = kgx_window_get_instance_private (self);

  kgx_pages_detach_page (KGX_PAGES (priv->pages));
}


static void
new_activated (GtkWidget  *widget,
               const char *action_name,
               GVariant   *parameter)
{
  KgxWindow *self = KGX_WINDOW (widget);
  guint32 timestamp = GDK_CURRENT_TIME;
  GtkApplication *application = NULL;
  g_autoptr (GFile) dir = NULL;

  application = gtk_window_get_application (GTK_WINDOW (self));
  dir = kgx_window_get_working_dir (self);

  kgx_application_add_terminal (KGX_APPLICATION (application),
                                NULL,
                                timestamp,
                                dir,
                                NULL,
                                NULL);
}


static void
new_tab_activated (GtkWidget  *widget,
                   const char *action_name,
                   GVariant   *parameter)
{
  KgxWindow *self = KGX_WINDOW (widget);
  guint32 timestamp = GDK_CURRENT_TIME;
  GtkApplication *application = NULL;
  g_autoptr (GFile) dir = NULL;

  application = gtk_window_get_application (GTK_WINDOW (self));
  dir = kgx_window_get_working_dir (self);

  kgx_application_add_terminal (KGX_APPLICATION (application),
                                self,
                                timestamp,
                                dir,
                                NULL,
                                NULL);
}


static void
about_activated  (GtkWidget  *widget,
                  const char *action_name,
                  GVariant   *parameter)
{
  const char *developers[] = { "Zander Brown <zbrown@gnome.org>", NULL };
  const char *designers[] = { "Tobias Bernard", NULL };
  g_autofree char *copyright = NULL;

  /* Translators: %s is the year range */
  copyright = g_strdup_printf (_("© %s Zander Brown"), "2019-2023");

  adw_show_about_window (GTK_WINDOW (widget),
                         "application-name", KGX_DISPLAY_NAME,
                         "application-icon", KGX_APPLICATION_ID,
                         "developer-name", _("The GNOME Project"),
                         "issue-url", "https://gitlab.gnome.org/GNOME/console/-/issues/new",
                         "website", "https://apps.gnome.org/en-GB/app/org.gnome.Console/",
                         "version", PACKAGE_VERSION,
                         "developers", developers,
                         "designers", designers,
                         /* Translators: Credit yourself here */
                         "translator-credits", _("translator-credits"),
                         "copyright", copyright,
                         "license-type", GTK_LICENSE_GPL_3_0,
                         NULL);
}


static void
tab_switcher_activated (GtkWidget  *widget,
                                   const char *action_name,
                                   GVariant   *parameter)
{
  KgxWindow *self = KGX_WINDOW (widget);
  KgxWindowPrivate *priv = kgx_window_get_instance_private (self);

  adw_tab_overview_set_open (ADW_TAB_OVERVIEW (priv->tab_overview), TRUE);
}


static void
show_preferences_window_activated (GtkWidget  *widget,
                                   const char *action_name,
                                   GVariant   *parameter)
{
  KgxWindow *self = KGX_WINDOW (widget);
  KgxWindowPrivate *priv = kgx_window_get_instance_private (self);
  GtkWindow *preferences;

  preferences = g_object_new (KGX_TYPE_PREFERENCES_WINDOW,
                              "transient-for", self,
                              "settings", priv->settings,
                              NULL);
  gtk_window_present (preferences);
}


static void
kgx_window_class_init (KgxWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkWindowClass *window_class = GTK_WINDOW_CLASS (klass);

  object_class->dispose = kgx_window_dispose;
  object_class->set_property = kgx_window_set_property;
  object_class->get_property = kgx_window_get_property;

  window_class->close_request = kgx_window_close_request;

  pspecs[PROP_SETTINGS] =
    g_param_spec_object ("settings", NULL, NULL,
                         KGX_TYPE_SETTINGS,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_SEARCH_MODE_ENABLED] =
    g_param_spec_boolean ("search-mode-enabled", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);

  gtk_widget_class_install_property_action (widget_class,
                                            "win.find",
                                            "search-mode-enabled");

  gtk_widget_class_set_template_from_resource (widget_class,
                                               KGX_APPLICATION_PATH "kgx-window.ui");

  gtk_widget_class_bind_template_child_private (widget_class, KgxWindow, theme_switcher);
  gtk_widget_class_bind_template_child_private (widget_class, KgxWindow, tab_bar);
  gtk_widget_class_bind_template_child_private (widget_class, KgxWindow, tab_overview);
  gtk_widget_class_bind_template_child_private (widget_class, KgxWindow, pages);
  gtk_widget_class_bind_template_child_private (widget_class, KgxWindow, settings_binds);

  gtk_widget_class_bind_template_callback (widget_class, active_changed);
  gtk_widget_class_bind_template_callback (widget_class, zoom);
  gtk_widget_class_bind_template_callback (widget_class, create_tearoff_host);
  gtk_widget_class_bind_template_callback (widget_class, maybe_close_window);
  gtk_widget_class_bind_template_callback (widget_class, status_changed);
  gtk_widget_class_bind_template_callback (widget_class, ringing_changed);
  gtk_widget_class_bind_template_callback (widget_class, extra_drag_drop);
  gtk_widget_class_bind_template_callback (widget_class, create_tab_cb);
  gtk_widget_class_bind_template_callback (widget_class, breakpoint_applied);
  gtk_widget_class_bind_template_callback (widget_class, breakpoint_unapplied);
  gtk_widget_class_bind_template_callback (widget_class, title_or_fallback);
  gtk_widget_class_bind_template_callback (widget_class, path_as_subtitle);
  gtk_widget_class_bind_template_callback (widget_class, scale_as_label);

  gtk_widget_class_install_action (widget_class, "tab.close", NULL, close_tab_activated);
  gtk_widget_class_install_action (widget_class, "tab.detach", NULL, detach_tab_activated);

  gtk_widget_class_install_action (widget_class,
                                   "win.new-window",
                                   NULL,
                                   new_activated);
  gtk_widget_class_install_action (widget_class,
                                   "win.new-tab",
                                   NULL,
                                   new_tab_activated);
  gtk_widget_class_install_action (widget_class,
                                   "win.close-tab",
                                   NULL,
                                   close_tab_activated);
  gtk_widget_class_install_action (widget_class,
                                   "win.about",
                                   NULL,
                                   about_activated);
  gtk_widget_class_install_action (widget_class,
                                   "win.show-tabs",
                                   NULL,
                                   tab_switcher_activated);
  gtk_widget_class_install_action (widget_class,
                                   "win.show-tabs-desktop",
                                   NULL,
                                   tab_switcher_activated);
  gtk_widget_class_install_action (widget_class,
                                   "win.show-preferences-window",
                                   NULL,
                                   show_preferences_window_activated);
}


static void
kgx_window_init (KgxWindow *self)
{
  KgxWindowPrivate *priv = kgx_window_get_instance_private (self);
  GType drop_types[] = { GDK_TYPE_FILE_LIST, G_TYPE_STRING };
  g_autoptr (GtkWindowGroup) group = NULL;
  AdwStyleManager *style_manager;

  g_type_ensure (KGX_TYPE_THEME_SWITCHER);

  gtk_widget_init_template (GTK_WIDGET (self));

  style_manager = adw_style_manager_get_default ();

  g_object_bind_property (style_manager, "system-supports-color-schemes",
                          priv->theme_switcher, "show-system",
                          G_BINDING_SYNC_CREATE);

  g_binding_group_bind (priv->settings_binds, "theme",
                        priv->theme_switcher, "theme",
                        G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);

  #ifdef IS_DEVEL
  gtk_widget_add_css_class (GTK_WIDGET (self), "devel");
  #endif

  /* Note this unfortunately doesn't allow us to workaround the portal
     situation, but hopefully dropping folders on tabs is relatively rare */
  adw_tab_bar_setup_extra_drop_target (ADW_TAB_BAR (priv->tab_bar),
                                       GDK_ACTION_COPY,
                                       drop_types,
                                       G_N_ELEMENTS (drop_types));
  adw_tab_overview_setup_extra_drop_target (ADW_TAB_OVERVIEW (priv->tab_overview),
                                            GDK_ACTION_COPY,
                                            drop_types,
                                            G_N_ELEMENTS (drop_types));

  group = gtk_window_group_new ();
  gtk_window_group_add_window (group, GTK_WINDOW (self));
}


/**
 * kgx_window_get_working_dir:
 * @self: the #KgxWindow
 *
 * Get the working directory path of this window, used to open new windows
 * in the same directory
 *
 * Returns: (transfer full):
 */
GFile *
kgx_window_get_working_dir (KgxWindow *self)
{
  KgxWindowPrivate *priv;
  GFile *file = NULL;

  g_return_val_if_fail (KGX_IS_WINDOW (self), NULL);

  priv = kgx_window_get_instance_private (self);

  g_object_get (priv->pages, "path", &file, NULL);

  return file;
}


/**
 * kgx_window_add_tab:
 * @self: the #KgxWindow
 * @tab: (transfer none): a #KgxTab
 *
 * Adopt a (currently unowned) tab into @self, and present it
 */
void
kgx_window_add_tab (KgxWindow *self,
                    KgxTab    *tab)
{
  KgxWindowPrivate *priv;

  g_return_if_fail (KGX_IS_WINDOW (self));
  g_return_if_fail (KGX_IS_TAB (tab));

  priv = kgx_window_get_instance_private (self);

  kgx_pages_add_page (KGX_PAGES (priv->pages), tab);
  kgx_pages_focus_page (KGX_PAGES (priv->pages), tab);
}
