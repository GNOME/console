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
 */
KgxProcess *
kgx_process_new (GPid pid)
{
  glibtop_proc_uid   info;
  glibtop_proc_args  args_size;
  g_auto(GStrv)      args = NULL;
  KgxProcess        *self = NULL;
  
  self = g_rc_box_new0 (KgxProcess);

  self->pid = pid;

  glibtop_get_proc_uid (&info, pid);

  self->parent = info.ppid;
  self->uid = info.uid;

  args = glibtop_get_proc_argv (&args_size, pid, 0);

  self->exec = g_strdup (args[0]);

  return self;
}

/**
 * kgx_process_get_pid:
 * @self: the #KgxProcess
 * 
 * Returns: The process id
 */
GPid
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
 */
gint32
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
 */
gboolean
kgx_process_get_is_root (KgxProcess *self)
{
  g_return_val_if_fail (self != NULL, 0);

  return self->uid == 0;
}

/**
 * kgx_process_get_parent:
 * @self: the #KgxProcess
 * 
 * Get information about the processes parent
 * 
 * Returns: the parent, free with kgx_process_unref()
 */
KgxProcess *
kgx_process_get_parent (KgxProcess *self)
{
  g_return_val_if_fail (self != NULL, 0);

  return kgx_process_new (self->parent);
}

/**
 * kgx_process_get_exec:
 * @self: the #KgxProcess
 * 
 * Get the command line used to invoke to process
 */
const char *
kgx_process_get_exec (KgxProcess *self)
{
  g_return_val_if_fail (self != NULL, 0);

  return self->exec;
}

/**
 * kgx_process_get_list:
 * 
 * Get the list of running processes
 * 
 * Returns: (transfer full) (element-type Kgx.Process): List of processes free with g_ptr_array_unref()
 */
GPtrArray *
kgx_process_get_list (void)
{
  glibtop_proclist pid_list;
  g_autofree GPid *pids = NULL;
  GPtrArray *list = NULL;
  
  list = g_ptr_array_new_with_free_func ((GDestroyNotify) kgx_process_unref);

  pids = glibtop_get_proclist (&pid_list, GLIBTOP_KERN_PROC_ALL, 0);

  g_return_val_if_fail (pids != NULL, NULL);

  for (int i = 0; i < pid_list.number; i++) {
    g_ptr_array_add (list, kgx_process_new (pids[i]));
  }

  return list;
}
