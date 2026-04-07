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
#include "kgx-test-utils.h"

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
  gboolean *done = data;

  *done = TRUE;

  prop_notified = g_param_spec_get_name (pspec);
}


static void
test_empty_map_unmap (void)
{
  if (g_test_subprocess ()) {
    g_autoptr (KgxEmpty) empty = g_object_new (KGX_TYPE_EMPTY, NULL);
    GtkWidget *window = gtk_window_new ();
    gboolean done = FALSE;
    gboolean working = TRUE;
    gboolean logo_visible = TRUE;
    gboolean spinner_visible = TRUE;

    g_object_ref_sink (empty);

    gtk_window_set_child (GTK_WINDOW (window), GTK_WIDGET (empty));

    g_signal_connect (empty,
                      "notify::logo-visible", G_CALLBACK (got_notify),
                      &done);
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

    while (!done) {
      g_main_context_iteration (NULL, TRUE);
    }

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

    return;
  }

  /* Shouldn't take anything close to five seconds, but process start can
   * be slow on CI runners */
  g_test_trap_subprocess (NULL, 5 * G_USEC_PER_SEC, 0);
  g_test_trap_assert_passed ();
}


static void
test_empty_working (void)
{
  if (g_test_subprocess ()) {
    g_autoptr (KgxEmpty) empty = g_object_new (KGX_TYPE_EMPTY, NULL);
    gboolean done = FALSE;
    gboolean working = TRUE;
    gboolean logo_visible = TRUE;
    gboolean spinner_visible = TRUE;

    g_object_ref_sink (empty);

    g_signal_connect (empty,
                      "notify::spinner-visible", G_CALLBACK (got_notify),
                      &done);

    g_object_get (empty,
                  "working", &working,
                  "logo-visible", &logo_visible,
                  "spinner-visible", &spinner_visible,
                  NULL);

    g_assert_false (working);
    g_assert_false (logo_visible);
    g_assert_false (spinner_visible);

    kgx_expect_property_notify (empty, "working");
    g_object_set (empty, "working", TRUE, NULL);
    kgx_assert_expected_notifies (empty);

    g_object_get (empty,
                  "working", &working,
                  "logo-visible", &logo_visible,
                  "spinner-visible", &spinner_visible,
                  NULL);

    g_assert_true (working);
    g_assert_false (logo_visible);
    g_assert_false (spinner_visible);

    prop_notified = NULL;

    while (!done) {
      g_main_context_iteration (NULL, TRUE);
    }

    g_assert_cmpstr (prop_notified, ==, "spinner-visible");

    g_object_get (empty,
                  "working", &working,
                  "logo-visible", &logo_visible,
                  "spinner-visible", &spinner_visible,
                  NULL);

    g_assert_true (working);
    g_assert_false (logo_visible);
    g_assert_true (spinner_visible);

    kgx_expect_property_notify (empty, "working");
    g_object_set (empty, "working", FALSE, NULL);
    kgx_assert_expected_notifies (empty);

    g_object_get (empty,
                  "working", &working,
                  "logo-visible", &logo_visible,
                  "spinner-visible", &spinner_visible,
                  NULL);

    g_assert_false (working);
    g_assert_false (logo_visible);
    g_assert_false (spinner_visible);

    return;
  }

  /* Shouldn't take anything close to five seconds, but process start can
   * be slow on CI runners */
  g_test_trap_subprocess (NULL, 5 * G_USEC_PER_SEC, 0);
  g_test_trap_assert_passed ();
}


static void
test_empty_working_unmap (void)
{
  g_autoptr (KgxEmpty) empty = g_object_new (KGX_TYPE_EMPTY, NULL);
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
