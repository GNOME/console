/* kgx-session-manager.h
 *
 * Copyright 2021 Zander Brown
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

#include <glib-object.h>

G_BEGIN_DECLS

#define KGX_TYPE_SESSION_MANAGER kgx_session_manager_get_type ()

G_DECLARE_FINAL_TYPE (KgxSessionManager, kgx_session_manager, KGX, SESSION_MANAGER, GObject)

KgxSessionManager *kgx_session_manager_new (void);

G_END_DECLS
