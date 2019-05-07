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

#define G_LOG_DOMAIN "fp-vte-util"

#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#include <errno.h>
#include <fcntl.h>
#include <glib-unix.h>
#include <sys/ioctl.h>
#ifdef __linux__
# include <sys/prctl.h>
#endif
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

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

static gboolean
is_flatpak (void)
{
  static gsize initialized;
  static gboolean _is_flatpak;

  if (g_once_init_enter (&initialized))
    {
      _is_flatpak = g_file_test ("/.flatpak-info", G_FILE_TEST_EXISTS);
      g_once_init_leave (&initialized, TRUE);
    }

  return _is_flatpak;
}

/**
 * fp_vte_guess_shell:
 * @cancellable: (nullable): a #GCancellable or %NULL
 * @error: a location for a #GError, or %NULL
 *
 * Guesses the users preferred shell, possibly querying the host.
 *
 * Returns: (transfer full): a string containing the shell or %NULL
 *   and @error is set.
 */
gchar *
fp_vte_guess_shell (GCancellable  *cancellable,
                    GError       **error)
{
  g_autoptr(GPtrArray) argv = NULL;
  g_autoptr(GSubprocessLauncher) launcher = NULL;
  g_autoptr(GSubprocess) subprocess = NULL;
  g_autofree gchar *stdout_buf = NULL;
  g_auto(GStrv) parts = NULL;

  if (!is_flatpak ())
    return vte_get_user_shell ();

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (gchar *)"flatpak-spawn");
  g_ptr_array_add (argv, (gchar *)"--host");
  g_ptr_array_add (argv, (gchar *)"getent");
  g_ptr_array_add (argv, (gchar *)"passwd");
  g_ptr_array_add (argv, (gchar *)g_get_user_name ());
  g_ptr_array_add (argv, NULL);

  launcher = g_subprocess_launcher_new (G_SUBPROCESS_FLAGS_STDOUT_PIPE |
                                        G_SUBPROCESS_FLAGS_STDERR_SILENCE);
  /* Be certain that G_MESSAGES_DEBUG cannot effect stdout */
  g_subprocess_launcher_unsetenv (launcher, "G_MESSAGES_DEBUG");
  subprocess = g_subprocess_launcher_spawnv (launcher,
                                             (const gchar * const *)argv->pdata,
                                             error);

  if (subprocess == NULL)
    return NULL;

  if (!g_subprocess_communicate_utf8 (subprocess, NULL, cancellable, &stdout_buf, NULL, error))
    return NULL;

  parts = g_strsplit (stdout_buf, ":", 0);

  if (g_strv_length (parts) < 7)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_FAILED,
                   "Failed to locate user entry");
      return NULL;
    }

  return g_strdup (g_strstrip (parts[6]));
}

static void
fp_vte_pty_child_setup_cb (VtePty *pty)
{
  if (VTE_IS_PTY (pty))
    vte_pty_child_setup (pty);

#ifdef __linux__
  /* Ensure this process gets SIGTERM if the parent dies */
  if (prctl (PR_SET_PDEATHSIG, SIGKILL) != 0)
    fprintf (stderr, "Failed to set PR_SET_PDEATHSIG: %s\n",
             g_strerror (errno));
#endif
}

static gint
create_inferior_pty (gint superior_fd)
{
#if defined(HAVE_PTSNAME_R) || defined(__FreeBSD__)
  char name[256];
#else
  const char *name;
#endif
  gint fd = -1;

  g_return_val_if_fail (superior_fd != -1, -1);

  if (grantpt (superior_fd) != 0)
    return -1;

  if (unlockpt (superior_fd) != 0)
    return -1;

#ifdef HAVE_PTSNAME_R
  if (ptsname_r (superior_fd, name, sizeof name - 1) != 0)
    return IDE_PTY_FD_INVALID;
  name[sizeof name - 1] = '\0';
#elif defined(__FreeBSD__)
  if (fdevname_r (superior_fd, name + 5, sizeof name - 6) == NULL)
    return IDE_PTY_FD_INVALID;
  memcpy (name, "/dev/", 5);
  name[sizeof name - 1] = '\0';
#else
  if (!(name = ptsname (superior_fd)))
    return -1;
#endif

  fd = open (name, O_RDWR | O_CLOEXEC);

  if (fd == -1 && errno == EINVAL)
    {
      gint flags;

      fd = open (name, O_RDWR | O_CLOEXEC);
      if (fd == -1 && errno == EINVAL)
        fd = open (name, O_RDWR);

      if (fd == -1)
        return -1;

      /* Add FD_CLOEXEC if O_CLOEXEC failed */
      flags = fcntl (fd, F_GETFD, 0);
      if ((flags & FD_CLOEXEC) == 0)
        fcntl (fd, F_SETFD, flags | FD_CLOEXEC);
    }

  return fd;
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
                        const gchar         *working_directory,
                        const gchar * const *argv,
                        const gchar * const *env,
                        gint                 timeout,
                        GCancellable        *cancellable,
                        GAsyncReadyCallback  callback,
                        gpointer             user_data)
{
  g_autoptr(GSubprocessLauncher) launcher = NULL;
  g_autoptr(GSubprocess) subprocess = NULL;
  g_autoptr(GPtrArray) real_argv = NULL;
  g_autoptr(GError) error = NULL;
  g_autoptr(GTask) task = NULL;
  g_auto(GStrv) copy_env = NULL;
  gint child_fd;

  g_return_if_fail (VTE_IS_PTY (pty));
  g_return_if_fail (argv != NULL);
  g_return_if_fail (argv[0] != NULL);

  if (timeout < 0)
    timeout = -1;

  if (working_directory == NULL)
    working_directory = g_get_home_dir ();

  if (env == NULL)
    {
      copy_env = g_get_environ ();
      env = (const gchar * const *)copy_env;
    }

  task = g_task_new (pty, cancellable, callback, user_data);
  g_task_set_source_tag (task, fp_vte_pty_spawn_async);

  /* Create the inferior side of the PTY */
  if ((child_fd = create_inferior_pty (vte_pty_get_fd (pty))) == -1)
    {
      g_task_return_new_error (task,
                               G_IO_ERROR,
                               g_io_error_from_errno (errno),
                               "%s", g_strerror (errno));
      return;
    }

  /*
   * We can't use vte_pty_spawn_async() because it will try to launch
   * our process from a thread, and that means we can't use prctl() to
   * ensure the child receives SIGTERM when the application exists. And
   * who wants that? If you don't need that for some reason, you can
   * probably ignore this and call vte_pty_spawn_async().
   */
  launcher = g_subprocess_launcher_new (0);
  g_subprocess_launcher_set_environ (launcher, (gchar **)env);
  g_subprocess_launcher_set_cwd (launcher, working_directory);
  g_subprocess_launcher_take_stdout_fd (launcher, dup (child_fd));
  g_subprocess_launcher_take_stderr_fd (launcher, dup (child_fd));
  g_subprocess_launcher_take_stdin_fd (launcher, child_fd);
  g_subprocess_launcher_set_child_setup (launcher,
                                         (GSpawnChildSetupFunc) fp_vte_pty_child_setup_cb,
                                         is_flatpak () ? NULL : pty, NULL);

  /* Setup argv[] for the child process, possibly running via
   * flatpak-spawn to call via the host and proxy signals to/from
   * the host process. We might need to pass along some environment
   * variables to the host using --env=FOO=BAR to flatpak-spawn.
   * You might want to pre-filter those in your application to some
   * limited set (like DISPLAY, WAYLAND_DISPLAY, etc).
   */
  real_argv = g_ptr_array_new_with_free_func (g_free);
  if (is_flatpak ())
    {
      g_ptr_array_add (real_argv, g_strdup ("flatpak-spawn"));
      g_ptr_array_add (real_argv, g_strdup ("--host"));
      g_ptr_array_add (real_argv, g_strdup ("--watch-bus"));
      for (guint i = 0; env[i]; i++)
        g_ptr_array_add (real_argv, g_strdup_printf ("--env=%s", env[i]));
    }
  for (guint i = 0; argv[i]; i++)
    g_ptr_array_add (real_argv, g_strdup (argv[i]));
  g_ptr_array_add (real_argv, NULL);

  subprocess = g_subprocess_launcher_spawnv (launcher,
                                             (const gchar * const *)real_argv->pdata,
                                             &error);

  if (subprocess == NULL)
    g_task_return_error (task, g_steal_pointer (&error));
  else
    g_task_return_pointer (task, g_steal_pointer (&subprocess), g_object_unref);
}

/**
 * fp_vte_pty_spawn_finish:
 * @pty: a #VtePty
 * @result: a #GAsyncResult
 * @error: a location for a #GError, or %NULL
 *
 * Completes a request to spawn a process for a #VtePty.
 *
 * To wait for the exit of the subprocess, use g_subprocess_wait_async()
 * or g_subprocess_wait_check_async().
 *
 * Returns: (transfer full): a #GSubprocess if successful; otherwise
 *   %NULL and @error is set.
 */
GSubprocess *
fp_vte_pty_spawn_finish (VtePty        *pty,
                         GAsyncResult  *result,
                         GError       **error)
{
  g_return_val_if_fail (VTE_IS_PTY (pty), FALSE);
  g_return_val_if_fail (G_IS_TASK (result), FALSE);

  return g_task_propagate_pointer (G_TASK (result), error);
}
