/* kgx-spad.h
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

#include <adwaita.h>

G_BEGIN_DECLS


typedef enum /*< flags,prefix=KGX >*/ {
  KGX_SPAD_NONE = 0,                   /*< nick=none >*/
  KGX_SPAD_SHOW_REPORT = (1 << 0),     /*< nick=show-report >*/
  KGX_SPAD_SHOW_SYS_INFO = (1 << 1),   /*< nick=show-sys-info >*/
} KgxSpadFlags;


#define KGX_TYPE_SPAD (kgx_spad_get_type ())

G_DECLARE_FINAL_TYPE (KgxSpad, kgx_spad, KGX, SPAD, AdwDialog)


GVariant  *kgx_spad_build_bundle               (const char   *title,
                                                KgxSpadFlags  flags,
                                                const char   *error_body,
                                                const char   *error_content,
                                                GError       *error);
AdwDialog *kgx_spad_new_from_bundle            (GVariant     *bundle);


G_END_DECLS
