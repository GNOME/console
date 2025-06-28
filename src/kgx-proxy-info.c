/* kgx-proxy-info.c
 *
 * Copyright 2021-2025 Zander Brown
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

#include "kgx-utils.h"

#include "kgx-proxy-info.h"


/**
 * KgxProxyInfo:
 *
 * #KgxProxyInfo maps org.gnome.system.proxy to environmental variables
 * when launching new sessions
 *
 * Note that whilst changes to system settings are tracked, they _cannot_ be
 * applied existing terminals
 *
 * Only manual proxies are supported
 */
struct _KgxProxyInfo {
  GObject            parent_instance;

  GSettings         *settings;
  GSettings         *protocols[4];

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

  g_clear_object (&self->settings);

  for (size_t i = 0; i < G_N_ELEMENTS (self->protocols); i++) {
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
static inline void
env_set_both (KgxProxyInfo *self,
              const char   *key,
              char         *value)
{
  if (!kgx_str_non_empty (value)) {
    return;
  }

  g_hash_table_replace (self->environ,
                        g_strdup (key),
                        g_strdup (value));
  g_hash_table_replace (self->environ,
                        g_ascii_strup (key, -1),
                        value);
}


static inline void
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

  if (!kgx_str_non_empty (host) || port == 0) {
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
                               kgx_str_non_empty (user) ? user : NULL,
                               kgx_str_non_empty (password) ? password : NULL,
                               NULL,
                               host,
                               port,
                               "",
                               NULL,
                               NULL);

  env_set_both (self, proxy_variable[protocol], g_uri_to_string (uri));
}


static inline void
handle_ignored (KgxProxyInfo *self)
{
  g_autoptr (GString) value = NULL;
  g_auto (GStrv) hosts = NULL;

  g_settings_get (self->settings, "ignore-hosts", "^as", &hosts);
  if (hosts == NULL) {
    return;
  }

  value = g_string_sized_new (100);
  for (size_t i = 0; kgx_str_non_empty (hosts[i]); ++i) {
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
manual_settings_changed (G_GNUC_UNUSED GSettings  *settings_ignored,
                         G_GNUC_UNUSED const char *key,
                         KgxProxyInfo             *self)
{
  g_hash_table_remove_all (self->environ);

  for (size_t i = 0; i < G_N_ELEMENTS (self->protocols); i++) {
    handle_protocol (self, i);
  }

  handle_ignored (self);
}


static void
proxy_settings_changed (G_GNUC_UNUSED GSettings  *settings_ignored,
                        G_GNUC_UNUSED const char *key,
                        KgxProxyInfo             *self)
{
  self->mode = g_settings_get_enum (self->settings, "mode");

  switch (self->mode) {
    case G_DESKTOP_PROXY_MODE_MANUAL:
      manual_settings_changed (NULL, NULL, self);
      break;
    case G_DESKTOP_PROXY_MODE_AUTO:
      g_info ("proxy-info: we don't know how to handle ‘auto’ proxies");
      break;
    case G_DESKTOP_PROXY_MODE_NONE:
      break;
    default:
      g_return_if_reached ();
  }
}


static inline GSettings *
get_protocol_settings (KgxProxyInfo *self, const char *protocol)
{
  GSettings *settings = g_settings_get_child (self->settings, protocol);

  g_signal_connect_object (settings,
                           "changed", G_CALLBACK (manual_settings_changed), self,
                           G_CONNECT_DEFAULT);

  return settings;
}


static void
kgx_proxy_info_init (KgxProxyInfo *self)
{
  self->environ = g_hash_table_new_full (g_str_hash,
                                         g_str_equal,
                                         g_free,
                                         g_free);

  self->settings = g_settings_new ("org.gnome.system.proxy");
  g_signal_connect_object (self->settings,
                           "changed", G_CALLBACK (proxy_settings_changed), self,
                           G_CONNECT_DEFAULT);

  self->protocols[HTTP] = get_protocol_settings (self, "http");
  self->protocols[HTTPS] = get_protocol_settings (self, "https");
  self->protocols[FTP] = get_protocol_settings (self, "ftp");
  self->protocols[SOCKS] = get_protocol_settings (self, "socks");

  proxy_settings_changed (self->settings, NULL, self);
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

  /* Just do nothing when we don't know what to do */
  if (self->mode != G_DESKTOP_PROXY_MODE_MANUAL) {
    return;
  }

  g_hash_table_iter_init (&iter, self->environ);
  while (g_hash_table_iter_next (&iter,
                                 (gpointer *) &key,
                                 (gpointer *) &value)) {
    *env = g_environ_setenv (*env, key, value, TRUE);
  }
}
