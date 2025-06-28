/* test-proxy-info.c
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
#include <gdesktop-enums.h>

#include "kgx-proxy-info.h"


typedef struct {
  GSettings *settings;
} Fixture;


static const char *schemas[] = {
  "org.gnome.system.proxy",
  "org.gnome.system.proxy.http",
  "org.gnome.system.proxy.https",
  "org.gnome.system.proxy.ftp",
  "org.gnome.system.proxy.socks",
  NULL,
};


static void
fixture_setup (Fixture *fixture, gconstpointer unused)
{
  fixture->settings = g_settings_new ("org.gnome.system.proxy");

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


typedef struct { const char *key; const char *value; } ExpectedPair;

#define ENV_PAIR(name, value) (&((ExpectedPair) { (name), (value) }))


static inline void
assert_env_contains (GStrv actual, ExpectedPair *expected[])
{
  for (size_t i = 0; expected[i]; i++) {
    const char *value = g_environ_getenv (actual, expected[i]->key);

    g_assert_cmpstr (value, ==, expected[i]->value);
  }
}


static void
test_proxy_info_new_destroy (Fixture *fixture, gconstpointer unused)
{
  KgxProxyInfo *obj = g_object_new (KGX_TYPE_PROXY_INFO, NULL);

  g_assert_finalize_object (obj);
}


static void
test_proxy_info_apply_none (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (KgxProxyInfo) obj = NULL;
  g_auto (GStrv) env = NULL;

  g_settings_set_enum (fixture->settings, "mode", G_DESKTOP_PROXY_MODE_NONE);

  obj = g_object_new (KGX_TYPE_PROXY_INFO, NULL);

  kgx_proxy_info_apply_to_environ (obj, &env);

  g_assert_cmpstrv (env, NULL);
}


static void
test_proxy_info_apply_auto (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (KgxProxyInfo) obj = NULL;
  g_auto (GStrv) env = NULL;

  g_settings_set_enum (fixture->settings, "mode", G_DESKTOP_PROXY_MODE_AUTO);

  obj = g_object_new (KGX_TYPE_PROXY_INFO, NULL);

  kgx_proxy_info_apply_to_environ (obj, &env);

  /* We don't know how to handle auto */
  g_assert_cmpstrv (env, NULL);
}


static void
test_proxy_info_apply_manual_http (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (KgxProxyInfo) obj = NULL;
  g_autoptr (GSettings) http = NULL;
  g_auto (GStrv) env = NULL;
  ExpectedPair *expected[] = {
    ENV_PAIR ("http_proxy", "http://example.com:123"),
    ENV_PAIR ("HTTP_PROXY", "http://example.com:123"),
    ENV_PAIR ("no_proxy", "localhost,127.0.0.0"),
    ENV_PAIR ("NO_PROXY", "localhost,127.0.0.0"),
    NULL,
  };

  g_settings_set_enum (fixture->settings, "mode", G_DESKTOP_PROXY_MODE_MANUAL);
  g_settings_set_strv (fixture->settings,
                       "ignore-hosts",
                       ((const char *[]) { "localhost", "127.0.0.0", NULL}));

  http = g_settings_get_child (fixture->settings, "http");
  g_settings_set_string (http, "host", "example.com");
  g_settings_set_int (http, "port", 123);

  obj = g_object_new (KGX_TYPE_PROXY_INFO, NULL);

  kgx_proxy_info_apply_to_environ (obj, &env);

  assert_env_contains (env, expected);
}


static void
test_proxy_info_apply_manual_http_auth (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (KgxProxyInfo) obj = NULL;
  g_autoptr (GSettings) http = NULL;
  g_auto (GStrv) env = NULL;
  g_auto (GStrv) env_auth = NULL;
  ExpectedPair *expected[] = {
    ENV_PAIR ("http_proxy", "http://example.com:123"),
    ENV_PAIR ("HTTP_PROXY", "http://example.com:123"),
    ENV_PAIR ("no_proxy", "localhost,127.0.0.0"),
    ENV_PAIR ("NO_PROXY", "localhost,127.0.0.0"),
    NULL,
  };
  ExpectedPair *expected_auth[] = {
    ENV_PAIR ("http_proxy", "http://user:pass@example.com:123"),
    ENV_PAIR ("HTTP_PROXY", "http://user:pass@example.com:123"),
    ENV_PAIR ("no_proxy", "localhost,127.0.0.0"),
    ENV_PAIR ("NO_PROXY", "localhost,127.0.0.0"),
    NULL,
  };

  g_settings_set_enum (fixture->settings, "mode", G_DESKTOP_PROXY_MODE_MANUAL);
  g_settings_set_strv (fixture->settings,
                       "ignore-hosts",
                       ((const char *[]) { "localhost", "127.0.0.0", NULL}));

  http = g_settings_get_child (fixture->settings, "http");
  g_settings_set_string (http, "host", "example.com");
  g_settings_set_int (http, "port", 123);
  g_settings_set_boolean (http, "use-authentication", FALSE);
  g_settings_set_string (http, "authentication-user", "user");
  g_settings_set_string (http, "authentication-password", "pass");

  obj = g_object_new (KGX_TYPE_PROXY_INFO, NULL);

  kgx_proxy_info_apply_to_environ (obj, &env);

  assert_env_contains (env, expected);

  g_settings_set_boolean (http, "use-authentication", TRUE);

  kgx_proxy_info_apply_to_environ (obj, &env_auth);

  assert_env_contains (env_auth, expected_auth);
}


static void
test_proxy_info_apply_manual_https (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (KgxProxyInfo) obj = NULL;
  g_autoptr (GSettings) http = NULL;
  g_auto (GStrv) env = NULL;
  ExpectedPair *expected[] = {
    ENV_PAIR ("https_proxy", "http://example.com:123"),
    ENV_PAIR ("HTTPS_PROXY", "http://example.com:123"),
    ENV_PAIR ("no_proxy", "localhost,127.0.0.0"),
    ENV_PAIR ("NO_PROXY", "localhost,127.0.0.0"),
    NULL,
  };

  g_settings_set_enum (fixture->settings, "mode", G_DESKTOP_PROXY_MODE_MANUAL);
  g_settings_set_strv (fixture->settings,
                       "ignore-hosts",
                       ((const char *[]) { "localhost", "127.0.0.0", NULL}));

  http = g_settings_get_child (fixture->settings, "https");
  g_settings_set_string (http, "host", "example.com");
  g_settings_set_int (http, "port", 123);

  obj = g_object_new (KGX_TYPE_PROXY_INFO, NULL);

  kgx_proxy_info_apply_to_environ (obj, &env);

  assert_env_contains (env, expected);
}


static void
test_proxy_info_apply_manual_ftp (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (KgxProxyInfo) obj = NULL;
  g_autoptr (GSettings) http = NULL;
  g_auto (GStrv) env = NULL;
  ExpectedPair *expected[] = {
    ENV_PAIR ("ftp_proxy", "http://example.com:123"),
    ENV_PAIR ("FTP_PROXY", "http://example.com:123"),
    ENV_PAIR ("no_proxy", "localhost,127.0.0.0"),
    ENV_PAIR ("NO_PROXY", "localhost,127.0.0.0"),
    NULL,
  };

  g_settings_set_enum (fixture->settings, "mode", G_DESKTOP_PROXY_MODE_MANUAL);
  g_settings_set_strv (fixture->settings,
                       "ignore-hosts",
                       ((const char *[]) { "localhost", "127.0.0.0", NULL}));

  http = g_settings_get_child (fixture->settings, "ftp");
  g_settings_set_string (http, "host", "example.com");
  g_settings_set_int (http, "port", 123);

  obj = g_object_new (KGX_TYPE_PROXY_INFO, NULL);

  kgx_proxy_info_apply_to_environ (obj, &env);

  assert_env_contains (env, expected);
}


static void
test_proxy_info_apply_manual_socks (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (KgxProxyInfo) obj = NULL;
  g_autoptr (GSettings) http = NULL;
  g_auto (GStrv) env = NULL;
  ExpectedPair *expected[] = {
    ENV_PAIR ("all_proxy", "socks://example.com:123"),
    ENV_PAIR ("ALL_PROXY", "socks://example.com:123"),
    ENV_PAIR ("no_proxy", "localhost,127.0.0.0"),
    ENV_PAIR ("NO_PROXY", "localhost,127.0.0.0"),
    NULL,
  };

  g_settings_set_enum (fixture->settings, "mode", G_DESKTOP_PROXY_MODE_MANUAL);
  g_settings_set_strv (fixture->settings,
                       "ignore-hosts",
                       ((const char *[]) { "localhost", "127.0.0.0", NULL}));

  http = g_settings_get_child (fixture->settings, "socks");
  g_settings_set_string (http, "host", "example.com");
  g_settings_set_int (http, "port", 123);

  obj = g_object_new (KGX_TYPE_PROXY_INFO, NULL);

  kgx_proxy_info_apply_to_environ (obj, &env);

  assert_env_contains (env, expected);
}


static void
test_proxy_info_apply_mode_changed (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (KgxProxyInfo) obj = NULL;
  g_autoptr (GSettings) http = NULL;
  g_auto (GStrv) initial_env = NULL;
  g_auto (GStrv) manual_env = NULL;
  g_auto (GStrv) manual_env_modified = NULL;
  g_auto (GStrv) final_env = NULL;
  ExpectedPair *manual_expected[] = {
    ENV_PAIR ("http_proxy", "http://example.com:123"),
    ENV_PAIR ("HTTP_PROXY", "http://example.com:123"),
    ENV_PAIR ("no_proxy", "localhost,127.0.0.0"),
    ENV_PAIR ("NO_PROXY", "localhost,127.0.0.0"),
    NULL,
  };
  ExpectedPair *manual_expected_modified[] = {
    ENV_PAIR ("http_proxy", "http://example.com:456"),
    ENV_PAIR ("HTTP_PROXY", "http://example.com:456"),
    ENV_PAIR ("no_proxy", "localhost,127.0.0.0"),
    ENV_PAIR ("NO_PROXY", "localhost,127.0.0.0"),
    NULL,
  };

  /* Start with no proxy */
  g_settings_set_enum (fixture->settings, "mode", G_DESKTOP_PROXY_MODE_NONE);
  g_settings_set_strv (fixture->settings,
                       "ignore-hosts",
                       ((const char *[]) { "localhost", "127.0.0.0", NULL}));

  http = g_settings_get_child (fixture->settings, "http");
  g_settings_set_string (http, "host", "example.com");
  g_settings_set_int (http, "port", 123);
  g_settings_set_boolean (http, "use-authentication", FALSE);
  g_settings_set_string (http, "authentication-user", "user");
  g_settings_set_string (http, "authentication-password", "pass");

  obj = g_object_new (KGX_TYPE_PROXY_INFO, NULL);

  kgx_proxy_info_apply_to_environ (obj, &initial_env);

  g_assert_cmpstrv (initial_env, NULL);

  /* Enable proxy */
  g_settings_set_enum (fixture->settings, "mode", G_DESKTOP_PROXY_MODE_MANUAL);

  kgx_proxy_info_apply_to_environ (obj, &manual_env);

  assert_env_contains (manual_env, manual_expected);

  /* Tweak proxy */
  g_settings_set_int (http, "port", 456);

  kgx_proxy_info_apply_to_environ (obj, &manual_env_modified);

  assert_env_contains (manual_env_modified, manual_expected_modified);

  /* Back to no proxy */
  g_settings_set_enum (fixture->settings, "mode", G_DESKTOP_PROXY_MODE_NONE);

  kgx_proxy_info_apply_to_environ (obj, &final_env);

  g_assert_cmpstrv (final_env, NULL);
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add ("/kgx/proxy-info/new-destroy",
              Fixture, NULL,
              fixture_setup,
              test_proxy_info_new_destroy,
              fixture_tear);
  g_test_add ("/kgx/proxy-info/apply/none",
              Fixture, NULL,
              fixture_setup,
              test_proxy_info_apply_none,
              fixture_tear);
  g_test_add ("/kgx/proxy-info/apply/auto",
              Fixture, NULL,
              fixture_setup,
              test_proxy_info_apply_auto,
              fixture_tear);
  g_test_add ("/kgx/proxy-info/apply/manual-http",
              Fixture, NULL,
              fixture_setup,
              test_proxy_info_apply_manual_http,
              fixture_tear);
  g_test_add ("/kgx/proxy-info/apply/manual-http-auth",
              Fixture, NULL,
              fixture_setup,
              test_proxy_info_apply_manual_http_auth,
              fixture_tear);
  g_test_add ("/kgx/proxy-info/apply/manual-https",
              Fixture, NULL,
              fixture_setup,
              test_proxy_info_apply_manual_https,
              fixture_tear);
  g_test_add ("/kgx/proxy-info/apply/manual-ftp",
              Fixture, NULL,
              fixture_setup,
              test_proxy_info_apply_manual_ftp,
              fixture_tear);
  g_test_add ("/kgx/proxy-info/apply/manual-socks",
              Fixture, NULL,
              fixture_setup,
              test_proxy_info_apply_manual_socks,
              fixture_tear);
  g_test_add ("/kgx/proxy-info/apply/mode-changed",
              Fixture, NULL,
              fixture_setup,
              test_proxy_info_apply_mode_changed,
              fixture_tear);

  return g_test_run ();
}
