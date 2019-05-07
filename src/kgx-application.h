/* kgx-application.h
 *
 * Copyright 2019 Zander Brown
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

#include "kgx-process.h"
#include "kgx-window.h"
#include "kgx-enums.h"

G_BEGIN_DECLS

/**
 * KgxTheme:
 * @KGX_THEME_NIGHT: The default, public, theme
 * @KGX_THEME_HACKER: Little easter egg theme
 * 
 * Until [meson#1687](https://github.com/mesonbuild/meson/issues/1687) is
 * resolved this enum must be manually kept in sync with
 * org.gnome.zbrown.KingsCross.Theme in the gschema
 */
typedef enum /*< enum,prefix=KGX >*/
{
  KGX_THEME_NIGHT = 1,  /*< nick=night >*/
  KGX_THEME_HACKER = 2, /*< nick=hacker >*/
} KgxTheme;

#define KGX_TYPE_APPLICATION (kgx_application_get_type())

G_DECLARE_FINAL_TYPE (KgxApplication, kgx_application, KGX, APPLICATION, GtkApplication)

void kgx_application_add_watch (KgxApplication *self,
                                KgxProcess     *process,
                                KgxWindow      *window);

G_END_DECLS

