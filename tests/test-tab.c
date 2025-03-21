/* test-tab.c
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

#include "kgx-application.h"
#include "kgx-settings.h"

#include "kgx-tab.h"


#define KGX_TYPE_TEST_TAB (kgx_test_tab_get_type ())

G_DECLARE_FINAL_TYPE (KgxTestTab, kgx_test_tab, KGX, TEST_TAB, KgxTab)


struct _KgxTestTab {
  KgxTab parent;
};


G_DEFINE_FINAL_TYPE (KgxTestTab, kgx_test_tab, KGX_TYPE_TAB)


static void
kgx_test_tab_class_init (KgxTestTabClass *klass)
{

}


static void
kgx_test_tab_init (KgxTestTab *self)
{

}


static void
test_tab_new_destroy (void)
{
  g_autoptr (KgxApplication) app = g_object_new (KGX_TYPE_APPLICATION, NULL);
  KgxTab *tab = g_object_new (KGX_TYPE_TEST_TAB, "application", app, NULL);

  g_object_ref_sink (tab);

  g_assert_finalize_object (tab);
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
test_tab_working (void)
{
  g_autoptr (KgxApplication) app = g_object_new (KGX_TYPE_APPLICATION, NULL);
  g_autoptr (KgxTab) tab = g_object_new (KGX_TYPE_TEST_TAB, "application", app, NULL);
  g_autoptr (GMainLoop) loop = g_main_loop_new (NULL, FALSE);
  g_autoptr (GError) error = NULL;
  gboolean working = FALSE;

  g_object_ref_sink (tab);

  g_application_register (G_APPLICATION (app), NULL, &error);

  g_assert_no_error (error);

  g_assert_false (g_application_get_is_busy (G_APPLICATION (app)));

  g_object_get (tab, "working", &working, NULL);
  g_assert_false (working);

  g_signal_connect (tab,
                    "notify::working", G_CALLBACK (got_notify),
                    loop);
  prop_notified = NULL;

  kgx_tab_mark_working (tab);

  g_assert_cmpstr (prop_notified, ==, "working");

  g_object_get (tab, "working", &working, NULL);
  g_assert_true (working);

  g_assert_true (g_application_get_is_busy (G_APPLICATION (app)));

  prop_notified = NULL;
  kgx_tab_mark_working (tab);

  g_assert_cmpstr (prop_notified, ==, NULL);

  g_object_get (tab, "working", &working, NULL);
  g_assert_true (working);

  kgx_tab_unmark_working (tab);

  g_object_get (tab, "working", &working, NULL);
  g_assert_true (working);

  prop_notified = NULL;
  kgx_tab_unmark_working (tab);

  g_assert_cmpstr (prop_notified, ==, "working");

  g_object_get (tab, "working", &working, NULL);
  g_assert_false (working);

  g_assert_false (g_application_get_is_busy (G_APPLICATION (app)));
}


static void
timeout (gpointer data)
{
  GMainLoop *loop = data;

  g_test_fail ();

  g_main_loop_quit (loop);
}


static void
test_tab_bell (void)
{
  g_autoptr (KgxApplication) app = g_object_new (KGX_TYPE_APPLICATION, NULL);
  g_autoptr (KgxSettings) settings = g_object_new (KGX_TYPE_SETTINGS, NULL);
  g_autoptr (KgxTab) tab = g_object_new (KGX_TYPE_TEST_TAB,
                                         "application", app,
                                         "settings", settings,
                                         NULL);
  g_autoptr (GMainLoop) loop = g_main_loop_new (NULL, FALSE);
  gboolean ringing = FALSE;
  guint source;

  g_object_ref_sink (tab);

  g_object_get (tab, "ringing", &ringing, NULL);
  g_assert_false (ringing);

  g_signal_connect (tab,
                    "notify::ringing", G_CALLBACK (got_notify),
                    loop);

  kgx_tab_bell (tab);

  g_object_get (tab, "ringing", &ringing, NULL);
  g_assert_true (ringing);

  /* bell should ring for 500, so if we are still running
   * after 550 we can assume it isn't going to happen, and fail */
  source = g_timeout_add_once (550, timeout, loop);

  g_main_loop_run (loop);

  g_assert_cmpstr (prop_notified, ==, "ringing");

  g_object_get (tab, "ringing", &ringing, NULL);
  g_assert_false (ringing);

  g_clear_handle_id (&source, g_source_remove);
}


static void
test_tab_initial_title (void)
{
  g_autoptr (KgxApplication) app = g_object_new (KGX_TYPE_APPLICATION, NULL);
  g_autoptr (KgxTab) tab = g_object_new (KGX_TYPE_TEST_TAB, "application", app, NULL);
  g_autoptr (GFile) path = g_file_new_for_path ("placeholder");
  g_autofree char *res_title = NULL;
  g_autoptr (GFile) res_path = NULL;
  g_autoptr (GMainLoop) loop = g_main_loop_new (NULL, FALSE);
  guint source;

  g_object_ref_sink (tab);

  g_signal_connect (tab,
                    "notify::initial-title", G_CALLBACK (got_notify),
                    loop);

  kgx_tab_set_initial_title (tab, "placeholder", path);

  g_object_get (tab,
                "tab-title", &res_title,
                "tab-path", &res_path,
                NULL);

  g_assert_cmpstr (res_title, ==, NULL);
  g_assert_null (res_path);

  g_object_get (tab,
                "initial-title", &res_title,
                "initial-path", &res_path,
                NULL);

  g_assert_cmpstr (res_title, ==, "placeholder");
  g_assert_true (g_file_equal (res_path, path));
  g_clear_pointer (&res_title, g_free);
  g_clear_object (&res_path);

  /* initial title should go away after 200, so if we are still running
   * after 250 we can assume it isn't going to happen, and fail */
  source = g_timeout_add_once (250, timeout, loop);

  g_main_loop_run (loop);

  g_assert_cmpstr (prop_notified, ==, "initial-title");

  g_object_get (tab,
                "initial-title", &res_title,
                "initial-path", &res_path,
                NULL);

  g_assert_cmpstr (res_title, ==, NULL);
  g_assert_null (res_path);

  g_clear_handle_id (&source, g_source_remove);
}


int
main (int argc, char *argv[])
{
  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func ("/kgx/tab/new-destroy", test_tab_new_destroy);
  g_test_add_func ("/kgx/tab/working", test_tab_working);
  g_test_add_func ("/kgx/tab/bell", test_tab_bell);
  g_test_add_func ("/kgx/tab/initial-title", test_tab_initial_title);

  return g_test_run ();
}
