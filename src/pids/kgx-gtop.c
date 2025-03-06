/* kgx-gtop.c
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

#include <glib.h>
#include <glibtop/proclist.h>
#include <glibtop/procuid.h>
#include <glibtop/procargs.h>

#include "kgx-gtop.h"


KgxPidsResult
kgx_gtop_get_pid_status (GPid pid, GPid *parent, uid_t *euid)
{
  glibtop_proc_uid info;

  glibtop_get_proc_uid (&info, pid);

  *parent = info.ppid;
  *euid = info.euid;

  return KGX_PIDS_OK;
}


KgxPidsResult
kgx_gtop_get_cmdline (GPid pid, GStrv *args)
{
  glibtop_proc_args args_size;

  *args = glibtop_get_proc_argv (&args_size, pid, 0);

  return KGX_PIDS_OK;
}


KgxPidsResult
kgx_gtop_get_list (GPid **pids, size_t *n_pids)
{
  glibtop_proclist pid_list;
  GPid *res = glibtop_get_proclist (&pid_list, GLIBTOP_KERN_PROC_ALL, 0);

  if (G_UNLIKELY (res == NULL)) {
    *pids = NULL;
    *n_pids = 0;

    return KGX_PIDS_ERR;
  }

  *pids = res;
  *n_pids = pid_list.number;

  return KGX_PIDS_OK;
}
