/* fp-vte-util.c
 *
 * Copyright 2019 Christian Hergert <chergert@redhat.com>
 *
 * Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
 * https://www.apache.org/licenses/LICENSE-2.0> or the MIT License
 * <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your
 * option. This file may not be copied, modified, or distributed
 * except according to those terms.
 *
 * SPDX-License-Identifier: (MIT OR Apache-2.0)
 */

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "fp-vte-util"


#include "fp-vte-util.h"


/**
 * SECTION:fp-vte-util
 * @title: FpVte
 * @short_description: Vte utils
 *
 * One of the more complicated things about making a terminal work across
 * pid namespaces is that you need to be careful about where you perform
 * the ioctl() for TIOCSCTTY to set the controlling terminal. If you pass
 * the PTY to another a process in another pid/pty namespace, then you risk
 * the ioctl() failing there if your fork()d process has already called the
 * same thing.
 *
 * Where this is troublesome, is that vte_pty_child_setup() does this for
 * you as part of the code that goes through the child-side of the PTY
 * dance (sometimes known in PTY parlance as the "slave FD").
 *
 * So the following code duplicates some of that to allow more precise
 * control over where the ioctl() is performed. If we are not in flatpak,
 * then we can just call vte_pty_child_setup() and have that work done for
 * us. But if we're going to pass the FD along to another pid/pty namespace,
 * we need to pass it along un-perterbed. Sadly, ioctl() TIOCNOTTY causes
 * signals to be delivered to the process group which is non-ideal.
 *
 * The API here maps vte_pty_spawn_async() mildly, but provides a GSubprocess
 * to the caller to conveniently wait/wait-check for the child.
 */


static void
fp_vte_pty_spawn_cb (VtePty       *pty,
                     GAsyncResult *result,
                     gpointer      user_data)
{
  g_autoptr (GTask) task = user_data;
  g_autoptr (GError) error = NULL;
  GPid child_pid;

  g_assert (VTE_IS_PTY (pty));
  g_assert (G_IS_ASYNC_RESULT (result));
  g_assert (G_IS_TASK (task));

  if (!vte_pty_spawn_finish (pty, result, &child_pid, &error)) {
    g_task_return_error (task, g_steal_pointer (&error));
  } else {
    g_task_return_int (task, child_pid);
  }
}


/**
 * fp_vte_pty_spawn_async:
 * @pty: a #VtePty
 * @working_directory: (nullable): the name of a directory the command should
 *   start in, or NULL to use the current working directory.
 * @argv: (array zero-terminated=1) (element-type filename): child's argument
 *   vector.
 * @env: (nullable) (array zero-terminated=1) (element-type filename): a list
 *   of environment variables to be added to the environment before starting
 *   the process, or NULL.
 * @timeout: a timeout value in ms, or -1 to wait indefinitely
 * @cancellable: (nullable): a #GCancellable, or %NULL
 * @callback: a callback to execute upon completion
 * @user_data: user data for @callback
 *
 * Creates a new process that uses #VtePty for stdin/stdout/stderr.
 *
 * See fp_vte_pty_spawn_finish() to complete the request.
 */
void
fp_vte_pty_spawn_async (VtePty              *pty,
                        const char          *working_directory,
                        const char *const   *argv,
                        const char *const   *env,
                        int                  timeout,
                        GCancellable        *cancellable,
                        GAsyncReadyCallback  callback,
                        gpointer             user_data)
{
  g_autoptr (GTask) task = NULL;
  g_auto (GStrv) copy_env = NULL;

  g_return_if_fail (VTE_IS_PTY (pty));
  g_return_if_fail (argv != NULL);
  g_return_if_fail (argv[0] != NULL);

  if (timeout < 0) {
    timeout = -1;
  }

  if (working_directory == NULL) {
    working_directory = g_get_home_dir ();
  }

  if (env == NULL)  {
    copy_env = g_get_environ ();
    env = (const char * const *) copy_env;
  }

  task = g_task_new (pty, cancellable, callback, user_data);
  g_task_set_source_tag (task, fp_vte_pty_spawn_async);

  vte_pty_spawn_async (pty,
                       working_directory,
                       (char **) argv,
                       (char **) env,
                       G_SPAWN_SEARCH_PATH | G_SPAWN_SEARCH_PATH_FROM_ENVP,
                       NULL, NULL, NULL,
                       -1,
                       cancellable,
                       (GAsyncReadyCallback) fp_vte_pty_spawn_cb,
                       g_steal_pointer (&task));
}


/**
 * fp_vte_pty_spawn_finish:
 * @pty: a #VtePty
 * @result: a #GAsyncResult
 * @child_pid: (out): a location for the #GPid
 * @error: a location for a #GError, or %NULL
 *
 * Completes a request to spawn a process for a #VtePty.
 *
 * To wait for the exit of the subprocess, use g_subprocess_wait_async()
 * or g_subprocess_wait_check_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 */
gboolean
fp_vte_pty_spawn_finish (VtePty        *pty,
                         GAsyncResult  *result,
                         GPid          *child_pid,
                         GError       **error)
{
  GPid pid;

  g_return_val_if_fail (VTE_IS_PTY (pty), FALSE);
  g_return_val_if_fail (G_IS_TASK (result), FALSE);

  pid = g_task_propagate_int (G_TASK (result), error);

  if (pid > 0) {
    if (child_pid != NULL) {
      *child_pid = pid;
    }

    return TRUE;
  }

  return FALSE;
}
