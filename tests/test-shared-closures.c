/* test-shared-closures.c
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

#include "kgx-shared-closures.h"


static void
test_manager_null_when_null (void)
{
  g_assert_null (kgx_style_manager_for_display (NULL, NULL));
}


static void
test_settings_null_when_null (void)
{
  g_assert_null (kgx_gtk_settings_for_display (NULL, NULL));
}


struct text_or_fallback_test {
  const char *text;
  const char *fallback;
  const char *expected;
} text_or_fallback_cases[] = {
  { NULL, NULL, NULL },
  { " ", "fallback", "fallback" },
  { " ", "  ", "  " },
  { "Title", "  ", "Title" },
  { "Title", NULL, "Title" },
  { NULL, "foo", "foo" },
};


static void
test_text_or_fallback (gconstpointer user_data)
{
  const struct text_or_fallback_test *test = user_data;
  g_autofree char *result = kgx_text_or_fallback (NULL,
                                                  test->text,
                                                  test->fallback);

  g_assert_cmpstr (test->expected, ==, result);
}


static void
test_object_or_fallback (void)
{
  g_autoptr (GObject) obj_a = g_object_new (G_TYPE_OBJECT, NULL);
  g_autoptr (GObject) obj_b = g_object_new (G_TYPE_OBJECT, NULL);
  g_autoptr (GObject) res = NULL;

  res = kgx_object_or_fallback (NULL, obj_a, obj_b);
  g_assert_nonnull (res);
  g_assert_true (res == obj_a);
  g_clear_object (&res);

  res = kgx_object_or_fallback (NULL, NULL, obj_b);
  g_assert_nonnull (res);
  g_assert_true (res == obj_b);
  g_clear_object (&res);

  res = kgx_object_or_fallback (NULL, NULL, NULL);
  g_assert_null (res);

  res = kgx_object_or_fallback (NULL, obj_a, NULL);
  g_assert_nonnull (res);
  g_assert_true (res == obj_a);
  g_clear_object (&res);
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/kgx/shared-closures/manager_null_when_null", test_manager_null_when_null);
  g_test_add_func ("/kgx/shared-closures/settings_null_when_null", test_settings_null_when_null);

  for (size_t i = 0; i < G_N_ELEMENTS (text_or_fallback_cases); i++) {
    g_autofree char *path =
      g_strdup_printf ("/kgx/shared-closures/text_or_fallback/case_%" G_GSIZE_FORMAT, i);

    g_test_add_data_func (path, &text_or_fallback_cases[i], test_text_or_fallback);
  }

  g_test_add_func ("/kgx/shared-closures/object_or_fallback", test_object_or_fallback);

  return g_test_run ();
}
