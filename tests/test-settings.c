/* test-settings.c
 *
 * Copyright 2022-2025 Zander Brown
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

#include "kgx-settings.h"


typedef struct {
  GSettings *settings;
} Fixture;


static void
fixture_setup (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (GSettingsSchema) schema = NULL;
  g_auto (GStrv) keys = NULL;

  fixture->settings = g_settings_new (KGX_APPLICATION_ID);
  schema = g_settings_schema_source_lookup (g_settings_schema_source_get_default (),
                                            KGX_APPLICATION_ID,
                                            FALSE);
  keys = g_settings_schema_list_keys (schema);

  for (int i = 0; keys[i]; i++) {
    g_settings_reset (fixture->settings, keys[i]);
  }
}


static void
fixture_tear (Fixture *fixture, gconstpointer unused)
{
  g_clear_object (&fixture->settings);
}


static void
test_settings_new_destroy (Fixture *fixture, gconstpointer unused)
{
  KgxSettings *settings = g_object_new (KGX_TYPE_SETTINGS, NULL);

  g_assert_finalize_object (settings);
}


static const char *prop_notified = NULL;


static void
got_notify (GObject *self, GParamSpec *pspec, gpointer data)
{
  prop_notified = g_param_spec_get_name (pspec);
}


static void
test_settings_set_scale (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (KgxSettings) settings = g_object_new (KGX_TYPE_SETTINGS, NULL);

  g_signal_connect (settings, "notify", G_CALLBACK (got_notify), NULL);

  prop_notified = NULL;
  g_object_set (settings, "font-scale", KGX_FONT_SCALE_MAX, NULL);
  g_assert_cmpstr (prop_notified, ==, "font-scale");

  prop_notified = NULL;
  g_object_set (settings, "font-scale", KGX_FONT_SCALE_MAX, NULL);
  g_assert_cmpstr (prop_notified, ==, NULL);

  prop_notified = NULL;
  g_object_set (settings, "font-scale", KGX_FONT_SCALE_MAX - 1, NULL);
  g_assert_cmpstr (prop_notified, ==, "font-scale");
}


static void
test_settings_increase_font (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (KgxSettings) settings = g_object_new (KGX_TYPE_SETTINGS, NULL);
  double scale = -1.0;
  gboolean can_scale = FALSE;

  g_object_get (settings,
                "font-scale", &scale,
                "scale-can-increase", &can_scale,
                NULL);
  g_assert_cmpfloat_with_epsilon (scale, KGX_FONT_SCALE_DEFAULT, DBL_EPSILON);
  g_assert_true (can_scale);

  kgx_settings_increase_scale (settings);

  g_object_get (settings,
                "font-scale", &scale,
                "scale-can-increase", &can_scale,
                NULL);
  g_assert_cmpfloat_with_epsilon (scale, KGX_FONT_SCALE_DEFAULT + 0.1, DBL_EPSILON);
  g_assert_true (can_scale);

  g_object_set (settings, "font-scale", KGX_FONT_SCALE_MAX, NULL);

  kgx_settings_increase_scale (settings);

  g_object_get (settings,
                "font-scale", &scale,
                "scale-can-increase", &can_scale,
                NULL);
  g_assert_cmpfloat_with_epsilon (scale, KGX_FONT_SCALE_MAX, DBL_EPSILON);
  g_assert_false (can_scale);

  kgx_settings_decrease_scale (settings);

  g_object_get (settings,
                "font-scale", &scale,
                "scale-can-increase", &can_scale,
                NULL);
  g_assert_cmpfloat_with_epsilon (scale, KGX_FONT_SCALE_MAX - 0.1, DBL_EPSILON);
  g_assert_true (can_scale);
}


static void
test_settings_decrease_font (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (KgxSettings) settings = g_object_new (KGX_TYPE_SETTINGS, NULL);
  double scale = -1.0;
  gboolean can_scale = FALSE;

  g_object_get (settings,
                "font-scale", &scale,
                "scale-can-decrease", &can_scale,
                NULL);
  g_assert_cmpfloat_with_epsilon (scale, KGX_FONT_SCALE_DEFAULT, DBL_EPSILON);
  g_assert_true (can_scale);

  kgx_settings_decrease_scale (settings);

  g_object_get (settings,
                "font-scale", &scale,
                "scale-can-decrease", &can_scale,
                NULL);
  g_assert_cmpfloat_with_epsilon (scale, KGX_FONT_SCALE_DEFAULT - 0.1, DBL_EPSILON);
  g_assert_true (can_scale);

  g_object_set (settings, "font-scale", KGX_FONT_SCALE_MIN, NULL);

  kgx_settings_decrease_scale (settings);

  g_object_get (settings,
                "font-scale", &scale,
                "scale-can-decrease", &can_scale,
                NULL);
  g_assert_cmpfloat_with_epsilon (scale, KGX_FONT_SCALE_MIN, DBL_EPSILON);
  g_assert_false (can_scale);

  kgx_settings_increase_scale (settings);

  g_object_get (settings,
                "font-scale", &scale,
                "scale-can-decrease", &can_scale,
                NULL);
  g_assert_cmpfloat_with_epsilon (scale, KGX_FONT_SCALE_MIN + 0.1, DBL_EPSILON);
  g_assert_true (can_scale);
}


static void
test_settings_reset_font (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (KgxSettings) settings = g_object_new (KGX_TYPE_SETTINGS, NULL);
  double scale = 30;

  g_object_get (settings, "font-scale", &scale, NULL);
  g_assert_cmpfloat_with_epsilon (scale, 1.0, DBL_EPSILON);

  kgx_settings_decrease_scale (settings);
  kgx_settings_reset_scale (settings);

  g_object_get (settings, "font-scale", &scale, NULL);
  g_assert_cmpfloat_with_epsilon (scale, 1.0, DBL_EPSILON);
}


static void
test_settings_size (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (KgxSettings) settings = g_object_new (KGX_TYPE_SETTINGS, NULL);
  int width = 42;
  int height = 42;
  gboolean maximised = TRUE;

  kgx_settings_get_size (settings, &width, &height, &maximised);

  g_assert_cmpint (width, ==, -1);
  g_assert_cmpint (height, ==, -1);
  g_assert_false (maximised);
}


static void
test_settings_custom_size (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (KgxSettings) settings = g_object_new (KGX_TYPE_SETTINGS, NULL);
  int width = 42;
  int height = 42;
  gboolean maximised = FALSE;

  kgx_settings_set_custom_size (settings, 200, 100, TRUE);
  kgx_settings_get_size (settings, &width, &height, &maximised);

  g_assert_cmpint (width, ==, 200);
  g_assert_cmpint (height, ==, 100);
  g_assert_true (maximised);
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add ("/kgx/settings/new-destroy",
              Fixture, NULL,
              fixture_setup,
              test_settings_new_destroy,
              NULL);
  g_test_add ("/kgx/settings/font/set-scale",
              Fixture, NULL,
              fixture_setup,
              test_settings_set_scale,
              NULL);
  g_test_add ("/kgx/settings/font/increase",
              Fixture, NULL,
              fixture_setup,
              test_settings_increase_font,
              NULL);
  g_test_add ("/kgx/settings/font/decrease",
              Fixture, NULL,
              fixture_setup,
              test_settings_decrease_font,
              NULL);
  g_test_add ("/kgx/settings/font/reset",
              Fixture, NULL,
              fixture_setup,
              test_settings_reset_font,
              fixture_tear);
  g_test_add ("/kgx/settings/size",
              Fixture, NULL,
              fixture_setup,
              test_settings_size,
              fixture_tear);
  g_test_add ("/kgx/settings/custom-size",
              Fixture, NULL,
              fixture_setup,
              test_settings_custom_size,
              fixture_tear);

  return g_test_run ();
}
