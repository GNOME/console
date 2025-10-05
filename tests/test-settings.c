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

#include "kgx-test-utils.h"

#include "kgx-settings.h"


typedef struct {
  GSettings *settings;
  GSettings *interface_settings;
} Fixture;


static const char *schemas[] = {
  KGX_APPLICATION_ID,
  "org.gnome.desktop.interface",
  NULL,
};


static void
fixture_setup (Fixture *fixture, gconstpointer unused)
{
  fixture->settings = g_settings_new (KGX_APPLICATION_ID);
  fixture->interface_settings = g_settings_new ("org.gnome.desktop.interface");

  for (size_t i = 0; schemas[i]; i++) {
    g_autoptr (GSettingsSchema) schema = NULL;
    g_auto (GStrv) keys = NULL;

    schema = g_settings_schema_source_lookup (g_settings_schema_source_get_default (),
                                              schemas[i],
                                              TRUE);
    keys = g_settings_schema_list_keys (schema);

    for (size_t j = 0; keys[j]; j++) {
      g_settings_reset (fixture->settings, keys[j]);
    }
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


static void
test_settings_font_scale_get_set (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (KgxSettings) settings = g_object_new (KGX_TYPE_SETTINGS, NULL);
  double scale = 42.0;

  g_settings_set_double (fixture->settings, "font-scale", 1.5);

  g_object_get (settings, "font-scale", &scale, NULL);
  g_assert_cmpfloat (scale, ==, 1.5);

  kgx_test_property_notify (settings,
                            "font-scale",
                            KGX_FONT_SCALE_MAX,
                            KGX_FONT_SCALE_MAX - 1);

  g_object_get (settings, "font-scale", &scale, NULL);
  g_assert_cmpfloat (scale, ==, KGX_FONT_SCALE_MAX - 1);

  g_assert_cmpfloat (g_settings_get_double (fixture->settings, "font-scale"),
                     ==,
                     KGX_FONT_SCALE_MAX - 1);
}


static void
test_settings_font_scale_increase (Fixture *fixture, gconstpointer unused)
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
test_settings_font_scale_decrease (Fixture *fixture, gconstpointer unused)
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
test_settings_font_scale_reset (Fixture *fixture, gconstpointer unused)
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
test_settings_custom_font (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (KgxSettings) settings = g_object_new (KGX_TYPE_SETTINGS, NULL);
  g_autoptr (PangoFontDescription) font_set = pango_font_description_from_string ("Funky 12");
  g_autoptr (PangoFontDescription) font_two = pango_font_description_from_string ("Other 12");
  g_autoptr (PangoFontDescription) font_got = NULL;
  g_autofree char *custom_font = NULL;

  g_assert_nonnull (font_set);
  g_assert_nonnull (font_two);

  kgx_settings_set_custom_font (settings, font_set);

  font_got = kgx_settings_dup_custom_font (settings);

  g_assert_true (pango_font_description_equal (font_set, font_got));

  g_assert_cmpstr (pango_font_description_get_family (font_got), ==, "Funky");
  g_assert_cmpint (pango_font_description_get_size (font_got), ==, 12 * PANGO_SCALE);

  g_clear_pointer (&font_got, pango_font_description_free);
  g_object_get (settings, "custom-font", &font_got, NULL);

  g_assert_true (pango_font_description_equal (font_set, font_got));

  g_assert_cmpstr (pango_font_description_get_family (font_got), ==, "Funky");
  g_assert_cmpint (pango_font_description_get_size (font_got), ==, 12 * PANGO_SCALE);

  g_object_set (settings, "custom-font", NULL, NULL);

  g_clear_pointer (&font_got, pango_font_description_free);
  font_got = kgx_settings_dup_custom_font (settings);
  custom_font = g_settings_get_string (fixture->settings, "custom-font");

  g_assert_null (font_got);

  g_assert_cmpstr (custom_font, ==, "");

  kgx_test_property_notify (settings, "custom-font", font_set, font_two);

  g_clear_pointer (&custom_font, g_free);
  custom_font = g_settings_get_string (fixture->settings, "custom-font");

  g_assert_cmpstr (custom_font, ==, "Other 12");
}


static void
test_settings_use_system_font (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (KgxSettings) settings = g_object_new (KGX_TYPE_SETTINGS, NULL);
  gboolean result;

  g_settings_set_boolean (fixture->settings, "use-system-font", TRUE);

  g_object_get (settings, "use-system-font", &result, NULL);
  g_assert_true (result);

  g_object_set (settings, "use-system-font", FALSE, NULL);
  g_object_get (settings, "use-system-font", &result, NULL);
  g_assert_false (result);

  g_assert_false (g_settings_get_boolean (fixture->settings, "use-system-font"));

  kgx_test_property_notify (settings, "use-system-font", TRUE, FALSE);
}


static void
test_settings_font_default_is_system (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (KgxSettings) settings = g_object_new (KGX_TYPE_SETTINGS, NULL);
  g_autoptr (PangoFontDescription) font = NULL;

  g_settings_set_string (fixture->interface_settings, "monospace-font-name", "Monospace 20");

  g_object_get (settings, "font", &font, NULL);

  g_assert_cmpstr (pango_font_description_get_family (font), ==, "Monospace");
  g_assert_cmpint (pango_font_description_get_size (font), ==, 20 * PANGO_SCALE);
}


static void
test_settings_font_switch_to_custom (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (KgxSettings) settings = g_object_new (KGX_TYPE_SETTINGS, NULL);
  g_autoptr (PangoFontDescription) font_set = pango_font_description_from_string ("Funky 12");
  g_autoptr (PangoFontDescription) font_got = NULL;

  g_assert_nonnull (font_set);

  kgx_settings_set_custom_font (settings, font_set);

  kgx_expect_property_notify (settings, "use-system-font");
  kgx_expect_property_notify (settings, "font");
  g_settings_set_boolean (fixture->settings, "use-system-font", FALSE);
  kgx_assert_expected_notifies (settings);

  g_object_get (settings, "font", &font_got, NULL);
  g_assert_true (pango_font_description_equal (font_set, font_got));
  g_clear_pointer (&font_got, pango_font_description_free);

  kgx_expect_property_notify (settings, "use-system-font");
  kgx_expect_property_notify (settings, "font");
  g_settings_set_boolean (fixture->settings, "use-system-font", TRUE);
  kgx_assert_expected_notifies (settings);

  g_object_get (settings, "font", &font_got, NULL);
  g_assert_false (pango_font_description_equal (font_set, font_got));

  kgx_expect_no_notify (settings);
  g_settings_set_boolean (fixture->settings, "use-system-font", TRUE);
  kgx_assert_expected_notifies (settings);
}


static void
test_settings_font_custom_is_null (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (KgxSettings) settings = g_object_new (KGX_TYPE_SETTINGS, NULL);
  g_autoptr (PangoFontDescription) font_got = NULL;

  g_settings_set_string (fixture->interface_settings, "monospace-font-name", "System 36");
  g_settings_set_string (fixture->settings, "custom-font", "");
  g_settings_set_boolean (fixture->settings, "use-system-font", FALSE);

  g_object_get (settings, "font", &font_got, NULL);
  g_assert_nonnull (font_got);

  /* When there is no custom font, we get the system font after all */
  g_assert_cmpstr (pango_font_description_get_family (font_got), ==, "System");
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


static void
test_settings_custom_shell (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (KgxSettings) settings = g_object_new (KGX_TYPE_SETTINGS, NULL);

  {
    g_auto (GStrv) shell = kgx_settings_get_shell (settings);

    /* A user shell is only a single entry long */
    g_assert_cmpint (g_strv_length (shell), ==, 1);
  }

  /* Set a custom shell with multiple elements */
  kgx_settings_set_custom_shell (settings,
                                 ((const char *[]) { "/usr/bin/fancy-shell", "with-arg", NULL }));

  {
    g_auto (GStrv) shell = kgx_settings_get_shell (settings);

    /* We should have got the custom shell */
    g_assert_cmpstrv (shell, ((const char *[]) { "/usr/bin/fancy-shell", "with-arg", NULL }));
  }

  /* Clear the shell via KgxSettings */
  kgx_settings_set_custom_shell (settings, NULL);

  {
    g_auto (GStrv) shell = kgx_settings_get_shell (settings);

    /* A user shell is only a single entry long */
    g_assert_cmpint (g_strv_length (shell), ==, 1);
  }

  /* Set a custom shell again */
  kgx_settings_set_custom_shell (settings,
                                 ((const char *[]) { "/usr/bin/different-shell", NULL }));

  {
    g_auto (GStrv) shell = kgx_settings_get_shell (settings);

    /* We should have got the custom shell */
    g_assert_cmpstrv (shell, ((const char *[]) { "/usr/bin/different-shell", NULL }));
  }

  /* Clear the shell via GSettings */
  g_settings_set_strv (fixture->settings, "shell", NULL);

  {
    g_auto (GStrv) shell = kgx_settings_get_shell (settings);

    /* A user shell is only a single entry long */
    g_assert_cmpint (g_strv_length (shell), ==, 1);
  }
}


static struct resolve_test {
  KgxTheme setting;
  gboolean dark;
  KgxTheme result;
} resolve_cases[] = {
  { KGX_THEME_AUTO, FALSE, KGX_THEME_DAY },
  { KGX_THEME_AUTO, TRUE, KGX_THEME_NIGHT },
  { KGX_THEME_DAY, FALSE, KGX_THEME_DAY },
  { KGX_THEME_DAY, TRUE, KGX_THEME_DAY },
  { KGX_THEME_NIGHT, FALSE, KGX_THEME_NIGHT },
  { KGX_THEME_NIGHT, TRUE, KGX_THEME_NIGHT },
  { KGX_THEME_HACKER, FALSE, KGX_THEME_NIGHT },
  { KGX_THEME_HACKER, TRUE, KGX_THEME_NIGHT },
};


static void
test_settings_resolve_theme (Fixture *fixture, gconstpointer user_data)
{
  g_autoptr (KgxSettings) settings = g_object_new (KGX_TYPE_SETTINGS, NULL);
  const struct resolve_test *test = user_data;

  g_settings_set_enum (fixture->settings, "theme", test->setting);

  g_assert_cmpint (kgx_settings_resolve_theme (settings, test->dark),
                   ==,
                   test->result);
}


static void
test_settings_audible_bell (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (KgxSettings) settings = g_object_new (KGX_TYPE_SETTINGS, NULL);
  gboolean audible_bell = TRUE;

  g_settings_set_boolean (fixture->settings, "audible-bell", TRUE);

  kgx_test_property_notify (settings, "audible-bell", FALSE, TRUE);

  g_assert_true (kgx_settings_get_audible_bell (settings));

  g_object_get (settings, "audible-bell", &audible_bell, NULL);
  g_assert_true (audible_bell);

  g_object_set (settings, "audible-bell", FALSE, NULL);
  g_assert_false (kgx_settings_get_audible_bell (settings));
  g_assert_false (g_settings_get_boolean (fixture->settings, "audible-bell"));
}


static void
test_settings_visual_bell (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (KgxSettings) settings = g_object_new (KGX_TYPE_SETTINGS, NULL);
  gboolean visual_bell = TRUE;

  g_settings_set_boolean (fixture->settings, "visual-bell", TRUE);

  kgx_test_property_notify (settings, "visual-bell", FALSE, TRUE);

  g_assert_true (kgx_settings_get_visual_bell (settings));

  g_object_get (settings, "visual-bell", &visual_bell, NULL);
  g_assert_true (visual_bell);

  g_object_set (settings, "visual-bell", FALSE, NULL);
  g_assert_false (kgx_settings_get_visual_bell (settings));
  g_assert_false (g_settings_get_boolean (fixture->settings, "visual-bell"));
}


static void
test_settings_livery (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (KgxSettings) settings = g_object_new (KGX_TYPE_SETTINGS, NULL);
  g_autoptr (KgxPalette) palette =
    kgx_palette_new (&((GdkRGBA) { 0, }),
                     &((GdkRGBA) { 0, }),
                     0.0,
                     0,
                     ((GdkRGBA []) { 0, }));
  g_autoptr (KgxLivery) new_livery = NULL;
  g_autofree char *uuid = g_uuid_string_random ();
  g_autofree char *uuid_from_settings = NULL;
  KgxLivery *livery;

  g_settings_set_string (fixture->settings, "livery", KGX_LIVERY_UUID_KGX);
  livery = kgx_settings_get_livery (settings);
  g_assert_cmpstr (kgx_livery_get_uuid (livery), ==, KGX_LIVERY_UUID_KGX);

  g_settings_set_string (fixture->settings, "livery", KGX_LIVERY_UUID_XTERM);
  livery = kgx_settings_get_livery (settings);
  g_assert_cmpstr (kgx_livery_get_uuid (livery), ==, KGX_LIVERY_UUID_XTERM);

  new_livery = kgx_livery_new (uuid, "Test", palette, palette);

  kgx_settings_set_livery (settings, new_livery);

  uuid_from_settings = g_settings_get_string (fixture->settings, "livery");

  g_assert_cmpstr (uuid_from_settings, ==, uuid);

  kgx_test_property_notify (settings, "livery", NULL, new_livery);
}


static void
test_settings_scrollback_basic (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (KgxSettings) settings = g_object_new (KGX_TYPE_SETTINGS, NULL);
  int64_t limit;
  int lines;

  g_settings_set_boolean (fixture->settings, "ignore-scrollback-limit", FALSE);
  g_settings_set_int64 (fixture->settings, "scrollback-lines", 10000);

  /* With limiting enabled, the allowed lines should be equal to the limit */
  g_object_get (settings, "scrollback-lines", &lines, NULL);
  g_assert_cmpint (lines, ==, 10000);

  g_object_get (settings, "scrollback-limit", &limit, NULL);
  g_assert_cmpint (limit, ==, 10000);

  /* When we change the limit */
  kgx_expect_property_notify (settings, "scrollback-limit");
  kgx_expect_property_notify (settings, "scrollback-lines");
  kgx_settings_set_scrollback_limit (settings, 20000);
  kgx_assert_expected_notifies (settings);

  /* The gsetting should be updated */
  g_assert_cmpint (g_settings_get_int64 (fixture->settings, "scrollback-lines"),
                   ==,
                   20000);

  /* And the allowed lines should be in sync */
  g_object_get (settings, "scrollback-lines", &lines, NULL);
  g_assert_cmpint (lines, ==, 20000);

  /* Now lets have unlimited scrollback */
  kgx_expect_property_notify (settings, "scrollback-lines");
  kgx_expect_property_notify (settings, "ignore-scrollback-limit");
  g_settings_set_boolean (fixture->settings, "ignore-scrollback-limit", TRUE);
  kgx_assert_expected_notifies (settings);

  /* So the lines is now ‘-1’ — infinite */
  g_object_get (settings, "scrollback-lines", &lines, NULL);
  g_assert_cmpint (lines, ==, -1);

  /* But the stored limit should be unchanged */
  g_object_get (settings, "scrollback-limit", &limit, NULL);
  g_assert_cmpint (limit, ==, 20000);

  /* Updating the limit with the limit disabled */
  g_object_set (settings, "scrollback-limit", (int64_t) 5000, NULL);

  /* …the acutal lines value should remain -1 */
  g_object_get (settings, "scrollback-lines", &lines, NULL);
  g_assert_cmpint (lines, ==, -1);

  /* …even as we remember the new limit */
  g_object_get (settings, "scrollback-limit", &limit, NULL);
  g_assert_cmpint (limit, ==, 5000);
}


static struct resolve_lines_test {
  gboolean ignore_limit;
  int64_t limit;
  int result;
} resolve_lines_cases[] = {
  { FALSE, 100000, 100000 },
  { FALSE, G_MAXINT, G_MAXINT },
  { FALSE, G_MAXINT + ((int64_t) 10), G_MAXINT },
  { FALSE, G_MAXINT - ((int64_t) 10), G_MAXINT - 10 },
  { FALSE, G_MININT + ((int64_t) 10), -1 },
  { FALSE, G_MININT - ((int64_t) 10), -1 },
  { TRUE, 100000, -1 },
  { TRUE, G_MAXINT, -1 },
  { TRUE, G_MAXINT + ((int64_t) 10), -1 },
  { TRUE, G_MAXINT - ((int64_t) 10), -1 },
  { TRUE, G_MININT + ((int64_t) 10), -1 },
  { TRUE, G_MININT - ((int64_t) 10), -1 },
};


static void
test_settings_scrollback_resolve (Fixture *fixture, gconstpointer user_data)
{
  g_autoptr (KgxSettings) settings = g_object_new (KGX_TYPE_SETTINGS, NULL);
  const struct resolve_lines_test *test = user_data;
  int lines;

  g_settings_set_boolean (fixture->settings,
                          "ignore-scrollback-limit",
                          test->ignore_limit);
  g_settings_set_int64 (fixture->settings, "scrollback-lines", test->limit);

  g_object_get (settings, "scrollback-lines", &lines, NULL);
  g_assert_cmpint (lines, ==, test->result);
}


static void
test_settings_software_flow_control (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (KgxSettings) settings = g_object_new (KGX_TYPE_SETTINGS, NULL);
  gboolean result = TRUE;

  g_settings_set_boolean (fixture->settings, "software-flow-control", TRUE);

  kgx_test_property_notify (settings, "software-flow-control", FALSE, TRUE);

  g_assert_true (kgx_settings_get_software_flow_control (settings));

  g_object_get (settings, "software-flow-control", &result, NULL);
  g_assert_true (result);

  g_object_set (settings, "software-flow-control", FALSE, NULL);
  g_assert_false (kgx_settings_get_software_flow_control (settings));
  g_assert_false (g_settings_get_boolean (fixture->settings, "software-flow-control"));
}


static void
test_settings_transparency (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (KgxSettings) settings = g_object_new (KGX_TYPE_SETTINGS, NULL);
  gboolean result = TRUE;

  g_settings_set_boolean (fixture->settings, "transparency", TRUE);

  g_object_get (settings, "transparency", &result, NULL);
  g_assert_true (result);

  kgx_test_property_notify (settings, "transparency", FALSE, TRUE);

  g_object_get (settings, "transparency", &result, NULL);
  g_assert_true (result);
}


#define fixtured_test(path, data, func) \
  g_test_add ((path), Fixture, (data), fixture_setup, (func), fixture_tear);


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  fixtured_test ("/kgx/settings/new-destroy", NULL, test_settings_new_destroy);
  fixtured_test ("/kgx/settings/font-scale/get-set", NULL, test_settings_font_scale_get_set);
  fixtured_test ("/kgx/settings/font-scale/increase", NULL, test_settings_font_scale_increase);
  fixtured_test ("/kgx/settings/font-scale/decrease", NULL, test_settings_font_scale_decrease);
  fixtured_test ("/kgx/settings/font-scale/reset", NULL, test_settings_font_scale_reset);
  fixtured_test ("/kgx/settings/custom-font", NULL, test_settings_custom_font);
  fixtured_test ("/kgx/settings/use-system-font", NULL, test_settings_use_system_font);
  fixtured_test ("/kgx/settings/font/default-is-system", NULL, test_settings_font_default_is_system);
  fixtured_test ("/kgx/settings/font/switch-to-custom", NULL, test_settings_font_switch_to_custom);
  fixtured_test ("/kgx/settings/font/custom-is-null", NULL, test_settings_font_custom_is_null);
  fixtured_test ("/kgx/settings/size", NULL, test_settings_size);
  fixtured_test ("/kgx/settings/custom-size", NULL, test_settings_custom_size);
  fixtured_test ("/kgx/settings/custom-shell", NULL, test_settings_custom_shell);

  for (size_t i = 0; i < G_N_ELEMENTS (resolve_cases); i++) {
    g_autofree char *name =
      g_enum_to_string (KGX_TYPE_THEME, resolve_cases[i].setting);
    g_autofree char *path =
      g_strdup_printf ("/kgx/settings/resolve-theme/%s/%s",
                       name,
                       resolve_cases[i].dark ? "dark" : "not-dark");

    fixtured_test (path, &resolve_cases[i], test_settings_resolve_theme);
  }

  fixtured_test ("/kgx/settings/audible-bell", NULL, test_settings_audible_bell);
  fixtured_test ("/kgx/settings/visual-bell", NULL, test_settings_visual_bell);
  fixtured_test ("/kgx/settings/livery", NULL, test_settings_livery);

  fixtured_test ("/kgx/settings/scrollback/basic", NULL, test_settings_scrollback_basic);
  for (size_t i = 0; i < G_N_ELEMENTS (resolve_lines_cases); i++) {
    g_autofree char *path =
      g_strdup_printf ("/kgx/settings/scrollback/resolve/%s/%" G_GINT64_FORMAT,
                       resolve_lines_cases[i].ignore_limit ? "not-limited" : "limited",
                       resolve_lines_cases[i].limit);

    fixtured_test (path, &resolve_lines_cases[i], test_settings_scrollback_resolve);
  }

  fixtured_test ("/kgx/settings/software-flow-control", NULL, test_settings_software_flow_control);
  fixtured_test ("/kgx/settings/transparency", NULL, test_settings_transparency);

  return g_test_run ();
}
