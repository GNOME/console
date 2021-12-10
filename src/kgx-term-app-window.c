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
#include <gdk/wayland/gdkwayland.h>
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

  g_clear_object (&self->entry);

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


static GdkTexture *
load_icon_texture (KgxTermAppWindow *self,
                   GtkIconTheme     *theme,
                   GIcon            *icon,
                   int               scale_factor,
                   GtkDirectionType  dir)
{
  g_autoptr (GtkIconPaintable) paintable = NULL;
  g_autoptr (GFile) file = NULL;
  g_autoptr (GdkTexture) texture = NULL;
  g_autoptr (GError) error = NULL;

  paintable = gtk_icon_theme_lookup_by_gicon (theme,
                                              icon,
                                              64,
                                              scale_factor,
                                              dir,
                                              0);
  file = gtk_icon_paintable_get_file (paintable);
  if (!file) {
    return NULL;
  }

  texture = gdk_texture_new_from_file (file, &error);
  if (error) {
    g_debug ("Unable to load icon (%s): %s",
             g_file_get_uri (file),
             error->message);
    return NULL;
  }

  return g_steal_pointer (&texture);
}


static void
setup_icon (KgxTermAppWindow *self)
{
  g_autolist (GdkTexture) list = NULL;
  g_autoptr (GdkTexture) texture = NULL;
  g_autoptr (GIcon) fallback = NULL;
  GtkIconTheme *theme;
  GdkDisplay *display;
  GIcon *icon;
  int scale_factor;
  GtkDirectionType dir;
  GtkNative *native;
  GdkSurface *surface;

  display = gtk_widget_get_display (GTK_WIDGET (self));

  theme = gtk_icon_theme_get_for_display (display);
  icon = g_app_info_get_icon (G_APP_INFO (self->entry));
  scale_factor = gtk_widget_get_scale_factor (GTK_WIDGET (self));
  dir = gtk_widget_get_direction (GTK_WIDGET (self));

  texture = load_icon_texture (self, theme, icon, scale_factor, dir);
  if (texture) {
    list = g_list_append (list, g_steal_pointer (&texture));
  }

  fallback = g_themed_icon_new ("text-x-script");
  texture = load_icon_texture (self, theme, fallback, scale_factor, dir);

  if (texture) {
    list = g_list_append (list, g_steal_pointer (&texture));
  }

  native = gtk_widget_get_native (GTK_WIDGET (self));
  surface = gtk_native_get_surface (native);

  gdk_toplevel_set_icon_list (GDK_TOPLEVEL (surface), list);
}


#ifdef GDK_WINDOWING_WAYLAND
static void
setup_appid (KgxTermAppWindow *self)
{
  g_autofree char *app_id = NULL;
  GtkNative *native;
  GdkSurface *surface;

  native = gtk_widget_get_native (GTK_WIDGET (self));
  surface = gtk_native_get_surface (native);

  if (!GDK_IS_WAYLAND_SURFACE (surface)) {
    return;
  }

  app_id = g_strdup (g_app_info_get_id (G_APP_INFO (self->entry)));

  if (G_UNLIKELY (!app_id)) {
    return;
  }

  if (G_LIKELY (g_str_has_suffix (app_id, ".desktop"))) {
    size_t len = strlen (app_id);

    for (size_t i = len - 1; i > 0; i--) {
      if (app_id[i] == '.') {
        app_id[i] = '\0';
        break;
      }
    }
  }

  g_debug ("Adopting app-id ‘%s’ for surface %p", app_id, surface);

  gdk_wayland_toplevel_set_application_id (GDK_TOPLEVEL (surface), app_id);
}
#endif


static void
kgx_term_app_window_map (GtkWidget *widget)
{
  KgxTermAppWindow *self = KGX_TERM_APP_WINDOW (widget);

  GTK_WIDGET_CLASS (kgx_term_app_window_parent_class)->map (widget);

  setup_icon (self);
#ifdef GDK_WINDOWING_WAYLAND
  setup_appid (self);
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
  g_object_connect (self,
                    "swapped-signal::notify::scale-factor", G_CALLBACK (setup_icon), self,
                    "swapped-signal::direction-changed", G_CALLBACK (setup_icon), self,
                    NULL);
}
