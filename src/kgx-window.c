/* kgx-window.c
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

#include <vte/vte.h>

#include "kgx-config.h"
#include "kgx-window.h"

struct _KgxWindow
{
  GtkApplicationWindow  parent_instance;

  /* Template widgets */
  GtkWidget        *header_bar;
  GtkWidget        *terminal;
};

G_DEFINE_TYPE (KgxWindow, kgx_window, GTK_TYPE_APPLICATION_WINDOW)

static void
kgx_window_class_init (KgxWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/zbrown/KingsCross/kgx-window.ui");
  gtk_widget_class_bind_template_child (widget_class, KgxWindow, header_bar);
  gtk_widget_class_bind_template_child (widget_class, KgxWindow, terminal);
}

static void
kgx_window_init (KgxWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gchar *shell[2] = {NULL, NULL};

  shell[0] = vte_get_user_shell ();
  if (shell[0] == NULL) {
    shell[0] = "/bin/sh";
    g_warning ("Defaulting to /bin/sh");
  }

  vte_terminal_spawn_async (VTE_TERMINAL (self->terminal),
                            VTE_PTY_DEFAULT,
                            NULL,
                            shell,
                            NULL,
                            G_SPAWN_DEFAULT,
                            NULL,
                            NULL,
                            NULL,
                            -1,
                            NULL,
                            NULL,
                            NULL);
}
