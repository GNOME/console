/* main.c
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

#include <glib/gi18n.h>
#include <kgx.h>

int
main (int   argc,
      char *argv[])
{
  g_autoptr(GtkApplication) app = NULL;
  int ret;

  /* Set up gettext translations */
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  #if IS_GENERIC
  g_set_application_name (_("Terminal"));
  gtk_window_set_default_icon_name ("org.gnome.zbrown.KingsCross.Generic");
  #else
  g_set_application_name (_("King’s Cross"));
  gtk_window_set_default_icon_name ("org.gnome.zbrown.KingsCross");
  #endif
  g_set_prgname ("org.gnome.zbrown.KingsCross");

  /*
   * Create a new GtkApplication. The application manages our main loop,
   * application windows, integration with the window manager/compositor, and
   * desktop features such as file opening and single-instance applications.
   */
  app = g_object_new (KGX_TYPE_APPLICATION,
                      "application_id", "org.gnome.zbrown.KingsCross",
                      "flags", G_APPLICATION_HANDLES_COMMAND_LINE,
                      NULL);

  /*
  * Run the application. This function will block until the application
   * exits. Upon return, we have our exit code to return to the shell. (This
   * is the code you see when you do `echo $?` after running a command in a
   * terminal.
   *
   * Since GtkApplication inherits from GApplication, we use the parent class
   * method "run". But we need to cast, which is what the "G_APPLICATION()"
   * macro does.
   */
  ret = g_application_run (G_APPLICATION (app), argc, argv);

  return ret;
}
