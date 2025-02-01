/* kgx-pids.c
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

#include "kgx-gtop.h"

#include "kgx-pids.h"


KgxPidsResult
kgx_pids_get_pid_info (GPid       pid,
                       GPid      *parent,
                       uid_t     *euid)
{
  return kgx_gtop_get_pid_status (pid, parent, euid);
}


KgxPidsResult
kgx_pids_get_pid_cmdline (GPid       pid,
                          GStrv     *args)
{
  return kgx_gtop_get_cmdline (pid, args);
}


KgxPidsResult
kgx_pids_get_running_pids (GPid     **pids,
                           size_t    *n_pids)
{
  return kgx_gtop_get_list (pids, n_pids);
}
