/* kgx-depot.h
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
 */

#pragma once

#include <vte/vte.h>

#include "kgx-settings.h"
#include "kgx-train.h"

G_BEGIN_DECLS

#define KGX_TYPE_DEPOT (kgx_depot_get_type ())

G_DECLARE_FINAL_TYPE (KgxDepot, kgx_depot, KGX, DEPOT, GObject)

void          kgx_depot_spawn               (KgxDepot                  *self,
                                             KgxSettings               *settings,
                                             VtePty                    *pty,
                                             const char        *const   working_directory,
                                             const char *const *const   argv,
                                             const char *const *const   env,
                                             GCancellable              *cancellable,
                                             GAsyncReadyCallback        callback,
                                             gpointer                   user_data);
KgxTrain     *kgx_depot_spawn_finish        (KgxDepot                  *self,
                                             GAsyncResult              *result,
                                             GError                   **error);

G_END_DECLS
