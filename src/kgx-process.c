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

#include "kgx-config.h"

#include <glib/gi18n.h>

#include <gio/gio.h>
#include <sys/types.h>

#include "kgx-pids.h"
#include "kgx-utils.h"

#include "kgx-process.h"

#define MAX_TITLE_LENGTH 100


struct _KgxProcess {
  GPid  pid;
  GPid  parent;
  uid_t euid;
  GStrv argv;
};

static void
clear_process (KgxProcess *self)
{
  g_clear_pointer (&self->argv, g_strfreev);
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
  g_autoptr (KgxProcess) self = g_rc_box_new0 (KgxProcess);

  self->pid = pid;

  if (G_UNLIKELY (kgx_pids_get_pid_info (pid,
                                         &self->parent,
                                         &self->euid) != KGX_PIDS_OK)) {
    return NULL;
  }

  return g_steal_pointer (&self);
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
 * kgx_process_get_argv:
 * @self: the #KgxProcess
 *
 * Get the command line used to invoke to process, as an array
 *
 * Stability: Private
 */
inline GStrv
kgx_process_get_argv (KgxProcess *self)
{
  g_return_val_if_fail (self != NULL, NULL);

  if (G_LIKELY (self->argv == NULL)) {
    if (G_UNLIKELY (kgx_pids_get_pid_cmdline (self->pid,
                                              &self->argv) != KGX_PIDS_OK)) {
      self->argv = NULL;
    }
  }

  return self->argv;
}


inline void
kgx_process_get_title (KgxProcess *self, char **title, char **subtitle)
{
  g_autoptr (GString) exec = g_string_sized_new (MAX_TITLE_LENGTH);
  GStrv argv;

  g_return_if_fail (self != NULL);

  argv = kgx_process_get_argv (self);

  for (size_t i = 0; argv && argv[i]; i++) {
    if (G_UNLIKELY (kgx_str_constrained_append (exec,
                                                argv[i],
                                                MAX_TITLE_LENGTH))) {
      *title = g_strdup_printf (_("Process %d"), self->pid);
      *subtitle = g_string_free (g_steal_pointer (&exec), FALSE);
      return;
    }

    g_string_append_c (exec, ' ');
  }

  *title = g_string_free (g_steal_pointer (&exec), FALSE);
  *subtitle = NULL;
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
  g_autofree GPid *pids = NULL;
  GTree *list = NULL;
  size_t n_pids;

  list = g_tree_new_full (kgx_pid_cmp,
                          NULL,
                          NULL,
                          (GDestroyNotify) kgx_process_unref);

  if (G_UNLIKELY (kgx_pids_get_running_pids (&pids, &n_pids) != KGX_PIDS_OK)) {
    return list;
  }

  g_return_val_if_fail (pids != NULL, NULL);

  for (size_t i = 0; i < n_pids; i++) {
    g_tree_insert (list, GINT_TO_POINTER (pids[i]), kgx_process_new (pids[i]));
  }

  return list;
}
