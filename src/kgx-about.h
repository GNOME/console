/* kgx-about.h
 *
 * Copyright 2024 Zander Brown
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

void            kgx_about_print_logo              (void);
void            kgx_about_print_version           (void);
char           *kgx_about_dup_version_string      (void);
char           *kgx_about_dup_copyright_string    (void);
void            kgx_about_present_dialogue        (GtkWidget     *parent);
void            kgx_about_append_sys_info         (GString       *buf,
                                                   GtkRoot       *root);

G_END_DECLS
