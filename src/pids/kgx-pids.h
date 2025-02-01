/* kgx-pids.h
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

#pragma once

#include <glib.h>
#include <sys/types.h>

#include "kgx-pids-enums.h"

G_BEGIN_DECLS

typedef enum {
  KGX_PIDS_OK,
  KGX_PIDS_ERR,
} KgxPidsResult;


KgxPidsResult   kgx_pids_get_pid_info          (GPid       pid,
                                                GPid      *parent,
                                                uid_t     *euid);
KgxPidsResult   kgx_pids_get_pid_cmdline       (GPid       pid,
                                                GStrv     *args);
KgxPidsResult   kgx_pids_get_running_pids      (GPid     **pids,
                                                size_t    *n_pids);

G_END_DECLS
