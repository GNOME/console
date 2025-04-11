/* test-file-closures.c
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

#include "kgx-file-closures.h"


static void
test_file_as_subtitle_null_when_null (void)
{
  g_assert_null (kgx_file_as_subtitle (NULL, NULL, NULL));
}


static void
test_file_as_subtitle_no_path (void)
{
  g_autoptr (GFile) file = g_file_new_for_uri ("trash://");
  g_autofree char *path = g_file_get_path (file);

  g_assert_null (path);

  g_assert_null (kgx_file_as_subtitle (NULL, file, NULL));
}


static void
test_file_as_subtitle_raw_matches_title (void)
{
  g_autoptr (GFile) file = g_file_new_for_uri ("file:///test/me");
  g_autofree char *path = g_file_get_path (file);

  g_assert_cmpstr (path, ==, "/test/me");

  g_assert_null (kgx_file_as_subtitle (NULL, file, "/test/me"));
}


static void
test_file_as_subtitle_non_utf8 (void)
{
  g_autoptr (GFile) file = g_file_new_for_uri ("file:///test%C0/me");
  g_autofree char *path = g_file_get_path (file);
  g_autofree char *result = kgx_file_as_subtitle (NULL, file, NULL);

  g_assert_cmpstr (path, ==, "/test\xC0/me");
  g_assert_false (g_utf8_validate ("test\xC0", -1, NULL));

  g_assert_cmpstr (result, ==, "file:///test%C0/me");
}


static void
test_file_as_subtitle_non_utf8_in_home (void)
{
  g_autoptr (GFile) file = g_file_new_build_filename (g_get_home_dir (),
                                                      "test\xC0",
                                                      NULL);
  g_autofree char *uri = g_file_get_uri (file);
  g_autofree char *result = kgx_file_as_subtitle (NULL, file, NULL);

  g_assert_cmpstr (result, ==, uri);
}


static void
test_file_as_subtitle_outside_home (void)
{
  g_autoptr (GFile) file = g_file_new_for_uri ("file:///not/in/home");
  g_autofree char *path = g_file_get_path (file);
  g_autofree char *result = kgx_file_as_subtitle (NULL, file, NULL);

  g_assert_cmpstr (path, ==, "/not/in/home");
  g_assert_cmpstr (result, ==, "/not/in/home");

  g_assert_null (kgx_file_as_subtitle (NULL, file, "/not/in/home"));
}


static void
test_file_as_subtitle_is_home (void)
{
  g_autoptr (GFile) file = g_file_new_for_path (g_get_home_dir ());
  g_autofree char *result = kgx_file_as_subtitle (NULL, file, NULL);

  g_assert_cmpstr (result, ==, "~");

  g_assert_null (kgx_file_as_subtitle (NULL, file, "~"));
}


static void
test_file_as_subtitle_in_home (void)
{
  g_autoptr (GFile) file = g_file_new_build_filename (g_get_home_dir (),
                                                      "in",
                                                      "home",
                                                      NULL);
  g_autofree char *result = kgx_file_as_subtitle (NULL, file, NULL);

  g_assert_cmpstr (result, ==, "~/in/home");

  g_assert_null (kgx_file_as_subtitle (NULL, file, "~/in/home"));
}


static void
test_file_as_display_null_when_null (void)
{
  g_assert_null (kgx_file_as_display (NULL, NULL));
}


static void
test_file_as_display_utf8 (void)
{
  g_autoptr (GFile) file = g_file_new_for_uri ("file:///test/me");
  g_autofree char *result = kgx_file_as_display (NULL, file);

  g_assert_cmpstr (result, ==, "/test/me");
}


static void
test_file_as_display_non_utf8 (void)
{
  g_autoptr (GFile) file = g_file_new_for_uri ("file:///test%C0/me");
  g_autofree char *result = kgx_file_as_display (NULL, file);

  /* Hey look, it's an intentional replacement character! */
  g_assert_cmpstr (result, ==, "/test�/me");
}


static void
test_file_as_display_no_path (void)
{
  g_autoptr (GFile) file = g_file_new_for_uri ("trash://");

  g_assert_null (kgx_file_as_display (NULL, file));
}


static void
test_file_as_display_or_uri_null_when_null (void)
{
  g_assert_null (kgx_file_as_display_or_uri (NULL, NULL));
}


static void
test_file_as_display_or_uri_utf8 (void)
{
  g_autoptr (GFile) file = g_file_new_for_uri ("file:///test/me");
  g_autofree char *path = g_file_get_path (file);
  g_autofree char *result = kgx_file_as_display_or_uri (NULL, file);

  g_assert_cmpstr (result, ==, path);
}


static void
test_file_as_display_or_uri_non_utf8 (void)
{
  g_autoptr (GFile) file = g_file_new_for_uri ("file:///test%C0/me");
  g_autofree char *uri = g_file_get_uri (file);
  g_autofree char *result = kgx_file_as_display_or_uri (NULL, file);

  g_assert_cmpstr (result, ==, uri);
}


static void
test_file_as_display_or_uri_no_path (void)
{
  g_autoptr (GFile) file = g_file_new_for_uri ("trash://");
  g_autofree char *uri = g_file_get_uri (file);
  g_autofree char *result = kgx_file_as_display_or_uri (NULL, file);

  g_assert_cmpstr (result, ==, uri);
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/kgx/file-closures/file_as_subtitle/null_when_null", test_file_as_subtitle_null_when_null);
  g_test_add_func ("/kgx/file-closures/file_as_subtitle/no_path", test_file_as_subtitle_no_path);
  g_test_add_func ("/kgx/file-closures/file_as_subtitle/raw_matches_title", test_file_as_subtitle_raw_matches_title);
  g_test_add_func ("/kgx/file-closures/file_as_subtitle/non_utf8", test_file_as_subtitle_non_utf8);
  g_test_add_func ("/kgx/file-closures/file_as_subtitle/non_utf8_in_home", test_file_as_subtitle_non_utf8_in_home);
  g_test_add_func ("/kgx/file-closures/file_as_subtitle/outside_home", test_file_as_subtitle_outside_home);
  g_test_add_func ("/kgx/file-closures/file_as_subtitle/is_home", test_file_as_subtitle_is_home);
  g_test_add_func ("/kgx/file-closures/file_as_subtitle/in_home", test_file_as_subtitle_in_home);

  g_test_add_func ("/kgx/file-closures/file_as_display/null_when_null", test_file_as_display_null_when_null);
  g_test_add_func ("/kgx/file-closures/file_as_display/utf8", test_file_as_display_utf8);
  g_test_add_func ("/kgx/file-closures/file_as_display/non_utf8", test_file_as_display_non_utf8);
  g_test_add_func ("/kgx/file-closures/file_as_display/no_path", test_file_as_display_no_path);

  g_test_add_func ("/kgx/file-closures/file_as_display_or_uri/null_when_null", test_file_as_display_or_uri_null_when_null);
  g_test_add_func ("/kgx/file-closures/file_as_display_or_uri/utf8", test_file_as_display_or_uri_utf8);
  g_test_add_func ("/kgx/file-closures/file_as_display_or_uri/non_utf8", test_file_as_display_or_uri_non_utf8);
  g_test_add_func ("/kgx/file-closures/file_as_display_or_uri/no_path", test_file_as_display_or_uri_no_path);

  return g_test_run ();
}
