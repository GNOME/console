/* kgx-term-app-window.c
 *
 * Copyright 2021 Zander Brown
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
 * SECTION:kgx-term-app-window
 * @title: KgxTermAppWindow
 * @short_description: A #KgxWindow for a specific termapp
 */

#include "kgx-config.h"

#include <glib/gi18n.h>
#include <gdk/gdk.h>
#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/gdkwayland.h>
#endif

#include "kgx-term-app-window.h"


G_DEFINE_TYPE (KgxTermAppWindow, kgx_term_app_window, KGX_TYPE_WINDOW)


enum {
  PROP_0,
  PROP_DESKTOP_ENTRY,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };


static void
kgx_term_app_window_dispose (GObject *object)
{
  KgxTermAppWindow *self = KGX_TERM_APP_WINDOW (object);

  if (self->cancellable && G_IS_CANCELLABLE (self->cancellable)) {
    g_cancellable_cancel (self->cancellable);
  }

  g_clear_object (&self->entry);
  g_clear_object (&self->cancellable);

  G_OBJECT_CLASS (kgx_term_app_window_parent_class)->dispose (object);
}


static void
kgx_term_app_window_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  KgxTermAppWindow *self = KGX_TERM_APP_WINDOW (object);

  switch (property_id) {
    case PROP_DESKTOP_ENTRY:
      g_value_set_object (value, self->entry);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
kgx_term_app_window_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  KgxTermAppWindow *self = KGX_TERM_APP_WINDOW (object);

  switch (property_id) {
    case PROP_DESKTOP_ENTRY:
      g_clear_object (&self->entry);
      self->entry = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
icon_loaded (GObject *src, GAsyncResult *res, gpointer data)
{
  g_autoptr (GdkPixbuf) pixbuf = NULL;
  g_autoptr (GError) error = NULL;

  pixbuf = gtk_icon_info_load_icon_finish (GTK_ICON_INFO (src), res, &error);

  if (error) {
    g_warning ("Unable to set icon: %s", error->message);

    return;
  }

  gtk_window_set_icon (GTK_WINDOW (data), pixbuf);
}


static void
kgx_term_app_window_map (GtkWidget *widget)
{
  KgxTermAppWindow *self = KGX_TERM_APP_WINDOW (widget);
  g_autofree char *basename = NULL;
  g_autoptr (GtkIconInfo) info = NULL;
  g_auto (GStrv) parts = NULL;
  GIcon *icon;
  GdkWindow *window;

  GTK_WIDGET_CLASS (kgx_term_app_window_parent_class)->map (widget);

  window = gtk_widget_get_window (widget);

  if (!window) {
    g_debug ("Apparently no window?");
    return;
  }

  icon = g_app_info_get_icon (G_APP_INFO (self->entry));

  info = gtk_icon_theme_lookup_by_gicon (gtk_icon_theme_get_default (), icon, 64, 0);
  gtk_icon_info_load_icon_async (info, self->cancellable, icon_loaded, self);

#ifdef GDK_WINDOWING_WAYLAND
  basename = g_path_get_basename (g_desktop_app_info_get_filename (self->entry));

  parts = g_strsplit (basename, ".", 2);

  if (GDK_IS_WAYLAND_WINDOW (window) && parts[1] && g_strcmp0 (parts[1], "desktop") == 0) {
    g_print ("Hey: %p\n", self->entry);
    gdk_wayland_window_set_application_id (GDK_WAYLAND_WINDOW (window), parts[0]);
  }
#endif
}


static void
kgx_term_app_window_class_init (KgxTermAppWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = kgx_term_app_window_dispose;
  object_class->get_property = kgx_term_app_window_get_property;
  object_class->set_property = kgx_term_app_window_set_property;

  widget_class->map = kgx_term_app_window_map;

  /**
   * KgxTermAppWindow:desktop-entry:
   *
   * The #GDesktopAppInfo
   */
  pspecs[PROP_DESKTOP_ENTRY] =
    g_param_spec_object ("desktop-entry", "Desktop Entry", "The termapp entry",
                         G_TYPE_DESKTOP_APP_INFO,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);
}


static void
kgx_term_app_window_init (KgxTermAppWindow *self)
{
  self->cancellable = g_cancellable_new ();
}
