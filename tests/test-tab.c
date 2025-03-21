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


static const char *prop_notified = FALSE;


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

  g_main_loop_quit (loop);

  g_test_fail ();
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
  g_timeout_add_once (250, timeout, loop);

  g_main_loop_run (loop);

  g_assert_cmpstr (prop_notified, ==, "initial-title");

  g_object_get (tab,
                "initial-title", &res_title,
                "initial-path", &res_path,
                NULL);

  g_assert_cmpstr (res_title, ==, NULL);
  g_assert_null (res_path);
}


int
main (int argc, char *argv[])
{
  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func ("/kgx/tab/new-destroy", test_tab_new_destroy);
  g_test_add_func ("/kgx/tab/initial-title", test_tab_initial_title);

  return g_test_run ();
}
