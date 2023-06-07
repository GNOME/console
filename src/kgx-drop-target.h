/* kgx-drop-target.h
 *
 * Copyright 2023 Zander Brown
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

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define KGX_TYPE_DROP_TARGET (kgx_drop_target_get_type ())

G_DECLARE_FINAL_TYPE (KgxDropTarget, kgx_drop_target, KGX, DROP_TARGET, GObject)


void       kgx_drop_target_extra_drop         (KgxDropTarget *self,
                                               const GValue  *value);
void       kgx_drop_target_mount_on           (KgxDropTarget *self,
                                               GtkWidget     *widget);


G_END_DECLS
