/* test-utils.c
 *
 * Copyright 2024 Zander Brown
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

#include "kgx-utils.h"


#define PLAIN_KGX ((const char *[]) { KGX_BIN_NAME, NULL })
#define SH_WRAPPED(...) ((const char *[]) { "/bin/sh", "-c", __VA_ARGS__ })


static struct filter_test {
  const char **argv;
  const char **filtered;
  const char **extracted;
} filter_cases[] = {
  {
    (const char *[]) { KGX_BIN_NAME, "--", "ls", "-al", "/", NULL },
    PLAIN_KGX,
    (const char *[]) { "ls", "-al", "/", NULL },
  },
  {
    (const char *[]) { KGX_BIN_NAME, "-e", "ls", "-al", "/", NULL },
    PLAIN_KGX,
    (const char *[]) { "ls", "-al", "/", NULL },
  },
  {
    (const char *[]) { KGX_BIN_NAME, "--command=ls -al /", NULL },
    PLAIN_KGX,
    SH_WRAPPED ("ls -al /", NULL),
  },
  {
    (const char *[]) { KGX_BIN_NAME, "-e", "ls", "-T", "12", NULL },
    PLAIN_KGX,
    (const char *[]) { "ls", "-T", "12", NULL },
  },
  {
    (const char *[]) { KGX_BIN_NAME, "-e", "ls -al /", NULL },
    PLAIN_KGX,
    SH_WRAPPED ("ls -al /", NULL),
  },
  {
    (const char *[]) { KGX_BIN_NAME, "-T", "Directory listing", "-e", "vim", "/", NULL },
    (const char *[]) { KGX_BIN_NAME, "-T", "Directory listing", NULL },
    (const char *[]) { "vim", "/", NULL },
  },
  {
    (const char *[]) { KGX_BIN_NAME, "--", "xterm", "-T", "Directory listing", "-e", "vim", "/", NULL },
    PLAIN_KGX,
    (const char *[]) { "xterm", "-T", "Directory listing", "-e", "vim", "/", NULL },
  },
  {
    (const char *[]) { KGX_BIN_NAME, "--command=true", NULL },
    PLAIN_KGX,
    (const char *[]) { "/usr/bin/true", NULL },
  },
  {
    (const char *[]) { KGX_BIN_NAME, "--command=foo", "--command=bar", NULL },
    PLAIN_KGX,
    SH_WRAPPED ("bar", NULL),
  },
  {
    (const char *[]) { KGX_BIN_NAME, "-e", "true", NULL },
    PLAIN_KGX,
    (const char *[]) { "/usr/bin/true", NULL },
  },
  {
    (const char *[]) { KGX_BIN_NAME, "--", "true", NULL },
    PLAIN_KGX,
    (const char *[]) { "true", NULL },
  },
  {
    (const char *[]) { KGX_BIN_NAME, "-e", "command xeyes", NULL },
    PLAIN_KGX,
    SH_WRAPPED ("command xeyes", NULL),
  },
  {
    (const char *[]) { KGX_BIN_NAME, "--command=/usr/bin/true", NULL },
    PLAIN_KGX,
    (const char *[]) { "/usr/bin/true", NULL },
  },
  {
    (const char *[]) { KGX_BIN_NAME, "--command=ls *.txt", NULL },
    PLAIN_KGX,
    SH_WRAPPED ("ls *.txt", NULL),
  },
  {
    (const char *[]) { KGX_BIN_NAME, "--command=if [ -e foo ]; then xeyes; fi", NULL },
    PLAIN_KGX,
    SH_WRAPPED ("if [ -e foo ]; then xeyes; fi", NULL),
  },
  {
    (const char *[]) { KGX_BIN_NAME, "sl", NULL },
    (const char *[]) { KGX_BIN_NAME, "sl", NULL },
    NULL,
  },
  {
    (const char *[]) { KGX_BIN_NAME, "--commandbutnot", NULL },
    (const char *[]) { KGX_BIN_NAME, "--commandbutnot", NULL },
    NULL,
  },
  {
    (const char *[]) { NULL },
    (const char *[]) { NULL },
    NULL,
  },
  {
    NULL,
    (const char *[]) { NULL },
    NULL,
  },
};


static void
test_filter_arguments (gconstpointer user_data)
{
  const struct filter_test *test = user_data;
  g_auto (GStrv) filtered = g_strdupv ((GStrv) test->argv);
  g_auto (GStrv) extracted = NULL;
  g_autoptr (GString) str = g_string_new (NULL);
  g_autoptr (GError) error = NULL;

  kgx_filter_arguments (&filtered, &extracted, &error);

  g_assert_no_error (error);

  g_assert_cmpstrv (test->filtered, filtered);
  g_assert_cmpstrv (test->extracted, extracted);
}


static void
test_filter_arguments_both (void)
{
  g_auto (GStrv) filtered = g_strdupv ((GStrv) (const char *[]) { KGX_BIN_NAME, "--command=bash", "--", "zsh", NULL });
  g_auto (GStrv) command = NULL;
  g_autoptr (GError) error = NULL;

  kgx_filter_arguments (&filtered, &command, &error);

  g_assert_error (error, KGX_ARGUMENT_ERROR, KGX_ARGUMENT_ERROR_BOTH);
}


static void
test_filter_arguments_missing (void)
{
  g_auto (GStrv) filtered = g_strdupv ((GStrv) (const char *[]) { KGX_BIN_NAME, "--command", NULL });
  g_auto (GStrv) command = NULL;
  g_autoptr (GError) error = NULL;

  kgx_filter_arguments (&filtered, &command, &error);

  g_assert_error (error, KGX_ARGUMENT_ERROR, KGX_ARGUMENT_ERROR_MISSING);
}


static struct constrained_test {
  const char *input;
  size_t max_len;
  const char *output;
} constrained_cases[] = {
  { "hello world", 5, "hello…" },
  { "hello", 5, "hello" },
};


static void
test_str_constrained_dup (gconstpointer user_data)
{
  const struct constrained_test *test = user_data;
  g_autofree char *result = kgx_str_constrained_dup (test->input, test->max_len);

  g_assert_cmpstr (result, ==, test->output);
  g_assert_cmpint (strlen (result), <=, test->max_len + strlen ("…"));
}


static void
test_str_constrained_append (void)
{
  const size_t len = 10;
  g_autoptr (GString) buffer = g_string_sized_new (len);

  g_assert_cmpstr (buffer->str, ==, "");
  g_assert_cmpint (buffer->len, <=, len + strlen ("…"));

  kgx_str_constrained_append (buffer, "some", len);

  g_assert_cmpstr (buffer->str, ==, "some");
  g_assert_cmpint (buffer->len, <=, len + strlen ("…"));

  kgx_str_constrained_append (buffer, "thing", len);

  g_assert_cmpstr (buffer->str, ==, "something");
  g_assert_cmpint (buffer->len, <=, len + strlen ("…"));

  kgx_str_constrained_append (buffer, " more", len);

  g_assert_cmpstr (buffer->str, ==, "something …");
  g_assert_cmpint (buffer->len, <=, len + strlen ("…"));
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  for (size_t i = 0; i < G_N_ELEMENTS (filter_cases); i++) {
    g_autofree char *path =
      g_strdup_printf ("/kgx/utils/filter_argument/case_%li", i);

    g_test_add_data_func (path, &filter_cases[i], test_filter_arguments);
  }
  g_test_add_func ("/kgx/utils/filter_arguments/both", test_filter_arguments_both);
  g_test_add_func ("/kgx/utils/filter_arguments/missing", test_filter_arguments_missing);
  g_test_add_func ("/kgx/utils/str_constrained_append", test_str_constrained_append);
  for (size_t i = 0; i < G_N_ELEMENTS (constrained_cases); i++) {
    g_autofree char *path =
      g_strdup_printf ("/kgx/utils/str_constrained_dup/case_%li", i);

    g_test_add_data_func (path, &constrained_cases[i], test_str_constrained_dup);
  }

  return g_test_run ();
}
