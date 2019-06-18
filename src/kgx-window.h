/* kgx-window.h
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

#include "kgx-terminal.h"

G_BEGIN_DECLS

#define KGX_TYPE_WINDOW (kgx_window_get_type())

/**
 * KgxWindow:
 * @theme: the palette
 * @working_dir: the working directory of the #KgxTerminal
 * @command: the command to run, %NULL for default shell
 * @close_on_zero: should the window close when the command exits with 0
 * @last_cols: the column width last time we received #GtkWidget::size-allocate
 * @last_rows: the row count last time we received #GtkWidget::size-allocate
 * @timeout: the id of the #GSource used to hide the statusbar
 * @root: count of the children running as root, when its > 0 root styles are
 * applied to the headerbar
 * @remote: same as @root but for ssh
 * @header_bar: the #GtkHeaderBar that the styles are applied to
 * @terminal: the #KgxTerminal the window contains
 * @dims: the floating status bar
 * @search_bar: the windows #GtkSearchBar
 * @exit_info: the #GtkRevealer hat wraps @exit_message
 * @exit_message: the #GtkLabel for showing important messages
 */
struct _KgxWindow
{
  /*< private >*/
  GtkApplicationWindow  parent_instance;

  /*< public >*/
  KgxTheme              theme;
  char                 *working_dir;
  char                 *command;
  gboolean              close_on_zero;

  /* Size indicator */
  int                   last_cols;
  int                   last_rows;
  guint                 timeout;

  /* Remote/root states */
  int                   root;
  int                   remote;

  /* Template widgets */
  GtkWidget            *header_bar;
  GtkWidget            *terminal;
  GtkWidget            *dims;
  GtkWidget            *search_bar;
  GtkWidget            *exit_info;
  GtkWidget            *exit_message;
};

G_DECLARE_FINAL_TYPE (KgxWindow, kgx_window, KGX, WINDOW, GtkApplicationWindow)

const char *kgx_window_get_working_dir (KgxWindow *self);
void        kgx_window_push_root       (KgxWindow *self);
void        kgx_window_pop_root        (KgxWindow *self);
void        kgx_window_push_remote     (KgxWindow *self);
void        kgx_window_pop_remote      (KgxWindow *self);


G_END_DECLS
