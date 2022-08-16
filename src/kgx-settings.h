/* kgx-settings.h
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

#include <glib-object.h>
#include <pango/pango.h>

#include "kgx-terminal.h"

G_BEGIN_DECLS

#define KGX_TYPE_SETTINGS kgx_settings_get_type ()

G_DECLARE_FINAL_TYPE (KgxSettings, kgx_settings, KGX, SETTINGS, GObject)

PangoFontDescription *kgx_settings_get_font          (KgxSettings       *self);
void                  kgx_settings_increase_scale    (KgxSettings       *self);
void                  kgx_settings_decrease_scale    (KgxSettings       *self);
void                  kgx_settings_reset_scale       (KgxSettings       *self);
GStrv                 kgx_settings_get_shell         (KgxSettings       *self);
void                  kgx_settings_set_custom_shell  (KgxSettings       *self,
                                                      const char *const *shell);
void                  kgx_settings_set_scrollback    (KgxSettings       *self,
                                                      int64_t            value);

G_END_DECLS
