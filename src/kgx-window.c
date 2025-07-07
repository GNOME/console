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

#include <adwaita.h>

#include "kgx-about.h"
#include "kgx-application.h"
#include "kgx-close-dialog.h"
#include "kgx-empty.h"
#include "kgx-file-closures.h"
#include "kgx-fullscreen-box.h"
#include "kgx-pages.h"
#include "kgx-settings.h"
#include "kgx-shared-closures.h"
#include "kgx-terminal.h"
#include "kgx-theme-switcher.h"
#include "kgx-utils.h"
#include "preferences/kgx-preferences-window.h"

#include "kgx-window.h"


typedef struct _KgxWindowPrivate KgxWindowPrivate;
struct _KgxWindowPrivate {
  KgxSettings          *settings;
  GBindingGroup        *settings_binds;

  gboolean              search_enabled;
  gboolean              floating;
  gboolean              translucent;

  gboolean              close_anyway;

  /* Template widgets */
  GtkWidget            *theme_switcher;
  GtkWidget            *tab_bar;
  GtkWidget            *tab_overview;
  GtkWidget            *pages;

  GBindingGroup        *surface_binds;
};


G_DEFINE_TYPE_WITH_PRIVATE (KgxWindow, kgx_window, ADW_TYPE_APPLICATION_WINDOW)


enum {
  PROP_0,
  PROP_SETTINGS,
  PROP_SEARCH_MODE_ENABLED,
  PROP_FLOATING,
  PROP_TRANSLUCENT,
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
      kgx_set_boolean_prop (object, pspec, &priv->search_enabled, value);
      break;
    case PROP_FLOATING:
      kgx_set_boolean_prop (object, pspec, &priv->floating, value);
      break;
    case PROP_TRANSLUCENT:
      if (kgx_set_boolean_prop (object, pspec, &priv->translucent, value)) {
        if (priv->translucent) {
          gtk_widget_add_css_class (GTK_WIDGET (self), "translucent");
        } else {
          gtk_widget_remove_css_class (GTK_WIDGET (self), "translucent");
        }
      }
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
    case PROP_FLOATING:
      g_value_set_boolean (value, priv->floating);
      break;
    case PROP_TRANSLUCENT:
      g_value_set_boolean (value, priv->translucent);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
kgx_window_realize (GtkWidget *widget)
{
  KgxWindowPrivate *priv = kgx_window_get_instance_private (KGX_WINDOW (widget));

  GTK_WIDGET_CLASS (kgx_window_parent_class)->realize (widget);

  g_binding_group_set_source (priv->surface_binds,
                              gtk_native_get_surface (GTK_NATIVE (widget)));
}


static void
kgx_window_unrealize (GtkWidget *widget)
{
  KgxWindowPrivate *priv = kgx_window_get_instance_private (KGX_WINDOW (widget));

  g_binding_group_set_source (priv->surface_binds, NULL);

  GTK_WIDGET_CLASS (kgx_window_parent_class)->unrealize (widget);
}


static void
got_close (GObject      *source,
           GAsyncResult *res,
           gpointer      user_data)
{
  g_autoptr (KgxCloseDialog) dialogue = KGX_CLOSE_DIALOG (source);
  g_autoptr (KgxWindow) self = user_data;
  g_autoptr (GError) error = NULL;
  KgxWindowPrivate *priv = kgx_window_get_instance_private (self);
  KgxCloseDialogResult result;

  result = kgx_close_dialog_run_finish (dialogue, res, &error);

  if (G_UNLIKELY (error)) {
    g_critical ("Unexpected: %s", error->message);
    return;
  }

  if (result == KGX_CLOSE_ANYWAY) {
    priv->close_anyway = TRUE;
    gtk_window_close (GTK_WINDOW (self));
  }
}


static gboolean
kgx_window_close_request (GtkWindow *window)
{
  KgxWindow *self = KGX_WINDOW (window);
  KgxWindowPrivate *priv = kgx_window_get_instance_private (self);
  g_autoptr (GPtrArray) children = NULL;

  children = kgx_pages_get_children (KGX_PAGES (priv->pages));

  if (children->len < 1 || priv->close_anyway) {
    if (gtk_window_is_active (GTK_WINDOW (self)) &&
        !adw_application_window_get_adaptive_preview (ADW_APPLICATION_WINDOW (self))) {
      gboolean maximised = gtk_window_is_maximized (GTK_WINDOW (self));
      int width, height;

      gtk_window_get_default_size (GTK_WINDOW (self), &width, &height);
      kgx_settings_set_custom_size (priv->settings, width, height, maximised);
    }

    return FALSE; /* Aka no, I don’t want to block closing */
  }

  kgx_close_dialog_run (g_object_new (KGX_TYPE_CLOSE_DIALOG,
                                      "context", KGX_CONTEXT_WINDOW,
                                      "commands", children,
                                      NULL),
                        GTK_WIDGET (self),
                        NULL,
                        got_close,
                        g_object_ref (self));

  return TRUE; /* Block the close */
}


static void
fullscreened_changed (KgxWindow *self)
{
  gboolean fullscreen = gtk_window_is_fullscreen (GTK_WINDOW (self));
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "win.fullscreen", !fullscreen);
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "win.unfullscreen", fullscreen);
}


static char *
content_or_empty (G_GNUC_UNUSED GObject *object, int n_pages)
{
  if (G_LIKELY (n_pages > 0)) {
    return g_strdup ("content");
  }

  return g_strdup ("empty");
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
  GtkApplication *application = NULL;
  g_autoptr (GFile) dir = NULL;

  application = gtk_window_get_application (GTK_WINDOW (self));
  dir = kgx_window_get_working_dir (self);

  kgx_application_add_terminal (KGX_APPLICATION (application),
                                NULL,
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
  GtkApplication *application = NULL;
  g_autoptr (GFile) dir = NULL;

  application = gtk_window_get_application (GTK_WINDOW (self));
  dir = kgx_window_get_working_dir (self);

  kgx_application_add_terminal (KGX_APPLICATION (application),
                                self,
                                dir,
                                NULL,
                                NULL);
}


static void
about_activated  (GtkWidget                *widget,
                  G_GNUC_UNUSED const char *action_name,
                  G_GNUC_UNUSED GVariant   *parameter)
{
  kgx_about_present_dialogue (widget);
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
  KgxWindowPrivate *priv =
    kgx_window_get_instance_private (KGX_WINDOW (widget));

  adw_dialog_present (g_object_new (KGX_TYPE_PREFERENCES_WINDOW,
                                    "settings", priv->settings,
                                    NULL),
                      widget);
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

  widget_class->realize = kgx_window_realize;
  widget_class->unrealize = kgx_window_unrealize;

  window_class->close_request = kgx_window_close_request;

  pspecs[PROP_SETTINGS] =
    g_param_spec_object ("settings", NULL, NULL,
                         KGX_TYPE_SETTINGS,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_SEARCH_MODE_ENABLED] =
    g_param_spec_boolean ("search-mode-enabled", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  pspecs[PROP_FLOATING] =
    g_param_spec_boolean ("floating", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  pspecs[PROP_TRANSLUCENT] =
    g_param_spec_boolean ("translucent", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);

  gtk_widget_class_install_property_action (widget_class,
                                            "win.find",
                                            "search-mode-enabled");

  g_type_ensure (KGX_TYPE_EMPTY);
  g_type_ensure (KGX_TYPE_FULLSCREEN_BOX);
  g_type_ensure (KGX_TYPE_PAGES);
  g_type_ensure (KGX_TYPE_THEME_SWITCHER);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               KGX_APPLICATION_PATH "kgx-window.ui");

  gtk_widget_class_bind_template_child_private (widget_class, KgxWindow, theme_switcher);
  gtk_widget_class_bind_template_child_private (widget_class, KgxWindow, tab_bar);
  gtk_widget_class_bind_template_child_private (widget_class, KgxWindow, tab_overview);
  gtk_widget_class_bind_template_child_private (widget_class, KgxWindow, pages);
  gtk_widget_class_bind_template_child_private (widget_class, KgxWindow, settings_binds);
  gtk_widget_class_bind_template_child_private (widget_class, KgxWindow, surface_binds);

  gtk_widget_class_bind_template_callback (widget_class, fullscreened_changed);
  gtk_widget_class_bind_template_callback (widget_class, content_or_empty);
  gtk_widget_class_bind_template_callback (widget_class, zoom);
  gtk_widget_class_bind_template_callback (widget_class, create_tearoff_host);
  gtk_widget_class_bind_template_callback (widget_class, maybe_close_window);
  gtk_widget_class_bind_template_callback (widget_class, status_changed);
  gtk_widget_class_bind_template_callback (widget_class, ringing_changed);
  gtk_widget_class_bind_template_callback (widget_class, extra_drag_drop);
  gtk_widget_class_bind_template_callback (widget_class, create_tab_cb);
  gtk_widget_class_bind_template_callback (widget_class, breakpoint_applied);
  gtk_widget_class_bind_template_callback (widget_class, breakpoint_unapplied);

  gtk_widget_class_bind_template_callback (widget_class, kgx_gtk_settings_for_display);
  gtk_widget_class_bind_template_callback (widget_class, kgx_text_or_fallback);
  gtk_widget_class_bind_template_callback (widget_class, kgx_bool_and);
  gtk_widget_class_bind_template_callback (widget_class, kgx_format_percentage);
  gtk_widget_class_bind_template_callback (widget_class, kgx_decoration_layout_is_inverted);
  gtk_widget_class_bind_template_callback (widget_class, kgx_file_as_subtitle);

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

  gtk_widget_class_install_action (widget_class,
                                   "win.fullscreen",
                                   NULL,
                                   (GtkWidgetActionActivateFunc) gtk_window_fullscreen);
  gtk_widget_class_install_action (widget_class,
                                   "win.unfullscreen",
                                   NULL,
                                   (GtkWidgetActionActivateFunc) gtk_window_unfullscreen);
}


static gboolean
state_to_floating (GBinding     *binding,
                   const GValue *from_value,
                   GValue       *to_value,
                   gpointer      user_data)
{
  GdkToplevelState state = g_value_get_flags (from_value);

  g_value_set_boolean (to_value,
                       !((state & (GDK_TOPLEVEL_STATE_FULLSCREEN |
                                   GDK_TOPLEVEL_STATE_MAXIMIZED |
                                   GDK_TOPLEVEL_STATE_TILED |
                                   GDK_TOPLEVEL_STATE_TOP_TILED |
                                   GDK_TOPLEVEL_STATE_RIGHT_TILED |
                                   GDK_TOPLEVEL_STATE_BOTTOM_TILED |
                                   GDK_TOPLEVEL_STATE_LEFT_TILED)) > 0));

  return TRUE;
}


static void
kgx_window_init (KgxWindow *self)
{
  KgxWindowPrivate *priv = kgx_window_get_instance_private (self);
  GType drop_types[] = { GDK_TYPE_FILE_LIST, G_TYPE_STRING };
  g_autoptr (GtkWindowGroup) group = NULL;

  gtk_widget_init_template (GTK_WIDGET (self));

  g_binding_group_bind (priv->settings_binds, "theme",
                        priv->theme_switcher, "theme",
                        G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);

  g_binding_group_bind_full (priv->surface_binds, "state",
                             self, "floating",
                             G_BINDING_SYNC_CREATE,
                             state_to_floating, NULL, NULL, NULL);

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

  fullscreened_changed (self);

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
