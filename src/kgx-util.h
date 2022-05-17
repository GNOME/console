/* kgx-util.h
 *
 * Copyright 2022 Zander Brown
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
#include <gio/gio.h>

G_BEGIN_DECLS

void       kgx_util_transform_uris_to_quoted_fuse_paths (GStrv       uris);
char      *kgx_util_concat_uris                         (GStrv       uris,
                                                         gsize      *length);

G_END_DECLS
