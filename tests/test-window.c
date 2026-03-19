/* test-window.c
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

#include "kgx-application.h"
#include "kgx-ignore-deprecations.h"
#include "kgx-locale.h"
#include "kgx-settings.h"
#include "kgx-test-utils.h"

#include "kgx-window.h"


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
test_window_new_destroy (void)
{
  KgxWindow *window = g_object_new (KGX_TYPE_WINDOW, NULL);

  g_assert_finalize_object (window);
}


static void
test_window_search_mode (void)
{
  g_autoptr (KgxWindow) window = g_object_new (KGX_TYPE_WINDOW, NULL);
  gboolean enabled;

  g_object_get (window, "search-mode-enabled", &enabled, NULL);
  g_assert_false (enabled);

  kgx_test_property_notify (window, "search-mode-enabled", TRUE, FALSE);

  gtk_widget_activate_action (GTK_WIDGET (window), "win.find", NULL);

  g_object_get (window, "search-mode-enabled", &enabled, NULL);
  g_assert_true (enabled);

  gtk_widget_activate_action (GTK_WIDGET (window), "win.find", NULL);

  g_object_get (window, "search-mode-enabled", &enabled, NULL);
  g_assert_false (enabled);
}


static void
test_window_settings_direct (void)
{
  g_autoptr (KgxSettings) settings_a = g_object_new (KGX_TYPE_SETTINGS, NULL);
  g_autoptr (KgxSettings) settings_b = g_object_new (KGX_TYPE_SETTINGS, NULL);
  g_autoptr (KgxWindow) window_a = g_object_new (KGX_TYPE_WINDOW, NULL);
  g_autoptr (KgxWindow) window_b =
    g_object_new (KGX_TYPE_WINDOW, "settings", settings_a, NULL);
  g_autoptr (KgxSettings) settings_res = NULL;

  g_object_get (window_a, "settings", &settings_res, NULL);
  g_assert_null (settings_res);

  g_object_get (window_b, "settings", &settings_res, NULL);
  g_assert_nonnull (settings_res);
  g_assert_true (settings_res == settings_a);

  kgx_test_property_notify (window_a, "settings", settings_a, settings_b);
}


static void
test_window_floating (void)
{
  g_autoptr (KgxWindow) window = g_object_new (KGX_TYPE_WINDOW, NULL);
  gboolean floating;

  g_object_get (window, "floating", &floating, NULL);
  g_assert_false (floating);

  kgx_test_property_notify (window, "floating", TRUE, FALSE);

  gtk_window_present (GTK_WINDOW (window));

  g_assert_false (gtk_window_is_maximized (GTK_WINDOW (window)));

  g_object_get (window, "floating", &floating, NULL);
  g_assert_true (floating);

  gtk_test_widget_wait_for_draw (GTK_WIDGET (window));
  gtk_window_maximize (GTK_WINDOW (window));
  gtk_test_widget_wait_for_draw (GTK_WIDGET (window));

  g_assert_true (gtk_window_is_maximized (GTK_WINDOW (window)));

  g_object_get (window, "floating", &floating, NULL);
  g_assert_false (floating);
}


static void
test_window_translucent_direct (void)
{
  g_autoptr (KgxWindow) window = g_object_new (KGX_TYPE_WINDOW, NULL);
  gboolean translucent;

  g_object_get (window, "translucent", &translucent, NULL);
  g_assert_false (translucent);
  g_assert_false (gtk_widget_has_css_class (GTK_WIDGET (window), "translucent"));

  kgx_test_property_notify (window, "translucent", TRUE, FALSE);
  g_assert_false (gtk_widget_has_css_class (GTK_WIDGET (window), "translucent"));

  g_object_set (window, "translucent", TRUE, NULL);
  g_assert_true (gtk_widget_has_css_class (GTK_WIDGET (window), "translucent"));
}


static void
test_window_translucent_derived (void)
{
  g_autoptr (KgxSettings) settings = g_object_new (KGX_TYPE_SETTINGS, NULL);
  g_autoptr (KgxWindow) window = g_object_new (KGX_TYPE_WINDOW, "settings", settings, NULL);
  gboolean translucent;
  gboolean transparency;

  /* Prepare the window */
  gtk_window_present (GTK_WINDOW (window));
  gtk_test_widget_wait_for_draw (GTK_WIDGET (window));

  g_object_get (window, "translucent", &translucent, NULL);
  g_assert_false (translucent);
  g_assert_false (gtk_widget_has_css_class (GTK_WIDGET (window), "translucent"));

  g_object_get (settings, "transparency", &transparency, NULL);
  g_assert_false (transparency);

  g_assert_false (gtk_window_is_maximized (GTK_WINDOW (window)));

  /* Setting enabled, we are floating, should be translucent */
  kgx_expect_property_notify (window, "translucent");
  kgx_expect_property_notify (settings, "transparency");
  g_object_set (settings, "transparency", TRUE, NULL);
  gtk_test_widget_wait_for_draw (GTK_WIDGET (window));
  kgx_assert_expected_notifies (settings);
  kgx_assert_expected_notifies (window);

  g_object_get (window, "translucent", &translucent, NULL);
  g_assert_true (translucent);
  g_assert_true (gtk_widget_has_css_class (GTK_WIDGET (window), "translucent"));

  /* Maximised windows are opaque */
  kgx_expect_property_notify (window, "translucent");
  gtk_window_maximize (GTK_WINDOW (window));
  gtk_test_widget_wait_for_draw (GTK_WIDGET (window));
  kgx_assert_expected_notifies (window);

  g_object_get (window, "translucent", &translucent, NULL);
  g_assert_false (translucent);
  g_assert_false (gtk_widget_has_css_class (GTK_WIDGET (window), "translucent"));

  /* Back to floating, so translucent */
  kgx_expect_property_notify (window, "translucent");
  gtk_window_unmaximize (GTK_WINDOW (window));
  gtk_test_widget_wait_for_draw (GTK_WIDGET (window));
  kgx_assert_expected_notifies (window);

  g_object_get (window, "translucent", &translucent, NULL);
  g_assert_true (translucent);
  g_assert_true (gtk_widget_has_css_class (GTK_WIDGET (window), "translucent"));

  /* Disable setting, opaque even whilst floating */
  kgx_expect_property_notify (window, "translucent");
  kgx_expect_property_notify (settings, "transparency");
  g_object_set (settings, "transparency", FALSE, NULL);
  gtk_test_widget_wait_for_draw (GTK_WIDGET (window));
  kgx_assert_expected_notifies (settings);
  kgx_assert_expected_notifies (window);

  g_object_get (window, "translucent", &translucent, NULL);
  g_assert_false (translucent);
  g_assert_false (gtk_widget_has_css_class (GTK_WIDGET (window), "translucent"));

  /* We are already opaque, so maximising changes nothing */
  kgx_expect_property_not_notify (window, "translucent");
  gtk_window_maximize (GTK_WINDOW (window));
  gtk_test_widget_wait_for_draw (GTK_WIDGET (window));
  kgx_assert_expected_notifies (window);
}


static void
test_window_realize_unrealize (void)
{
  g_autoptr (KgxWindow) window = g_object_new (KGX_TYPE_WINDOW, NULL);

  gtk_widget_realize (GTK_WIDGET (window));

  gtk_widget_unrealize (GTK_WIDGET (window));
}


static void
test_window_trigger_breakpoints_empty (void)
{
  g_autoptr (KgxApplication) app = g_object_new (KGX_TYPE_APPLICATION, NULL);
  g_autoptr (KgxWindow) window = g_object_new (KGX_TYPE_WINDOW, NULL);
  int width;

  gtk_window_set_default_size (GTK_WINDOW (window), 360, 200);

  gtk_window_present (GTK_WINDOW (window));
  gtk_test_widget_wait_for_draw (GTK_WIDGET (window));

  /* Window is initally in the non-desktop size */
  gtk_window_get_default_size (GTK_WINDOW (window), &width, NULL);
  g_assert_cmpint (width, <, 400);

  /* Ideally we'd do something like assert win.show-tabs-desktop,
   * but we actually can't inspect that from outside gtk */

  /* Trigger a resize to a desktop size */
  g_object_set (window, "width-request", 460, NULL);
  gtk_test_widget_wait_for_draw (GTK_WIDGET (window));

  gtk_window_get_default_size (GTK_WINDOW (window), &width, NULL);
  g_assert_cmpint (width, >, 400);

  /* Again what we want to do is check win.show-tabs-desktop, but we can't,
   * so we just hope making it here means things are completely wrong */
}


static void
test_window_trigger_breakpoints_with_tab (void)
{
  /* TODO: Remove once libadwaita doesn't trigger this */
  G_GNUC_UNUSED g_autoptr (KgxIgnoreDeprecation) ignore =
    kgx_ignore_deprecation_new ("GtkPicture:keep-aspect-ratio", NULL);
  g_autoptr (KgxApplication) app = g_object_new (KGX_TYPE_APPLICATION, NULL);
  g_autoptr (KgxTestTab) tab =
    g_object_new (KGX_TYPE_TEST_TAB, "application", app, NULL);
  g_autoptr (KgxWindow) window = g_object_new (KGX_TYPE_WINDOW, NULL);
  int width;

  g_object_ref_sink (tab);

  gtk_window_set_default_size (GTK_WINDOW (window), 360, 200);

  gtk_window_present (GTK_WINDOW (window));
  gtk_test_widget_wait_for_draw (GTK_WIDGET (window));

  /* Now repeat with a tab loaded */
  kgx_window_add_tab (window, KGX_TAB (tab));

  gtk_window_get_default_size (GTK_WINDOW (window), &width, NULL);
  g_assert_cmpint (width, <, 400);

  /* To desktop size */
  g_object_set (window, "width-request", 460, NULL);
  gtk_test_widget_wait_for_draw (GTK_WIDGET (window));
}


static void
test_window_add_tab (void)
{
  /* TODO: Remove once libadwaita doesn't trigger this */
  G_GNUC_UNUSED g_autoptr (KgxIgnoreDeprecation) ignore =
    kgx_ignore_deprecation_new ("GtkPicture:keep-aspect-ratio", NULL);
  g_autoptr (KgxApplication) app = g_object_new (KGX_TYPE_APPLICATION, NULL);
  g_autoptr (KgxTestTab) tab_a =
    g_object_new (KGX_TYPE_TEST_TAB, "application", app, NULL);
  g_autoptr (KgxTestTab) tab_b =
    g_object_new (KGX_TYPE_TEST_TAB,
                  "application", app,
                  "tab-title", "World",
                  NULL);
  g_autoptr (KgxWindow) window = g_object_new (KGX_TYPE_WINDOW, NULL);

  g_object_ref_sink (tab_a);
  g_object_ref_sink (tab_b);

  /* Default title is just the display name */
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (window)),
                   ==,
                   KGX_DISPLAY_NAME);

  /* Tab with no title */
  kgx_window_add_tab (window, KGX_TAB (tab_a));
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (window)),
                   ==,
                   KGX_DISPLAY_NAME);

  /* Tab gains a title, now it's our title */
  g_object_set (tab_a, "tab-title", "Hello", NULL);
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (window)), ==, "Hello");

  /* Add another tab, this time already titled */
  kgx_window_add_tab (window, KGX_TAB (tab_b));
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (window)), ==, "World");
}


static void
done_ringing (GObject *object, GParamSpec *pspec, gpointer user_data)
{
  gboolean *done = user_data;

  *done = TRUE;
}


static void
test_window_ringing (void)
{
  /* TODO: Remove once libadwaita doesn't trigger this */
  G_GNUC_UNUSED g_autoptr (KgxIgnoreDeprecation) ignore =
    kgx_ignore_deprecation_new ("GtkPicture:keep-aspect-ratio", NULL);
  g_autoptr (KgxApplication) app = g_object_new (KGX_TYPE_APPLICATION, NULL);
  g_autoptr (KgxSettings) settings = g_object_new (KGX_TYPE_SETTINGS, NULL);
  g_autoptr (KgxTestTab) tab =
    g_object_new (KGX_TYPE_TEST_TAB,
                  "application", app,
                  "settings", settings,
                  NULL);
  g_autoptr (KgxWindow) window = g_object_new (KGX_TYPE_WINDOW, NULL);
  gboolean done = FALSE;

  g_object_ref_sink (tab);

  kgx_window_add_tab (window, KGX_TAB (tab));

  /* So we are initially not ringing */
  g_assert_false (gtk_widget_has_css_class (GTK_WIDGET (window), "bell"));

  /* Visual bell disabled */
  g_object_set (settings, "visual-bell", FALSE, NULL);

  /* Ring the bell */
  kgx_tab_bell (KGX_TAB (tab));

  /* Still not ringing */
  g_assert_false (gtk_widget_has_css_class (GTK_WIDGET (window), "bell"));

  /* Allow the visual bell */
  g_object_set (settings, "visual-bell", TRUE, NULL);

  /* Ring again */
  kgx_tab_bell (KGX_TAB (tab));

  /* Now we are ringing */
  g_assert_true (gtk_widget_has_css_class (GTK_WIDGET (window), "bell"));

  g_signal_connect (tab,
                    "notify::ringing", G_CALLBACK (done_ringing),
                    &done);

  /* Wait for ringing to end */
  while (!done) {
    g_main_context_iteration (NULL, TRUE);
  }

  g_assert_false (gtk_widget_has_css_class (GTK_WIDGET (window), "bell"));
}


static void
test_window_status (void)
{
  /* TODO: Remove once libadwaita doesn't trigger this */
  G_GNUC_UNUSED g_autoptr (KgxIgnoreDeprecation) ignore =
    kgx_ignore_deprecation_new ("GtkPicture:keep-aspect-ratio", NULL);
  g_autoptr (KgxApplication) app = g_object_new (KGX_TYPE_APPLICATION, NULL);
  g_autoptr (KgxSettings) settings = g_object_new (KGX_TYPE_SETTINGS, NULL);
  g_autoptr (KgxTestTab) tab =
    g_object_new (KGX_TYPE_TEST_TAB,
                  "application", app,
                  "settings", settings,
                  NULL);
  g_autoptr (KgxWindow) window = g_object_new (KGX_TYPE_WINDOW, NULL);

  g_object_ref_sink (tab);

  kgx_window_add_tab (window, KGX_TAB (tab));

  /* We are initially not root or remote */
  g_assert_false (gtk_widget_has_css_class (GTK_WIDGET (window), "root"));
  g_assert_false (gtk_widget_has_css_class (GTK_WIDGET (window), "remote"));
  g_assert_false (gtk_widget_has_css_class (GTK_WIDGET (window), "playbox"));

  g_object_set (tab, "tab-status", KGX_REMOTE, NULL);
  g_assert_false (gtk_widget_has_css_class (GTK_WIDGET (window), "root"));
  g_assert_true (gtk_widget_has_css_class (GTK_WIDGET (window), "remote"));
  g_assert_false (gtk_widget_has_css_class (GTK_WIDGET (window), "playbox"));

  g_object_set (tab, "tab-status", KGX_PRIVILEGED, NULL);
  g_assert_true (gtk_widget_has_css_class (GTK_WIDGET (window), "root"));
  g_assert_false (gtk_widget_has_css_class (GTK_WIDGET (window), "remote"));
  g_assert_false (gtk_widget_has_css_class (GTK_WIDGET (window), "playbox"));

  g_object_set (tab, "tab-status", KGX_PLAYBOX, NULL);
  g_assert_false (gtk_widget_has_css_class (GTK_WIDGET (window), "root"));
  g_assert_false (gtk_widget_has_css_class (GTK_WIDGET (window), "remote"));
  g_assert_true (gtk_widget_has_css_class (GTK_WIDGET (window), "playbox"));

  g_object_set (tab, "tab-status", KGX_REMOTE | KGX_PRIVILEGED, NULL);
  g_assert_true (gtk_widget_has_css_class (GTK_WIDGET (window), "root"));
  g_assert_true (gtk_widget_has_css_class (GTK_WIDGET (window), "remote"));
  g_assert_false (gtk_widget_has_css_class (GTK_WIDGET (window), "playbox"));
}


static void
test_window_get_working_dir (void)
{
  /* TODO: Remove once libadwaita doesn't trigger this */
  G_GNUC_UNUSED g_autoptr (KgxIgnoreDeprecation) ignore =
    kgx_ignore_deprecation_new ("GtkPicture:keep-aspect-ratio", NULL);
  g_autoptr (KgxApplication) app = g_object_new (KGX_TYPE_APPLICATION, NULL);
  g_autoptr (KgxSettings) settings = g_object_new (KGX_TYPE_SETTINGS, NULL);
  g_autoptr (GFile) file_a = g_file_new_for_uri ("https://example.com");
  g_autoptr (GFile) file_b = g_file_new_for_uri ("https://gnome.org");
  g_autoptr (KgxTestTab) tab_a =
    g_object_new (KGX_TYPE_TEST_TAB,
                  "application", app,
                  "tab-path", file_a,
                  NULL);
  g_autoptr (KgxTestTab) tab_b =
    g_object_new (KGX_TYPE_TEST_TAB,
                  "application", app,
                  "tab-path", file_b,
                  NULL);
  g_autoptr (KgxTestTab) tab_c =
    g_object_new (KGX_TYPE_TEST_TAB,
                  "application", app,
                  "tab-path", file_a,
                  NULL);
  g_autoptr (KgxWindow) window = g_object_new (KGX_TYPE_WINDOW, NULL);
  g_autoptr (GFile) res = NULL;

  g_object_ref_sink (tab_a);
  g_object_ref_sink (tab_b);
  g_object_ref_sink (tab_c);

  res = kgx_window_get_working_dir (window);
  g_assert_null (res);

  kgx_window_add_tab (window, KGX_TAB (tab_a));
  kgx_window_add_tab (window, KGX_TAB (tab_b));

  res = kgx_window_get_working_dir (window);
  g_assert_true (g_file_equal (file_b, res));
  g_clear_object (&res);

  kgx_window_add_tab (window, KGX_TAB (tab_c));
  res = kgx_window_get_working_dir (window);
  g_assert_true (g_file_equal (file_a, res));
}


int
main (int argc, char *argv[])
{
  kgx_locale_init (KGX_LOCALE_TEST);

  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func ("/kgx/window/new-destroy", test_window_new_destroy);
  g_test_add_func ("/kgx/window/settings/direct", test_window_settings_direct);
  g_test_add_func ("/kgx/window/search-mode", test_window_search_mode);
  g_test_add_func ("/kgx/window/floating", test_window_floating);
  g_test_add_func ("/kgx/window/translucent/direct", test_window_translucent_direct);
  g_test_add_func ("/kgx/window/translucent/derived", test_window_translucent_derived);
  g_test_add_func ("/kgx/window/realize-unrealize", test_window_realize_unrealize);
  g_test_add_func ("/kgx/window/trigger-breakpoints/empty", test_window_trigger_breakpoints_empty);
  g_test_add_func ("/kgx/window/trigger-breakpoints/with-tab", test_window_trigger_breakpoints_with_tab);
  g_test_add_func ("/kgx/window/add_tab", test_window_add_tab);
  g_test_add_func ("/kgx/window/ringing", test_window_ringing);
  g_test_add_func ("/kgx/window/status", test_window_status);
  g_test_add_func ("/kgx/window/get_working_dir", test_window_get_working_dir);

  return g_test_run ();
}
