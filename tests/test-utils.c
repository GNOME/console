/* test-utils.c
 *
 * Copyright 2024-2025 Zander Brown
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

#include "kgx-enums.h"
#include "kgx-train.h"

#include "kgx-utils.h"


#define KGX_TYPE_TEST_OBJECT (kgx_test_object_get_type ())

G_DECLARE_FINAL_TYPE (KgxTestObject, kgx_test_object, KGX, TEST_OBJECT, GObject)


struct _KgxTestObject {
  GObject parent;

  gboolean a_bool;
  int64_t a_int64;
  char *a_string;
  guint a_flags;
};


G_DEFINE_FINAL_TYPE (KgxTestObject, kgx_test_object, G_TYPE_OBJECT)


enum {
  PROP_0,
  PROP_A_BOOL,
  PROP_A_INT64,
  PROP_A_STRING,
  PROP_A_FLAGS,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };


static void
kgx_test_object_dispose (GObject *object)
{
  KgxTestObject *self = KGX_TEST_OBJECT (object);

  g_clear_pointer (&self->a_string, g_free);
}


static void
kgx_test_object_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  KgxTestObject *self = KGX_TEST_OBJECT (object);

  switch (property_id) {
    case PROP_A_BOOL:
      g_value_set_boolean (value, self->a_bool);
      break;
    case PROP_A_INT64:
      g_value_set_int64 (value, self->a_int64);
      break;
    case PROP_A_STRING:
      g_value_set_string (value, self->a_string);
      break;
    case PROP_A_FLAGS:
      g_value_set_flags (value, self->a_flags);
      break;
    KGX_INVALID_PROP (object, property_id, pspec);
  }
}


static void
kgx_test_object_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  KgxTestObject *self = KGX_TEST_OBJECT (object);

  switch (property_id) {
    case PROP_A_BOOL:
      kgx_set_boolean_prop (object, pspec, &self->a_bool, value);
      break;
    case PROP_A_INT64:
      kgx_set_int64_prop (object, pspec, &self->a_int64, value);
      break;
    case PROP_A_STRING:
      kgx_set_str_prop (object, pspec, &self->a_string, value);
      break;
    case PROP_A_FLAGS:
      kgx_set_flags_prop (object, pspec, &self->a_flags, value);
      break;
    KGX_INVALID_PROP (object, property_id, pspec);
  }
}


static void
kgx_test_object_class_init (KgxTestObjectClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = kgx_test_object_dispose;
  object_class->get_property = kgx_test_object_get_property;
  object_class->set_property = kgx_test_object_set_property;

  pspecs[PROP_A_BOOL] =
    g_param_spec_boolean ("a-bool", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_A_INT64] =
    g_param_spec_int64 ("a-int64", NULL, NULL,
                        G_MININT64, G_MAXINT64, 0,
                        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_A_STRING] =
    g_param_spec_string ("a-string", NULL, NULL,
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_A_FLAGS] =
    g_param_spec_flags ("a-flags", NULL, NULL,
                        KGX_TYPE_STATUS,
                        KGX_NONE,
                        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);
}


static void
kgx_test_object_init (KgxTestObject *self)
{
}


static const char *prop_notified = FALSE;


static void
got_notify (GObject *self, GParamSpec *pspec, gpointer data)
{
  prop_notified = g_param_spec_get_name (pspec);
}


static void
test_set_boolean (void)
{
  g_autoptr (KgxTestObject) obj = g_object_new (KGX_TYPE_TEST_OBJECT, NULL);
  gboolean res;

  g_signal_connect (obj, "notify::a-bool", G_CALLBACK (got_notify), NULL);

  g_object_get (obj, "a-bool", &res, NULL);
  g_assert_false (res);

  prop_notified = NULL;
  g_object_set (obj, "a-bool", TRUE, NULL);
  g_assert_cmpstr (prop_notified, ==, "a-bool");

  g_object_get (obj, "a-bool", &res, NULL);
  g_assert_true (res);

  prop_notified = NULL;
  g_object_set (obj, "a-bool", TRUE, NULL);
  g_assert_null (prop_notified);

  g_object_get (obj, "a-bool", &res, NULL);
  g_assert_true (res);

  prop_notified = NULL;
  g_object_set (obj, "a-bool", FALSE, NULL);
  g_assert_cmpstr (prop_notified, ==, "a-bool");

  g_object_get (obj, "a-bool", &res, NULL);
  g_assert_false (res);
}


static void
test_set_int64 (void)
{
  g_autoptr (KgxTestObject) obj = g_object_new (KGX_TYPE_TEST_OBJECT, NULL);
  int64_t res_a, res_b, res_c;

  g_signal_connect (obj, "notify::a-int64", G_CALLBACK (got_notify), NULL);

  g_object_get (obj, "a-int64", &res_a, NULL);
  g_assert_cmpint (res_a, ==, 0);

  prop_notified = NULL;
  g_object_set (obj, "a-int64", (int64_t) 1234, NULL);
  g_assert_cmpstr (prop_notified, ==, "a-int64");

  g_object_get (obj, "a-int64", &res_a, NULL);
  g_assert_cmpint (res_a, ==, 1234);

  prop_notified = NULL;
  g_object_set (obj, "a-int64", (int64_t) 1234, NULL);
  g_assert_null (prop_notified);

  g_object_get (obj, "a-int64", &res_b, NULL);
  g_assert_cmpint (res_b, ==, 1234);

  g_assert_cmpint (res_a, ==, res_b);

  prop_notified = NULL;
  g_object_set (obj, "a-int64", (int64_t) 789, NULL);
  g_assert_cmpstr (prop_notified, ==, "a-int64");

  g_object_get (obj, "a-int64", &res_c, NULL);
  g_assert_cmpint (res_c, ==, 789);

  g_assert_cmpint (res_c, !=, res_b);
}


static void
test_set_str (void)
{
  g_autoptr (KgxTestObject) obj = g_object_new (KGX_TYPE_TEST_OBJECT, NULL);
  g_autofree char *res_a = NULL;
  g_autofree char *res_b = NULL;
  g_autofree char *res_c = NULL;

  g_signal_connect (obj, "notify::a-string", G_CALLBACK (got_notify), NULL);

  g_object_get (obj, "a-string", &res_a, NULL);
  g_assert_null (res_a);

  prop_notified = NULL;
  g_object_set (obj, "a-string", "trans rights", NULL);
  g_assert_cmpstr (prop_notified, ==, "a-string");

  g_object_get (obj, "a-string", &res_a, NULL);
  g_assert_cmpstr (res_a, ==, "trans rights");

  prop_notified = NULL;
  g_object_set (obj, "a-string", "trans rights", NULL);
  g_assert_null (prop_notified);

  g_object_get (obj, "a-string", &res_b, NULL);
  g_assert_cmpstr (res_b, ==, "trans rights");

  g_assert_cmpstr (res_a, ==, res_b);

  prop_notified = NULL;
  g_object_set (obj, "a-string", "are human rights", NULL);
  g_assert_cmpstr (prop_notified, ==, "a-string");

  g_object_get (obj, "a-string", &res_c, NULL);
  g_assert_cmpstr (res_c, ==, "are human rights");

  g_assert_cmpstr (res_c, !=, res_b);
}


static void
test_set_flags (void)
{
  g_autoptr (KgxTestObject) obj = g_object_new (KGX_TYPE_TEST_OBJECT, NULL);
  guint res_a, res_b, res_c;

  g_signal_connect (obj, "notify::a-flags", G_CALLBACK (got_notify), NULL);

  g_object_get (obj, "a-flags", &res_a, NULL);
  g_assert_cmpint (res_a, ==, KGX_NONE);

  prop_notified = NULL;
  g_object_set (obj, "a-flags", KGX_PRIVILEGED | KGX_REMOTE, NULL);
  g_assert_cmpstr (prop_notified, ==, "a-flags");

  g_object_get (obj, "a-flags", &res_a, NULL);
  g_assert_cmpint (res_a, ==, KGX_PRIVILEGED | KGX_REMOTE);

  prop_notified = NULL;
  g_object_set (obj, "a-flags", KGX_PRIVILEGED | KGX_REMOTE, NULL);
  g_assert_null (prop_notified);

  g_object_get (obj, "a-flags", &res_b, NULL);
  g_assert_cmpint (res_b, ==, KGX_PRIVILEGED | KGX_REMOTE);

  g_assert_cmpint (res_a, ==, res_b);

  prop_notified = NULL;
  g_object_set (obj, "a-flags", KGX_PRIVILEGED, NULL);
  g_assert_cmpstr (prop_notified, ==, "a-flags");

  g_object_get (obj, "a-flags", &res_c, NULL);
  g_assert_cmpint (res_c, ==, KGX_PRIVILEGED);

  g_assert_cmpint (res_c, !=, res_b);
}


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
    (const char *[]) { TRUE_BIN_PATH, NULL },
  },
  {
    (const char *[]) { KGX_BIN_NAME, "--command=foo", "--command=bar", NULL },
    PLAIN_KGX,
    SH_WRAPPED ("bar", NULL),
  },
  {
    (const char *[]) { KGX_BIN_NAME, "-e", "true", NULL },
    PLAIN_KGX,
    (const char *[]) { TRUE_BIN_PATH, NULL },
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


struct parse_test {
  const char *text;
  double expected;
  gboolean should_pass;
} parse_cases[] = {
  { "12.4", 12.4, TRUE },
  { "12,4", 12.4, TRUE },
  { "%12.4", 12.4, TRUE },
  { "12,4%", 12.4, TRUE },
  { "%١٢.٣٤٥", 12.345, TRUE },
  { ".45", 0.45, TRUE },
  { " %%.67 %", 0.67, TRUE },
  { "a12", 0.0, FALSE },
  { "12a", 0.0, FALSE },
  { "1.a", 0.0, FALSE },
  { "1.2.3", 0.0, FALSE },
  { "-1", 0.0, FALSE },
  { "", 0.0, FALSE },
};


static void
test_parse_percentage (gconstpointer user_data)
{
  const struct parse_test *test = user_data;
  double result = 0.0;
  gboolean good = kgx_parse_percentage (test->text, &result);

  if (test->should_pass) {
    g_assert_true (good);
  } else {
    g_assert_false (good);
    g_assert_cmpfloat (result, ==, test->expected);
  }
}


struct non_empty_test {
  const char *text;
  gboolean expected;
} non_empty_cases[] = {
  { NULL, FALSE },
  { "", FALSE },
  { " ", FALSE },
  { "\t", FALSE },
  { "hello", TRUE },
  { " world", TRUE },
};


static void
test_str_non_empty (gconstpointer user_data)
{
  const struct non_empty_test *test = user_data;
  gboolean result = kgx_str_non_empty (test->text);

  if (test->expected) {
    g_assert_true (result);
  } else {
    g_assert_false (result);
  }
}


struct non_local_test {
  const char *scheme;
  const char *host;
  const char *path;
  gboolean expected;
};


static void
test_uri_is_non_local_file (gconstpointer user_data)
{
  const struct non_local_test *test = user_data;
  g_autoptr (GUri) uri = g_uri_build (G_URI_FLAGS_NONE, test->scheme, NULL, test->host, -1, test->path, NULL, NULL);
  gboolean result = kgx_uri_is_non_local_file (uri);

  if (test->expected) {
    g_assert_true (result);
  } else {
    g_assert_false (result);
  }
}


int
main (int argc, char *argv[])
{
  struct non_local_test non_local_cases[] = {
    { "file", g_get_host_name (), "foo", FALSE },
    { "file", "localhost", "bar", FALSE },
    { "file", "", "/baz", FALSE },
    { "file", "hopefully-not-the-host", "whoops", TRUE },
    { "https", "gnome.org", "", FALSE },
    { "man", "", "false", FALSE },
  };

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/kgx/utils/set_boolean", test_set_boolean);
  g_test_add_func ("/kgx/utils/set_int64", test_set_int64);
  g_test_add_func ("/kgx/utils/set_str", test_set_str);
  g_test_add_func ("/kgx/utils/set_flags", test_set_flags);

  for (size_t i = 0; i < G_N_ELEMENTS (filter_cases); i++) {
    g_autofree char *path =
      g_strdup_printf ("/kgx/utils/filter_argument/case_%" G_GSIZE_FORMAT, i);

    g_test_add_data_func (path, &filter_cases[i], test_filter_arguments);
  }
  g_test_add_func ("/kgx/utils/filter_arguments/both", test_filter_arguments_both);
  g_test_add_func ("/kgx/utils/filter_arguments/missing", test_filter_arguments_missing);
  g_test_add_func ("/kgx/utils/str_constrained_append", test_str_constrained_append);
  for (size_t i = 0; i < G_N_ELEMENTS (constrained_cases); i++) {
    g_autofree char *path =
      g_strdup_printf ("/kgx/utils/str_constrained_dup/case_%" G_GSIZE_FORMAT, i);

    g_test_add_data_func (path, &constrained_cases[i], test_str_constrained_dup);
  }

  for (size_t i = 0; i < G_N_ELEMENTS (non_empty_cases); i++) {
    g_autofree char *path =
      g_strdup_printf ("/kgx/utils/str_non_empty/case_%" G_GSIZE_FORMAT, i);

    g_test_add_data_func (path, &non_empty_cases[i], test_str_non_empty);
  }

  for (size_t i = 0; i < G_N_ELEMENTS (parse_cases); i++) {
    g_autofree char *path =
      g_strdup_printf ("/kgx/utils/parse_percentage/case_%" G_GSIZE_FORMAT, i);

    g_test_add_data_func (path, &parse_cases[i], test_parse_percentage);
  }

  for (size_t i = 0; i < G_N_ELEMENTS (non_local_cases); i++) {
    g_autofree char *path =
      g_strdup_printf ("/kgx/utils/uri_is_non_local_file/case_%" G_GSIZE_FORMAT, i);

    g_test_add_data_func (path, &non_local_cases[i], test_uri_is_non_local_file);
  }

  return g_test_run ();
}
