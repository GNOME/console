/* kgx-process.c
 *
 * Copyright 2019 Zander Brown
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
 * SECTION:kgx-process
 * @title: KgxProcess
 * @short_description: Information about running processes
 *
 * Provides an abstraction of libgtop to fetch information about running
 * process, this is used by #KgxApplication to monitor things happening in
 * a #KgxTerminal for the purposes of styling a #KgxWindow
 */

#include <gio/gio.h>
#include <glibtop/proclist.h>
#include <glibtop/procuid.h>
#include <glibtop/procargs.h>

#include "kgx-process.h"

struct _KgxProcess {
  GPid    pid;
  GPid    parent;
  gint32  uid;
  gint32  euid;
  char   *exec;
};

static void
clear_process (KgxProcess *self)
{
  g_clear_pointer (&self->exec, g_free);
}

/**
 * kgx_process_unref:
 * @self: the #KgxProcess
 *
 * Reduce the refrence count of @self, possibly freeing @self
 *
 * See g_rc_box_acquire() and g_rc_box_release_full()
 *
 * Stability: Private
 */
void
kgx_process_unref (KgxProcess *self)
{
  g_return_if_fail (self != NULL);

  g_rc_box_release_full (self, (GDestroyNotify) clear_process);
}

G_DEFINE_BOXED_TYPE (KgxProcess, kgx_process, g_rc_box_acquire, kgx_process_unref)

/**
 * kgx_process_new:
 * @pid: The #GPid to get info about
 *
 * Populate a new #KgxProcess with details about the process @pid
 *
 * Stability: Private
 */
inline KgxProcess *
kgx_process_new (GPid pid)
{
  glibtop_proc_uid   info;
  KgxProcess        *self = NULL;

  self = g_rc_box_new0 (KgxProcess);

  self->pid = pid;

  glibtop_get_proc_uid (&info, pid);

  self->parent = info.ppid;
  self->uid = info.uid;
  self->euid = info.euid;
  self->exec = NULL;

  return self;
}

/**
 * kgx_process_get_pid:
 * @self: the #KgxProcess
 *
 * Returns: The process id
 *
 * Stability: Private
 */
inline GPid
kgx_process_get_pid (KgxProcess *self)
{
  g_return_val_if_fail (self != NULL, 0);

  return self->pid;
}

/**
 * kgx_process_get_uid:
 * @self: the #KgxProcess
 *
 * Returns: The user id of the process
 *
 * Stability: Private
 */
inline gint32
kgx_process_get_uid (KgxProcess *self)
{
  g_return_val_if_fail (self != NULL, 0);

  return self->uid;
}

/**
 * kgx_process_get_is_root:
 * @self: the #KgxProcess
 *
 * Returns: %TRUE if this process is running as root
 *
 * Stability: Private
 */
inline gboolean
kgx_process_get_is_root (KgxProcess *self)
{
  g_return_val_if_fail (self != NULL, FALSE);

  return self->euid == 0;
}

/**
 * kgx_process_get_parent:
 * @self: the #KgxProcess
 *
 * Get information about the processes parent
 *
 * Note a previous version (0.1.0) returned a #KgxProcess, this has been
 * changed in favour of lazy loading
 *
 * Returns: the parent #GPid
 *
 * Stability: Private
 */
inline GPid
kgx_process_get_parent (KgxProcess *self)
{
  g_return_val_if_fail (self != NULL, 0);

  return self->parent;
}

/**
 * kgx_process_get_exec:
 * @self: the #KgxProcess
 *
 * Get the command line used to invoke to process
 *
 * Stability: Private
 */
inline const char *
kgx_process_get_exec (KgxProcess *self)
{
  g_return_val_if_fail (self != NULL, NULL);

  if (G_LIKELY (self->exec == NULL)) {
    g_auto(GStrv)     args = NULL;
    glibtop_proc_args args_size;

    args = glibtop_get_proc_argv (&args_size, self->pid, 0);

    self->exec = g_strjoinv (" ", args);
  }

  return self->exec;
}

/**
 * kgx_pid_cmp:
 * @a: the first #GPid
 * @b: the second #GPid
 * @data: unused
 *
 * Implementation of #GCompareDataFunc for comparing #GPid encoded with
 * GINT_TO_POINTER()
 *
 * Returns: difference between @a and @b
 *
 * Stability: Private
 */
int
kgx_pid_cmp (gconstpointer a, gconstpointer b, gpointer data)
{
  return a - b;
}

/**
 * kgx_process_get_list: (skip)
 *
 * Get the list of running processes
 *
 * Note: This originally (0.1.0) returned #GPtrArray but now returns #GTree
 * for faster lookup
 *
 * Note: (skip) due to
 * https://gitlab.gnome.org/GNOME/gobject-introspection/issues/310
 *
 * #GTree is map of #GPid -> #KgxProcess
 *
 * Returns: ~ (transfer full) (element-type GLib.Pid Kgx.Process)
 * List of processes, free with g_tree_unref()
 *
 * Stability: Private
 */
GTree *
kgx_process_get_list (void)
{
  glibtop_proclist pid_list;
  g_autofree GPid *pids = NULL;
  GTree *list = NULL;

  list = g_tree_new_full (kgx_pid_cmp,
                          NULL,
                          NULL,
                          (GDestroyNotify) kgx_process_unref);

  pids = glibtop_get_proclist (&pid_list, GLIBTOP_KERN_PROC_ALL, 0);

  g_return_val_if_fail (pids != NULL, NULL);

  for (int i = 0; i < pid_list.number; i++) {
    g_tree_insert (list, GINT_TO_POINTER (pids[i]), kgx_process_new (pids[i]));
  }

  return list;
}
