/* kgx-spad-source.h
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

#include <glib-object.h>

#include "kgx-spad.h"

G_BEGIN_DECLS

#define KGX_TYPE_SPAD_SOURCE (kgx_spad_source_get_type ())

G_DECLARE_INTERFACE (KgxSpadSource, kgx_spad_source, KGX, SPAD_SOURCE, GObject)


struct _KgxSpadSourceInterface {
  GTypeInterface g_iface;
};


void     kgx_spad_source_throw                 (KgxSpadSource *source,
                                                const char    *title,
                                                KgxSpadFlags   flags,
                                                const char    *error_body,
                                                const char    *error_content,
                                                GError        *error);


G_END_DECLS
