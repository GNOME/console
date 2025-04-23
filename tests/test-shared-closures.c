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

#include "kgx-settings.h"
#include "kgx-train.h"

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


static void
test_enum_is (void)
{
  g_assert_true (kgx_enum_is (NULL, KGX_THEME_DAY, KGX_THEME_DAY));
  g_assert_true (kgx_enum_is (NULL, KGX_THEME_NIGHT, KGX_THEME_NIGHT));

  g_assert_false (kgx_enum_is (NULL, KGX_THEME_NIGHT, KGX_THEME_DAY));
  g_assert_false (kgx_enum_is (NULL, KGX_THEME_DAY, KGX_THEME_NIGHT));
}


static void
test_flags_includes (void)
{
  g_assert_true (kgx_flags_includes (NULL, KGX_REMOTE, KGX_REMOTE));
  g_assert_true (kgx_flags_includes (NULL, KGX_PRIVILEGED, KGX_PRIVILEGED));
  g_assert_true (kgx_flags_includes (NULL,
                                     KGX_REMOTE | KGX_PRIVILEGED,
                                     KGX_REMOTE));
  g_assert_true (kgx_flags_includes (NULL,
                                     KGX_REMOTE | KGX_PRIVILEGED,
                                     KGX_PRIVILEGED));
  g_assert_true (kgx_flags_includes (NULL,
                                     KGX_REMOTE | KGX_PRIVILEGED,
                                     KGX_REMOTE | KGX_PRIVILEGED));

  g_assert_false (kgx_flags_includes (NULL, KGX_REMOTE, KGX_PRIVILEGED));
  g_assert_false (kgx_flags_includes (NULL, KGX_PRIVILEGED, KGX_REMOTE));
  g_assert_false (kgx_flags_includes (NULL,
                                      KGX_REMOTE,
                                      KGX_REMOTE | KGX_PRIVILEGED));
  g_assert_false (kgx_flags_includes (NULL,
                                      KGX_PRIVILEGED,
                                      KGX_REMOTE | KGX_PRIVILEGED));
  g_assert_false (kgx_flags_includes (NULL,
                                      KGX_NONE,
                                      KGX_REMOTE | KGX_PRIVILEGED));
}


static void
test_bool_and (void)
{
  /* Nice 'lil complete truth table */
  g_assert_true (kgx_bool_and (NULL, TRUE, TRUE));
  g_assert_false (kgx_bool_and (NULL, TRUE, FALSE));
  g_assert_false (kgx_bool_and (NULL, FALSE, TRUE));
  g_assert_false (kgx_bool_and (NULL, FALSE, FALSE));
}


struct format_percentage_test {
  double      value;
  const char *expected;
} format_percentage_cases[] = {
  { 0.0, "0%" },
  { 1.0, "100%" },
  { 2.55, "255%" },
  { 2.556, "256%" },
  { 2.554, "255%" },
  { 0.5, "50%" },
  { -0.5, "-50%" },
};


static void
test_format_percentage (gconstpointer user_data)
{
  const struct format_percentage_test *test = user_data;
  g_autofree char *result = kgx_format_percentage (NULL, test->value);

  g_assert_cmpstr (result, ==, test->expected);
}


static void
test_text_as_variant (void)
{
  g_autoptr (GVariant) variant = kgx_text_as_variant (NULL, "Hey!");

  g_assert_nonnull (variant);

  g_assert_cmpstr (g_variant_get_string (variant, NULL),
                   ==,
                   "Hey!");

  g_assert_null (kgx_text_as_variant (NULL, NULL));
}


static void
test_text_non_empty (void)
{
  /* We already have plenty of tests for kgx_str_non_empty, so just a quick
   * check nothing is totally off  */

  g_assert_false (kgx_text_non_empty (NULL, NULL));
  g_assert_false (kgx_str_non_empty (NULL));

  g_assert_true (kgx_text_non_empty (NULL, "Considering the Universal Declaration of Human Rights"));
  g_assert_true (kgx_str_non_empty ("Considering the Universal Declaration of Human Rights"));
}


struct decoration_layout_is_inverted_test {
  const char *decoration_layout;
  gboolean    expected;
} decoration_layout_is_inverted_cases[] = {
  { NULL, FALSE },
  { "", FALSE },
  { ":", FALSE },
  { ":close", FALSE },
  { "close:close", FALSE },
  { "close:close,close", FALSE },
  { "close,close:close", TRUE },
  { "close:", TRUE },
  { "close", TRUE },
};


static void
test_decoration_layout_is_inverted (gconstpointer user_data)
{
  const struct decoration_layout_is_inverted_test *test = user_data;
  gboolean result = kgx_decoration_layout_is_inverted (NULL, test->decoration_layout);

  if (test->expected) {
    g_assert_true (result);
  } else {
    g_assert_false (result);
  }
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
  g_test_add_func ("/kgx/shared-closures/enum_is", test_enum_is);
  g_test_add_func ("/kgx/shared-closures/flags_includes", test_flags_includes);
  g_test_add_func ("/kgx/shared-closures/bool_and", test_bool_and);

  for (size_t i = 0; i < G_N_ELEMENTS (format_percentage_cases); i++) {
    g_autofree char *path =
      g_strdup_printf ("/kgx/shared-closures/format_percentage/case_%" G_GSIZE_FORMAT, i);

    g_test_add_data_func (path, &format_percentage_cases[i], test_format_percentage);
  }

  g_test_add_func ("/kgx/shared-closures/text_as_variant", test_text_as_variant);
  g_test_add_func ("/kgx/shared-closures/text_non_empty", test_text_non_empty);

  for (size_t i = 0; i < G_N_ELEMENTS (decoration_layout_is_inverted_cases); i++) {
    g_autofree char *path =
      g_strdup_printf ("/kgx/shared-closures/decoration_layout_is_inverted/case_%" G_GSIZE_FORMAT, i);

    g_test_add_data_func (path, &decoration_layout_is_inverted_cases[i], test_decoration_layout_is_inverted);
  }

  return g_test_run ();
}
