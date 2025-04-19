/* test-empty.c
 *
 * Copyright 2025 Zander Brown
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

#include "kgx-locale.h"

#include "kgx-empty.h"


static void
test_empty_new_destroy (void)
{
  KgxEmpty *empty = g_object_new (KGX_TYPE_EMPTY, NULL);

  g_object_ref_sink (empty);

  g_assert_finalize_object (empty);
}


static const char *prop_notified = NULL;


static void
got_notify (GObject *self, GParamSpec *pspec, gpointer data)
{
  GMainLoop *loop = data;

  g_main_loop_quit (loop);

  prop_notified = g_param_spec_get_name (pspec);
}


static void
timeout (gpointer data)
{
  GMainLoop *loop = data;

  g_test_fail ();

  g_main_loop_quit (loop);
}


static void
test_empty_map_unmap (void)
{
  g_autoptr (KgxEmpty) empty = g_object_new (KGX_TYPE_EMPTY, NULL);
  g_autoptr (GMainLoop) loop = g_main_loop_new (NULL, FALSE);
  GtkWidget *window = gtk_window_new ();
  gboolean working = TRUE;
  gboolean logo_visible = TRUE;
  gboolean spinner_visible = TRUE;
  guint source;

  g_object_ref_sink (empty);

  gtk_window_set_child (GTK_WINDOW (window), GTK_WIDGET (empty));

  g_signal_connect (empty,
                    "notify::logo-visible", G_CALLBACK (got_notify),
                    loop);
  prop_notified = NULL;

  g_object_get (empty,
                "working", &working,
                "logo-visible", &logo_visible,
                "spinner-visible", &spinner_visible,
                NULL);

  g_assert_false (working);
  g_assert_false (logo_visible);
  g_assert_false (spinner_visible);

  gtk_widget_map (GTK_WIDGET (empty));

  source = g_timeout_add_once (100, timeout, loop);

  g_main_loop_run (loop);

  g_assert_cmpstr (prop_notified, ==, "logo-visible");

  g_object_get (empty,
                "working", &working,
                "logo-visible", &logo_visible,
                "spinner-visible", &spinner_visible,
                NULL);

  g_assert_false (working);
  g_assert_true (logo_visible);
  g_assert_false (spinner_visible);

  gtk_widget_unmap (GTK_WIDGET (empty));

  gtk_window_destroy (GTK_WINDOW (window));

  g_clear_handle_id (&source, g_source_remove);
}


static void
test_empty_working (void)
{
  g_autoptr (KgxEmpty) empty = g_object_new (KGX_TYPE_EMPTY, NULL);
  g_autoptr (GMainLoop) loop = g_main_loop_new (NULL, FALSE);
  gboolean working = TRUE;
  gboolean logo_visible = TRUE;
  gboolean spinner_visible = TRUE;
  guint source;

  g_object_ref_sink (empty);

  g_signal_connect (empty,
                    "notify::spinner-visible", G_CALLBACK (got_notify),
                    loop);
  g_signal_connect (empty,
                    "notify::working", G_CALLBACK (got_notify),
                    loop);
  prop_notified = NULL;

  g_object_get (empty,
                "working", &working,
                "logo-visible", &logo_visible,
                "spinner-visible", &spinner_visible,
                NULL);

  g_assert_false (working);
  g_assert_false (logo_visible);
  g_assert_false (spinner_visible);

  source = g_timeout_add_once (120, timeout, loop);

  g_object_set (empty, "working", TRUE, NULL);
  g_assert_cmpstr (prop_notified, ==, "working");

  g_object_get (empty,
                "working", &working,
                "logo-visible", &logo_visible,
                "spinner-visible", &spinner_visible,
                NULL);

  g_assert_true (working);
  g_assert_false (logo_visible);
  g_assert_false (spinner_visible);

  prop_notified = NULL;
  g_main_loop_run (loop);

  g_assert_cmpstr (prop_notified, ==, "spinner-visible");

  g_object_get (empty,
                "working", &working,
                "logo-visible", &logo_visible,
                "spinner-visible", &spinner_visible,
                NULL);

  g_assert_true (working);
  g_assert_false (logo_visible);
  g_assert_true (spinner_visible);

  g_object_set (empty, "working", FALSE, NULL);
  g_assert_cmpstr (prop_notified, ==, "working");

  g_object_get (empty,
                "working", &working,
                "logo-visible", &logo_visible,
                "spinner-visible", &spinner_visible,
                NULL);

  g_assert_false (working);
  g_assert_false (logo_visible);
  g_assert_false (spinner_visible);

  g_clear_handle_id (&source, g_source_remove);
}


static void
test_empty_working_unmap (void)
{
  g_autoptr (KgxEmpty) empty = g_object_new (KGX_TYPE_EMPTY, NULL);
  g_autoptr (GMainLoop) loop = g_main_loop_new (NULL, FALSE);
  GtkWidget *window = gtk_window_new ();
  gboolean working = TRUE;
  gboolean logo_visible = TRUE;
  gboolean spinner_visible = TRUE;

  g_object_ref_sink (empty);

  gtk_window_set_child (GTK_WINDOW (window), GTK_WIDGET (empty));

  g_object_get (empty,
                "working", &working,
                "logo-visible", &logo_visible,
                "spinner-visible", &spinner_visible,
                NULL);

  g_assert_false (working);
  g_assert_false (logo_visible);
  g_assert_false (spinner_visible);

  gtk_widget_map (GTK_WIDGET (empty));

  g_object_set (empty, "working", TRUE, NULL);

  g_object_get (empty,
                "working", &working,
                "logo-visible", &logo_visible,
                "spinner-visible", &spinner_visible,
                NULL);

  g_assert_true (working);
  g_assert_false (logo_visible);
  g_assert_false (spinner_visible);

  gtk_widget_unmap (GTK_WIDGET (empty));

  /* We get map/unmapped when switching tabs */
  gtk_widget_map (GTK_WIDGET (empty));

  gtk_widget_unmap (GTK_WIDGET (empty));

  gtk_window_destroy (GTK_WINDOW (window));
}


int
main (int argc, char *argv[])
{
  kgx_locale_init (KGX_LOCALE_TEST);

  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func ("/kgx/empty/new-destroy", test_empty_new_destroy);
  g_test_add_func ("/kgx/empty/map-unmap", test_empty_map_unmap);
  g_test_add_func ("/kgx/empty/working", test_empty_working);
  g_test_add_func ("/kgx/empty/working-unmap", test_empty_working_unmap);

  return g_test_run ();
}
