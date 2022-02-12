/* kgx-simple-tab.h
 *
 * Copyright 2019-2020 Zander Brown
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

#include "kgx-tab.h"

G_BEGIN_DECLS

#define KGX_TYPE_SIMPLE_TAB kgx_simple_tab_get_type ()

/**
 * KgxSimpleTab:
 * @title: the title of the page
 * @path: the current directory of the path
 * @initial_work_dir: the directory to start in
 * @command: the root/shell command to run
 * @terminal: the #KgxTerminal
 *
 * Stability: Private
 */
struct _KgxSimpleTab
{
  /*< private >*/
  KgxTab     parent_instance;

  /*< public >*/
  char      *title;
  GFile     *path;

  char      *initial_work_dir;
  GStrv      command;

  GtkWidget *terminal;
  GCancellable *spawn_cancellable;
};

G_DECLARE_FINAL_TYPE (KgxSimpleTab, kgx_simple_tab, KGX, SIMPLE_TAB, KgxTab)


G_END_DECLS
