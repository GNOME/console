/* test-spad.c
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

#include "kgx-spad.h"


static void
test_spad_new_destroy (void)
{
  KgxSpad *spad = g_object_new (KGX_TYPE_SPAD, NULL);

  g_object_ref_sink (spad);

  g_assert_finalize_object (spad);
}


static void
test_spad_get_set (void)
{
  g_autoptr (KgxSpad) spad = g_object_new (KGX_TYPE_SPAD, NULL);
  g_autofree char *error_body = NULL;
  g_autofree char *error_content = NULL;
  KgxSpadFlags spad_flags;

  g_object_ref_sink (spad);

  g_object_set (spad,
                "spad-flags", KGX_SPAD_SHOW_REPORT,
                "error-body", "Error Body",
                "error-content", "Error Content",
                NULL);

  g_object_get (spad,
                "spad-flags", &spad_flags,
                "error-body", &error_body,
                "error-content", &error_content,
                NULL);

  g_assert_cmpuint (spad_flags, ==, KGX_SPAD_SHOW_REPORT);
  g_assert_cmpstr (error_body, ==, "Error Body");
  g_assert_cmpstr (error_content, ==, "Error Content");
}


static void
test_spad_build_bundle (void)
{
  g_autoptr (GVariant) bundle = kgx_spad_build_bundle ("Title",
                                                       KGX_SPAD_SHOW_REPORT,
                                                       "Error Body",
                                                       "Error Content",
                                                       NULL);
  g_auto (GVariantDict) dict = G_VARIANT_DICT_INIT (bundle);

  g_assert_true (g_variant_dict_contains (&dict, "title"));
  g_assert_true (g_variant_dict_contains (&dict, "flags"));
  g_assert_true (g_variant_dict_contains (&dict, "error-body"));
  g_assert_true (g_variant_dict_contains (&dict, "error-content"));
  g_assert_false (g_variant_dict_contains (&dict, "error"));
}


static void
test_spad_build_bundle_with_error (void)
{
  g_autoptr (GError) error = g_error_new (G_IO_ERROR,
                                          G_IO_ERROR_BUSY,
                                          "Message");
  g_autoptr (GVariant) bundle = kgx_spad_build_bundle (NULL,
                                                       KGX_SPAD_NONE,
                                                       "Error Body",
                                                       NULL,
                                                       error);
  g_auto (GVariantDict) dict = G_VARIANT_DICT_INIT (bundle);
  const char *error_message = NULL;
  GQuark domain;
  int code;

  g_assert_false (g_variant_dict_contains (&dict, "title"));
  g_assert_true (g_variant_dict_contains (&dict, "flags"));
  g_assert_true (g_variant_dict_contains (&dict, "error-body"));
  g_assert_false (g_variant_dict_contains (&dict, "error-content"));

  g_assert_true (g_variant_dict_lookup (&dict,
                                        "error", "(ui&s)",
                                        &domain,
                                        &code,
                                        &error_message));

  g_assert_cmpuint (domain, ==, G_IO_ERROR);
  g_assert_cmpint (code, ==, G_IO_ERROR_BUSY);
  g_assert_cmpstr (error_message, ==, "Message");
}


static void
test_spad_new_from_bundle (void)
{
  g_autoptr (GVariant) bundle =
    kgx_spad_build_bundle ("No pasarán!",
                           KGX_SPAD_SHOW_REPORT,
                           "Human Rights Violation.",
                           "The Government has disregarded Human Rights.",
                           NULL);
  g_autoptr (AdwDialog) spad = kgx_spad_new_from_bundle (bundle);
  g_autofree char *title = NULL;
  g_autofree char *error_body = NULL;
  g_autofree char *error_content = NULL;
  KgxSpadFlags spad_flags;

  g_object_ref_sink (spad);

  g_object_get (spad,
                "title", &title,
                "spad-flags", &spad_flags,
                "error-body", &error_body,
                "error-content", &error_content,
                NULL);

  g_assert_cmpstr (title, ==, "No pasarán!");
  g_assert_cmpuint (spad_flags, ==, KGX_SPAD_SHOW_REPORT);
  g_assert_cmpstr (error_body, ==, "Human Rights Violation.");
  g_assert_cmpstr (error_content, ==, "The Government has disregarded Human Rights.");
}


static void
test_spad_new_from_bundle_no_title (void)
{
  g_autoptr (GVariant) bundle_1 =
    kgx_spad_build_bundle (NULL,
                           KGX_SPAD_SHOW_REPORT,
                           "I've got no title!",
                           NULL,
                           NULL);
  g_autoptr (GVariant) bundle_2 =
    kgx_spad_build_bundle (" ",
                           KGX_SPAD_SHOW_REPORT,
                           "I've got an empty title!",
                           NULL,
                           NULL);
  g_autoptr (AdwDialog) spad_1 = kgx_spad_new_from_bundle (bundle_1);
  g_autoptr (AdwDialog) spad_2 = kgx_spad_new_from_bundle (bundle_2);
  g_autofree char *title_1 = NULL;
  g_autofree char *title_2 = NULL;

  g_object_ref_sink (spad_1);
  g_object_ref_sink (spad_2);

  g_object_get (spad_1, "title", &title_1, NULL);
  g_object_get (spad_2, "title", &title_2, NULL);

  g_assert_cmpstr (title_1, ==, title_2);
  g_assert_cmpstr (title_1, ==, "Error Details");
  g_assert_cmpstr (title_2, ==, "Error Details");
}


static void
test_spad_new_from_bundle_with_error (void)
{
  g_autoptr (GError) error = g_error_new (G_IO_ERROR,
                                          G_IO_ERROR_AGAIN,
                                          "Rights violated again");
  g_autoptr (GVariant) bundle =
    kgx_spad_build_bundle ("No pasarán!",
                           KGX_SPAD_NONE,
                           "Human Rights Violation.",
                           "The Government has disregarded Human Rights.",
                           error);
  g_autoptr (AdwDialog) spad = kgx_spad_new_from_bundle (bundle);
  g_autofree char *title = NULL;
  g_autofree char *error_body = NULL;
  g_autofree char *error_content = NULL;
  KgxSpadFlags spad_flags;

  g_object_ref_sink (spad);

  g_object_get (spad,
                "title", &title,
                "spad-flags", &spad_flags,
                "error-body", &error_body,
                "error-content", &error_content,
                NULL);

  g_assert_cmpstr (title, ==, "No pasarán!");
  g_assert_cmpuint (spad_flags, ==, KGX_SPAD_NONE);
  g_assert_cmpstr (error_body, ==, "Human Rights Violation.");
  g_assert_cmpstr (error_content,
                   ==,
                   "The Government has disregarded Human Rights.\n\n"
                   "g-io-error-quark (1):\n"
                   "Rights violated again");
}


int
main (int argc, char *argv[])
{
  kgx_locale_init (KGX_LOCALE_TEST);

  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func ("/kgx/spad/new-destroy", test_spad_new_destroy);
  g_test_add_func ("/kgx/spad/get-set", test_spad_get_set);
  g_test_add_func ("/kgx/spad/build_bundle", test_spad_build_bundle);
  g_test_add_func ("/kgx/spad/build_bundle/with_error", test_spad_build_bundle_with_error);
  g_test_add_func ("/kgx/spad/new_from_bundle", test_spad_new_from_bundle);
  g_test_add_func ("/kgx/spad/new_from_bundle/no_title", test_spad_new_from_bundle_no_title);
  g_test_add_func ("/kgx/spad/new_from_bundle/with_error", test_spad_new_from_bundle_with_error);

  return g_test_run ();
}
