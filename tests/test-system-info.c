/* test-system-info.c
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

#include <gio/gio.h>
#include <pango/pango.h>

#include "kgx-test-utils.h"

#include "kgx-system-info.h"


typedef struct {
  GSettings *settings;
} Fixture;


static void
fixture_setup (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (GSettingsSchema) schema = NULL;
  g_auto (GStrv) keys = NULL;

  fixture->settings = g_settings_new ("org.gnome.desktop.interface");

  schema = g_settings_schema_source_lookup (g_settings_schema_source_get_default (),
                                            "org.gnome.desktop.interface",
                                            TRUE);
  keys = g_settings_schema_list_keys (schema);

  for (size_t i = 0; keys[i]; i++) {
    g_settings_reset (fixture->settings, keys[i]);
  }
}


static void
fixture_tear (Fixture *fixture, gconstpointer unused)
{
  g_clear_object (&fixture->settings);
}


static void
test_system_info_new_destroy (Fixture *fixture, gconstpointer unused)
{
  KgxSystemInfo *obj = g_object_new (KGX_TYPE_SYSTEM_INFO, NULL);

  g_assert_finalize_object (obj);
}


static void
test_system_info_monospace_font (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (KgxSystemInfo) obj = NULL;
  g_autoptr (PangoFontDescription) font = NULL;
  g_autoptr (PangoFontDescription) font_two = NULL;

  g_settings_set_string (fixture->settings, "monospace-font-name", "monospace 10");

  obj = g_object_new (KGX_TYPE_SYSTEM_INFO, NULL);

  g_object_get (obj, "monospace-font", &font, NULL);
  g_assert_cmpstr (pango_font_description_get_family (font), ==, "monospace");

  kgx_expect_property_notify (obj, "monospace-font");
  g_settings_set_string (fixture->settings, "monospace-font-name", "extra-space 10");
  kgx_assert_expected_notifies (obj);

  g_object_get (obj, "monospace-font", &font_two, NULL);
  g_assert_cmpstr (pango_font_description_get_family (font_two), ==, "extra-space");
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add ("/kgx/system-info/new-destroy",
              Fixture, NULL,
              fixture_setup,
              test_system_info_new_destroy,
              fixture_tear);
  g_test_add ("/kgx/system-info/monospace-font",
              Fixture, NULL,
              fixture_setup,
              test_system_info_monospace_font,
              fixture_tear);

  return g_test_run ();
}
