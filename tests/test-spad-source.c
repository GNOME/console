/* test-spad-source.c
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

#include "kgx-spad-source.h"


#define KGX_TYPE_TEST_SOURCE (kgx_test_source_get_type ())

G_DECLARE_FINAL_TYPE (KgxTestSource, kgx_test_source, KGX, TEST_SOURCE, GObject)


struct _KgxTestSource {
  GObject parent;
};


G_DEFINE_FINAL_TYPE_WITH_CODE (KgxTestSource, kgx_test_source, G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (KGX_TYPE_SPAD_SOURCE, NULL))


static void
kgx_test_source_class_init (KgxTestSourceClass *klass)
{
}


static void
kgx_test_source_init (KgxTestSource *self)
{
}


static void
test_spad_source_new_destroy (void)
{
  KgxTestSource *source = g_object_new (KGX_TYPE_TEST_SOURCE, NULL);

  g_assert_true (KGX_IS_SPAD_SOURCE (source));

  g_assert_finalize_object (source);
}


struct spad_thrown {
  const char *title;
  gboolean    got_spad;
};


static gboolean
spad_thrown (KgxSpadSource *source,
             const char    *title,
             GVariant      *bundle,
             gpointer       user_data)
{
  g_auto (GVariantDict) dict = G_VARIANT_DICT_INIT (bundle);
  struct spad_thrown *thrown = user_data;

  g_assert_cmpstr (thrown->title, ==, title);

  g_assert_true (g_variant_dict_contains (&dict, "title"));
  g_assert_true (g_variant_dict_contains (&dict, "flags"));
  g_assert_true (g_variant_dict_contains (&dict, "error-body"));
  g_assert_true (g_variant_dict_contains (&dict, "error-content"));
  g_assert_false (g_variant_dict_contains (&dict, "error"));

  thrown->got_spad = TRUE;

  return TRUE;
}


static void
test_spad_source_throw (void)
{
  g_autoptr (KgxSpadSource) source = g_object_new (KGX_TYPE_TEST_SOURCE, NULL);
  struct spad_thrown thrown = {
    .title = "Title",
    .got_spad = FALSE,
  };

  g_signal_connect (source, "spad-thrown", G_CALLBACK (spad_thrown), &thrown);

  g_assert_false (thrown.got_spad);

  kgx_spad_source_throw (source,
                         "Title",
                         KGX_SPAD_SHOW_REPORT,
                         "Error Body",
                         "Error Content",
                         NULL);

  g_assert_true (thrown.got_spad);
}


static void
test_spad_source_throw_unhandled_case_1 (void)
{
  if (g_test_subprocess ()) {
    g_autoptr (KgxSpadSource) source = g_object_new (KGX_TYPE_TEST_SOURCE, NULL);

    kgx_spad_source_throw (source, "Title", KGX_SPAD_NONE, "Error Body", NULL, NULL);

    return;
  }

  g_test_trap_subprocess (NULL, 0, 0);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*unhandled-spad: ‘Title’*");
}


static void
test_spad_source_throw_unhandled_case_2 (void)
{
  if (g_test_subprocess ()) {
    g_autoptr (KgxSpadSource) source = g_object_new (KGX_TYPE_TEST_SOURCE, NULL);

    kgx_spad_source_throw (source,
                           "Oh No",
                           KGX_SPAD_NONE,
                           "Error Body",
                           "Content",
                           NULL);

    return;
  }

  g_test_trap_subprocess (NULL, 0, 0);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*unhandled-spad: ‘Oh No’*");
}


int
main (int argc, char *argv[])
{
  kgx_locale_init (KGX_LOCALE_TEST);

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/kgx/spad-source/new-destroy", test_spad_source_new_destroy);
  g_test_add_func ("/kgx/spad-source/throw", test_spad_source_throw);
  g_test_add_func ("/kgx/spad-source/throw/unhandled/case_1", test_spad_source_throw_unhandled_case_1);
  g_test_add_func ("/kgx/spad-source/throw/unhandled/case_2", test_spad_source_throw_unhandled_case_2);

  return g_test_run ();
}
