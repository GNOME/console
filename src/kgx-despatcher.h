/* kgx-despatcher.h
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

#define KGX_TYPE_DESPATCHER (kgx_despatcher_get_type ())

G_DECLARE_FINAL_TYPE (KgxDespatcher, kgx_despatcher, KGX, DESPATCHER, GObject)


void       kgx_despatcher_open                 (KgxDespatcher        *self,
                                                const char           *uri,
                                                GtkWindow            *window,
                                                GCancellable         *cancellable,
                                                GAsyncReadyCallback   callback,
                                                gpointer              user_data);
void       kgx_despatcher_open_finish          (KgxDespatcher        *self,
                                                GAsyncResult         *result,
                                                GError              **error);
void       kgx_despatcher_show_item            (KgxDespatcher        *self,
                                                const char           *uri,
                                                GtkWindow            *window,
                                                GCancellable         *cancellable,
                                                GAsyncReadyCallback   callback,
                                                gpointer              user_data);
void       kgx_despatcher_show_item_finish     (KgxDespatcher        *self,
                                                GAsyncResult         *result,
                                                GError              **error);
void       kgx_despatcher_show_folder          (KgxDespatcher        *self,
                                                const char           *uri,
                                                GtkWindow            *window,
                                                GCancellable         *cancellable,
                                                GAsyncReadyCallback   callback,
                                                gpointer              user_data);
void       kgx_despatcher_show_folder_finish   (KgxDespatcher        *self,
                                                GAsyncResult         *result,
                                                GError              **error);


G_END_DECLS
