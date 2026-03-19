/* kgx-test-gtk-utils.h
 *
 * Copyright 2026 Zander Brown
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

#include <gtk/gtk.h>


G_BEGIN_DECLS


void         kgx_test_flush_widget        (GtkWidget    *widget);
gboolean     kgx_test_is_on_x11           (GtkWidget    *widget);
void         kgx_test_save_screenshot     (GtkWidget    *widget,
                                           const char   *filename);

G_END_DECLS
