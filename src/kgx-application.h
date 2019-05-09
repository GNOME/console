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
#include "kgx-terminal.h"

G_BEGIN_DECLS

#define KGX_TYPE_APPLICATION (kgx_application_get_type())

/**
 * ProcessWatch:
 * @window: the window the #KgxProcess is in
 * @process: what we are watching
 * 
 * Stability: Private
 */
struct ProcessWatch {
  KgxWindow  *window;
  KgxProcess *process;
};

/**
 * KgxApplication:
 * @theme: the colour palette in use
 * @watching: (element-type ProcessWatch): the shells running in windows
 * @children: (element-type ProcessWatch): the processes running in shells
 * 
 * Stability: Private
 */
struct _KgxApplication
{
  /*< private >*/
  GtkApplication  parent_instance;

  /*< public >*/
  KgxTheme        theme;

  GPtrArray      *watching;
  GPtrArray      *children;
};

G_DECLARE_FINAL_TYPE (KgxApplication, kgx_application, KGX, APPLICATION, GtkApplication)

void kgx_application_add_watch (KgxApplication *self,
                                KgxProcess     *process,
                                KgxWindow      *window);

G_END_DECLS

