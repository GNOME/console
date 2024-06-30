/* kgx-application.h
 *
 * Copyright 2019-2023 Zander Brown
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

#include "kgx-tab.h"
#include "kgx-window.h"

G_BEGIN_DECLS

#define KGX_DISPLAY_NAME _("Console")

#define KGX_TYPE_APPLICATION (kgx_application_get_type())

G_DECLARE_FINAL_TYPE (KgxApplication, kgx_application, KGX, APPLICATION, AdwApplication)


void                  kgx_application_add_page        (KgxApplication *self,
                                                       KgxTab         *page);
KgxTab               *kgx_application_lookup_page     (KgxApplication *self,
                                                       guint           id);
KgxTab *              kgx_application_add_terminal    (KgxApplication *self,
                                                       KgxWindow      *existing_window,
                                                       GFile          *working_directory,
                                                       GStrv           command,
                                                       const char     *title);

G_END_DECLS
