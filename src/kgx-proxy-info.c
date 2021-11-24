/* kgx-proxy-info.c
 *
 * Copyright 2021 Zander Brown
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

/**
 * SECTION:kgx-proxy-info
 * @short_description: An object with proxy details
 * @title: KgxProxyInfo 
 *
 * #KgxProxyInfo maps org.gnome.system.proxy to environmental variables
 * when launching new sessions
 * 
 * Note that whilst changes to system settings are tracked, they _cannot_ be
 * applied existing terminals
 * 
 * Only manual proxies are supported
 */

#include "kgx-config.h"

#include <gio/gio.h>
#include <gdesktop-enums.h>

#include "kgx-proxy-info.h"


struct _KgxProxyInfo {
  GObject            parent_instance;

  GSettings         *settings;
  GSettings         *protocols[4];
  gulong             changed_handler[4];

  GDesktopProxyMode  mode;

  GHashTable        *environ;
};


static char *proxy_scheme[] = {
  "http",
  "http",
  "http",
  "socks",
};


static char *proxy_variable[] = {
  "http_proxy",
  "https_proxy",
  "ftp_proxy",
  "all_proxy",
};


typedef enum {
  HTTP = 0,
  HTTPS,
  FTP,
  SOCKS,
} Protocol;


G_DEFINE_TYPE (KgxProxyInfo, kgx_proxy_info, G_TYPE_OBJECT)


static void
kgx_proxy_info_dispose (GObject *object)
{
  KgxProxyInfo *self = KGX_PROXY_INFO (object);

  G_STATIC_ASSERT (G_N_ELEMENTS (self->protocols) ==
                   G_N_ELEMENTS (self->changed_handler));

  g_clear_object (&self->settings);

  for (int i = 0; i < G_N_ELEMENTS (self->protocols); i++) {
    g_clear_signal_handler (&self->changed_handler[i], self->protocols[i]);
    g_clear_object (&self->protocols[i]);
  }

  g_clear_pointer (&self->environ, g_hash_table_unref);

  G_OBJECT_CLASS (kgx_proxy_info_parent_class)->dispose (object);
}


static void
kgx_proxy_info_class_init (KgxProxyInfoClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = kgx_proxy_info_dispose;
}


/**
 * env_set_both:
 * @self: the #KgxProxyInfo
 * @key: (transfer none): the lower case key name
 * @value: (transfer full): the value to set
 * 
 * Set both upper and lower case variants of @key to @value
 */
static void
env_set_both (KgxProxyInfo *self,
              const char   *key,
              char         *value)
{
  if (!value)
    return;

  g_hash_table_replace (self->environ,
                        g_strdup (key), 
                        g_strdup (value));
  g_hash_table_replace (self->environ,
                        g_ascii_strup (key, -1),
                        value);
}


static void
handle_protocol (KgxProxyInfo *self,
                 Protocol      protocol)
{
  g_autoptr (GUri) uri = NULL;
  g_autofree char *host = NULL;
  g_autofree char *user = NULL;
  g_autofree char *password = NULL;
  int port = -1;
  GSettings *settings = self->protocols[protocol];

  host = g_settings_get_string (settings, "host");
  port = g_settings_get_int (settings, "port");

  if (!host || !host[0] || port == 0) {
    g_hash_table_remove (self->environ, proxy_variable[protocol]);
    return;
  }

  if (G_UNLIKELY (protocol == HTTP &&
                  g_settings_get_boolean (settings,
                                          "use-authentication"))) {

    user = g_settings_get_string (settings, "authentication-user");
    password = g_settings_get_string (settings, "authentication-password");
  }

  uri = g_uri_build_with_user (G_URI_FLAGS_NONE,
                               proxy_scheme[protocol],
                               user && user[0] ? user : NULL,
                               password && password[0] ? password : NULL,
                               NULL,
                               host,
                               port,
                               "",
                               NULL,
                               NULL);

  env_set_both (self, proxy_variable[protocol], g_uri_to_string (uri));
}


static void
handle_ignored (KgxProxyInfo *self)
{
  g_autoptr (GString) value = NULL;
  g_auto (GStrv) hosts = NULL;

  g_settings_get (self->settings, "ignore-hosts", "^as", &hosts);
  if (hosts == NULL) {
    return;
  }

  value = g_string_sized_new (100);
  for (int i = 0; hosts[i] != NULL; ++i) {
    if (i > 0) {
      g_string_append_c (value, ',');
    }
    g_string_append (value, hosts[i]);
  }

  env_set_both (self,
                "no_proxy",
                g_string_free (g_steal_pointer (&value), FALSE));
}


static void
manual_settings_changed (GSettings *settings, const char *key, KgxProxyInfo *self)
{
  g_hash_table_remove_all (self->environ);

  for (int i = 0; i < G_N_ELEMENTS (self->protocols); i++) {
    handle_protocol (self, i);
  }

  handle_ignored (self);
}


static void
proxy_settings_changed (GSettings *settings, const char *key, KgxProxyInfo *self)
{
  GDesktopProxyMode mode = g_settings_get_enum (settings, "mode");

  if (mode == self->mode) {
    return;
  }

  switch (mode) {
    case G_DESKTOP_PROXY_MODE_MANUAL:
      for (int i = 0; i < G_N_ELEMENTS (self->changed_handler); i++) {
        self->changed_handler[i] =
          g_signal_connect (self->protocols[i],
                            "changed", G_CALLBACK (manual_settings_changed),
                            self);
      }
      manual_settings_changed (NULL, NULL, self);
      break;
    case G_DESKTOP_PROXY_MODE_AUTO:
      g_info ("Can't handle auto proxy");
    case G_DESKTOP_PROXY_MODE_NONE:
      for (int i = 0; i < G_N_ELEMENTS (self->changed_handler); i++) {
        g_clear_signal_handler (&self->changed_handler[i], self->protocols[i]);
      }
      g_hash_table_remove_all (self->environ);
      break;
    default:
      g_return_if_reached ();
  }
}


static void
kgx_proxy_info_init (KgxProxyInfo *self)
{
  self->settings = g_settings_new ("org.gnome.system.proxy");

  self->protocols[HTTP] = g_settings_get_child (self->settings, "http");
  self->protocols[HTTPS] = g_settings_get_child (self->settings, "https");
  self->protocols[FTP] = g_settings_get_child (self->settings, "ftp");
  self->protocols[SOCKS] = g_settings_get_child (self->settings, "socks");

  self->environ = g_hash_table_new_full (g_str_hash,
                                         g_str_equal,
                                         g_free,
                                         g_free);

  g_signal_connect (self->settings,
                    "changed", G_CALLBACK (proxy_settings_changed),
                    self);
  proxy_settings_changed (self->settings, NULL, self);
}


/**
 * kgx_proxy_info_get_default:
 * 
 * Returns: (transfer none): the #KgxProxyInfo singleton
 */
KgxProxyInfo *
kgx_proxy_info_get_default (void)
{ 
  static KgxProxyInfo *instance;

  if (instance == NULL) {
    instance = g_object_new (KGX_TYPE_PROXY_INFO, NULL);
    g_object_add_weak_pointer (G_OBJECT (instance), (gpointer *) &instance);
  }

  return instance;
}


/**
 * kgx_proxy_info_apply_to_environ:
 * @self: the #KgxProxyInfo
 * @env: (out caller-allocates): an environment to modify
 * 
 * Add the users current proxy settings to @env
 */
void
kgx_proxy_info_apply_to_environ (KgxProxyInfo   *self,
                                 char         ***env)
{
  GHashTableIter iter;
  char *key, *value;

  g_return_if_fail (KGX_IS_PROXY_INFO (self));

  g_hash_table_iter_init (&iter, self->environ);
  while (g_hash_table_iter_next (&iter,
                                 (gpointer *) &key,
                                 (gpointer *) &value)) {
    *env = g_environ_setenv (*env, key, value, TRUE);
  }
}
