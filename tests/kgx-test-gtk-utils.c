/* kgx-test-gtk-utils.c
 *
 * Copyright 2026 Zander Brown
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

#include <gtk/gtk.h>

#ifdef GDK_WINDOWING_X11
#include <gdk/x11/gdkx.h>
#endif

#include "kgx-utils.h"

#include "kgx-test-gtk-utils.h"


G_GNUC_UNUSED
void
kgx_test_flush_widget (GtkWidget *widget)
{
  GdkDisplay *display = gtk_widget_get_display (widget);

  g_assert_nonnull (display);

  gtk_test_widget_wait_for_draw (widget);
  gdk_display_sync (display);
}


G_GNUC_UNUSED
gboolean
kgx_test_is_on_x11 (GtkWidget *widget)
{
#ifdef GDK_WINDOWING_X11
  GdkDisplay *display = gtk_widget_get_display (widget);

  g_assert_nonnull (display);

  return GDK_IS_X11_DISPLAY (display);
#else
  return FALSE;
#endif
}


struct _ScreenshotData {
  GtkWidget *widget;
  char *filename;
  gboolean done;
};


KGX_DEFINE_DATA (ScreenshotData, screenshot_data)


static void
screenshot_data_cleanup (ScreenshotData *self)
{
  g_clear_weak_pointer (&self->widget);
  g_clear_pointer (&self->filename, g_free);
}


static void
invalidate_contents (GdkPaintable *paintable, gpointer user_data)
{
  g_autoptr (GtkSnapshot) snapshot = NULL;
  g_autoptr (GskRenderNode) node = NULL;
  g_autoptr (GdkTexture) texture = NULL;
  ScreenshotData *data = user_data;
  GskRenderer *renderer;
  graphene_rect_t bounds;

  if (data->done) {
    return;
  }

  snapshot = gtk_snapshot_new ();
  gdk_paintable_snapshot (paintable,
                          snapshot,
                          gdk_paintable_get_intrinsic_width (paintable),
                          gdk_paintable_get_intrinsic_height (paintable));
  node = gtk_snapshot_free_to_node (g_steal_pointer (&snapshot));
  if (!node) {
    return;
  }

  renderer = gtk_native_get_renderer (gtk_widget_get_native (data->widget));
  g_assert_nonnull (renderer);

  gsk_render_node_get_bounds (node, &bounds);
  graphene_rect_union (&bounds,
                       &GRAPHENE_RECT_INIT (
                         0, 0,
                         gdk_paintable_get_intrinsic_width (paintable),
                         gdk_paintable_get_intrinsic_height (paintable)
                       ),
                       &bounds);

  texture = gsk_renderer_render_texture (renderer, node, &bounds);

  gdk_texture_save_to_png (texture, data->filename);

  data->done = TRUE;
}


G_GNUC_UNUSED
void
kgx_test_save_screenshot (GtkWidget *widget, const char *filename)
{
  g_autoptr (ScreenshotData) data = screenshot_data_alloc ();
  g_autoptr (GdkPaintable) paintable = NULL;
  gboolean destroy_window = FALSE;
  GtkWidget *window;

  g_set_weak_pointer (&data->widget, widget);
  g_set_str (&data->filename, filename);
  data->done = FALSE;

  if (GTK_IS_WINDOW (widget)) {
    window = widget;
  } else {
    window = g_object_new (GTK_TYPE_WINDOW, "child", widget, NULL);
    destroy_window = TRUE;
  }

  gtk_window_present (GTK_WINDOW (window));

  paintable = gtk_widget_paintable_new (widget);
  g_signal_connect (paintable,
                    "invalidate-contents", G_CALLBACK (invalidate_contents),
                    data);

  while (!data->done) {
    g_main_context_iteration (NULL, TRUE);
  }

  if (destroy_window) {
    gtk_window_destroy (GTK_WINDOW (window));
  }
}
