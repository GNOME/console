/* main.c
 *
 * Copyright 2019-2021 Zander Brown
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
main (int argc, char *argv[])
{
  g_autoptr (GtkApplication) app = NULL;

  /* Set up gettext translations */
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  g_set_application_name (KGX_DISPLAY_NAME);
  gtk_window_set_default_icon_name (KGX_APPLICATION_ID);

  app = g_object_new (KGX_TYPE_APPLICATION,
                      "application_id", KGX_APPLICATION_ID,
                      "flags", G_APPLICATION_HANDLES_COMMAND_LINE |
                               G_APPLICATION_HANDLES_OPEN |
                               G_APPLICATION_CAN_OVERRIDE_APP_ID,
                      "register-session", TRUE,
                      NULL);

  return g_application_run (G_APPLICATION (app), argc, argv);
}
