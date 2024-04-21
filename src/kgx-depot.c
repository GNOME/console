/* kgx-depot.c
 *
 * Copyright 2021-2024 Zander Brown
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
 *
 * Loosly based on work from fp-vte-util.c:
 *      Copyright 2019 Christian Hergert <chergert@redhat.com>
 *      SPDX-License-Identifier: (MIT OR Apache-2.0)
 */

#include "kgx-config.h"

#include <termios.h>
#include <unistd.h>

#include "kgx-proxy-info.h"
#include "kgx-settings.h"
#include "kgx-utils.h"
#include "kgx-watcher.h"

#include "kgx-depot.h"


struct _KgxDepot {
  GObject parent;

  KgxWatcher *watcher;
  GType train_type;
  KgxProxyInfo *proxy_info;
};


G_DEFINE_FINAL_TYPE (KgxDepot, kgx_depot, G_TYPE_OBJECT)


enum {
  PROP_0,
  PROP_WATCHER,
  PROP_TRAIN_TYPE,
  PROP_PROXY_INFO,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };


static void
kgx_depot_dispose (GObject *object)
{
  KgxDepot *self = KGX_DEPOT (object);

  g_clear_object (&self->watcher);
  g_clear_object (&self->proxy_info);

  G_OBJECT_CLASS (kgx_depot_parent_class)->dispose (object);
}


static void
kgx_depot_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  KgxDepot *self = KGX_DEPOT (object);
  GType new_type = G_TYPE_INVALID;

  switch (property_id) {
    case PROP_WATCHER:
      if (g_set_object (&self->watcher, g_value_get_object (value))) {
        g_object_notify_by_pspec (object, pspec);
      }
      break;
    case PROP_TRAIN_TYPE:
      new_type = g_value_get_gtype (value);
      if (G_LIKELY (new_type != self->train_type)) {
        self->train_type = new_type;
        g_object_notify_by_pspec (object, pspec);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
kgx_depot_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  KgxDepot *self = KGX_DEPOT (object);

  switch (property_id) {
    case PROP_WATCHER:
      g_value_set_object (value, self->watcher);
      break;
    case PROP_TRAIN_TYPE:
      g_value_set_gtype (value, self->train_type);
      break;
    case PROP_PROXY_INFO:
      g_value_set_object (value, self->proxy_info);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
kgx_depot_class_init (KgxDepotClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = kgx_depot_dispose;
  object_class->set_property = kgx_depot_set_property;
  object_class->get_property = kgx_depot_get_property;

  pspecs[PROP_WATCHER] =
    g_param_spec_object ("watcher", NULL, NULL,
                         KGX_TYPE_WATCHER,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_TRAIN_TYPE] =
    g_param_spec_gtype ("train-type", NULL, NULL,
                        KGX_TYPE_TRAIN,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_PROXY_INFO] =
    g_param_spec_object ("proxy-info", NULL, NULL,
                         KGX_TYPE_PROXY_INFO,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);
}


static void
kgx_depot_init (KgxDepot *self)
{
  self->train_type = G_TYPE_INVALID;
  self->proxy_info = g_object_new (KGX_TYPE_PROXY_INFO, NULL);
}


struct _ChildData {
  gboolean disable_sfc;
};


KGX_DEFINE_DATA (ChildData, child_data)


static void
child_data_cleanup (ChildData *self)
{
}


static inline void
disable_sfc (void)
{
  struct termios attrs;

  if (G_UNLIKELY (tcgetattr (STDOUT_FILENO, &attrs) != 0)) {
    g_debug ("Unable to fetch attributes");
    return;
  }

  attrs.c_iflag &= ~(IXON | IXOFF);

  if (G_UNLIKELY (tcsetattr (STDOUT_FILENO, TCSANOW, &attrs) != 0)) {
    g_debug ("Unable to store attributes");
    return;
  }
}


static void
child_setup (gpointer user_data)
{
  ChildData *data = user_data;

  if (G_LIKELY (data->disable_sfc)) {
    disable_sfc ();
  }
}


struct _SpawnData {
  GStrv envv;
  GStrv argv;
};


KGX_DEFINE_DATA (SpawnData, spawn_data)


static void
spawn_data_cleanup (SpawnData *self)
{
  g_clear_pointer (&self->envv, g_strfreev);
  g_clear_pointer (&self->argv, g_strfreev);
}


static void
got_train (GObject      *source,
           GAsyncResult *result,
           gpointer      user_data)
{
  g_autoptr (GTask) task = user_data;
  g_autoptr (GError) error = NULL;
  g_autoptr (KgxTrain) train = NULL;
  KgxDepot *self = g_task_get_source_object (task);

  train = KGX_TRAIN (g_async_initable_new_finish (G_ASYNC_INITABLE (source),
                                                  result,
                                                  &error));
  if (error) {
    g_task_return_error (task, g_steal_pointer (&error));
    return;
  }

  kgx_watcher_watch (self->watcher, train);

  g_task_return_pointer (task, g_steal_pointer (&train), g_object_unref);
}


static void
did_spawn (GObject      *source,
           GAsyncResult *result,
           gpointer      user_data)
{
  g_autoptr (GTask) task = user_data;
  g_autoptr (GError) error = NULL;
  g_autofree char *basename = NULL;
  g_autofree char *tag = NULL;
  KgxDepot *self = g_task_get_source_object (task);
  SpawnData *data = kgx_task_get_spawn_data (task);
  GCancellable *cancellable = g_task_get_cancellable (task);
  GPid child_pid;

  g_return_if_fail (VTE_IS_PTY (source));
  g_return_if_fail (G_IS_ASYNC_RESULT (result));

  if (!vte_pty_spawn_finish (VTE_PTY (source), result, &child_pid, &error)) {
    g_task_return_error (task, g_steal_pointer (&error));
    return;
  }

  if (g_task_return_error_if_cancelled (task)) {
    return;
  }

  basename = g_path_get_basename (data->argv[0]);
  tag = g_filename_to_utf8 (basename, -1, NULL, NULL, &error);
  if (error) {
    g_set_str (&tag, "tab");
  }

  g_async_initable_new_async (self->train_type,
                              G_PRIORITY_HIGH_IDLE,
                              cancellable,
                              got_train, g_steal_pointer (&task),
                              "pid", child_pid,
                              "tag", tag,
                              NULL);
}


/**
 * kgx_depot_spawn:
 * @settings: the active #KgxSettings
 * @pty: a #VtePty
 * @working_directory: (nullable): the name of a directory the command should
 *   start in, or NULL to use the current working directory.
 * @argv: (array zero-terminated=1) (element-type filename): child's argument
 *   vector.
 * @env: (nullable) (array zero-terminated=1) (element-type filename): a list
 *   of environment variables to be added to the environment before starting
 *   the process, or NULL.
 * @cancellable: (nullable): a #GCancellable, or %NULL
 * @callback: a callback to execute upon completion
 * @user_data: user data for @callback
 *
 * Creates a new process that uses #VtePty for stdin/stdout/stderr.
 *
 * See kgx_depot_spawn_finish() to complete the request.
 */
void
kgx_depot_spawn (KgxDepot                 *self,
                 KgxSettings              *settings,
                 VtePty                   *pty,
                 const char        *const  working_directory,
                 const char *const *const  argv,
                 const char *const *const  env,
                 GCancellable             *cancellable,
                 GAsyncReadyCallback       callback,
                 gpointer                  user_data)
{
  g_autoptr (GTask) task = g_task_new (self, cancellable, callback, user_data);
  g_autoptr (ChildData) child_data = child_data_alloc ();
  g_autoptr (SpawnData) spawn_data = spawn_data_alloc ();
  GCancellable *task_cancellable = g_task_get_cancellable (task);
  const char *resolved_working_directory;
  GStrv resolved_argv;
  GStrv resolved_envv;

  g_task_set_source_tag (task, kgx_depot_spawn);

  g_return_if_fail (KGX_IS_DEPOT (self));
  g_return_if_fail (KGX_IS_SETTINGS (settings));
  g_return_if_fail (VTE_IS_PTY (pty));

  resolved_working_directory =
    working_directory ? working_directory : g_get_home_dir ();

  if (G_LIKELY (argv == NULL)) {
    spawn_data->argv = kgx_settings_get_shell (settings);
  } else {
    spawn_data->argv = g_strdupv ((char **) argv);
  }
  resolved_argv = spawn_data->argv;

  if (G_UNLIKELY (env == NULL))  {
    spawn_data->envv = g_get_environ ();
  } else {
    spawn_data->envv = g_strdupv ((char **) env);
  }

  spawn_data->envv = g_environ_setenv (spawn_data->envv,
                                       "TERM_PROGRAM", "kgx",
                                       TRUE);
  spawn_data->envv = g_environ_setenv (spawn_data->envv,
                                       "TERM_PROGRAM_VERSION", PACKAGE_VERSION,
                                       TRUE);

  kgx_proxy_info_apply_to_environ (self->proxy_info, &spawn_data->envv);

  resolved_envv = spawn_data->envv;

  kgx_task_set_spawn_data (task, g_steal_pointer (&spawn_data));

  child_data->disable_sfc =
    !kgx_settings_get_software_flow_control (settings);

  g_return_if_fail (resolved_argv != NULL);
  g_return_if_fail (resolved_argv[0] != NULL);

  vte_pty_spawn_async (pty,
                       resolved_working_directory,
                       resolved_argv,
                       resolved_envv,
                       G_SPAWN_SEARCH_PATH | G_SPAWN_SEARCH_PATH_FROM_ENVP,
                       child_setup,
                       g_steal_pointer (&child_data),
                       child_data_free,
                       -1,
                       task_cancellable,
                       did_spawn,
                       g_steal_pointer (&task));
}


/**
 * kgx_depot_spawn_finish:
 * @self: a #KgxDepot
 * @result: a #GAsyncResult
 * @error: a location for a #GError, or %NULL
 *
 * Returns: a new #KgxTrain if successful
 */
KgxTrain *
kgx_depot_spawn_finish (KgxDepot      *self,
                        GAsyncResult  *result,
                        GError       **error)
{
  g_return_val_if_fail (g_task_is_valid (result, self), NULL);

  return g_task_propagate_pointer (G_TASK (result), error);
}
