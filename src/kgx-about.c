/* kgx-about.c
 *
 * Copyright 2024 Zander Brown
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

#include "kgx-config.h"

#include <glib/gi18n.h>

#include <unistd.h>
#include <sys/ioctl.h>
#include <gio/gio.h>
#include <vte/vte.h>

#include "kgx-about.h"

#define LOGO_COL_SIZE 28
#define LOGO_ROW_SIZE 14


static inline void
print_center (char *msg, int ign, short width)
{
  int half_msg = 0;
  int half_screen = 0;

  half_msg = strlen (msg) / 2;
  half_screen = width / 2;

  g_print ("%*s\n",
           half_screen + half_msg,
           msg);
}


static inline void
print_logo (short width)
{
  g_autoptr (GFile) logo = NULL;
  g_autoptr (GError) error = NULL;
  g_auto (GStrv) logo_lines = NULL;
  g_autofree char *logo_text = NULL;
  int i = 0;
  int half_screen = width / 2;

  logo = g_file_new_for_uri ("resource:/" KGX_APPLICATION_PATH "logo.txt");

  g_file_load_contents (logo, NULL, &logo_text, NULL, NULL, &error);

  if (error) {
    g_error ("Wat? %s", error->message);
  }

  logo_lines = g_strsplit (logo_text, "\n", -1);

  while (logo_lines[i]) {
    g_print ("%*s%s\n",
             half_screen - (LOGO_COL_SIZE / 2),
             "",
             logo_lines[i]);

    i++;
  }
}


void
kgx_about_print_logo (void)
{
  g_autofree char *copyright = g_strdup_printf (_("© %s Zander Brown"),
                                                "2019-2024");
  struct winsize w;
  int padding = 0;

  ioctl (STDOUT_FILENO, TIOCGWINSZ, &w);

  padding = ((w.ws_row -1) - (LOGO_ROW_SIZE + 5)) / 2;

  for (int i = 0; i < padding; i++) {
    g_print ("\n");
  }

  print_logo (w.ws_col);
  print_center ("KGX", -1, w.ws_col);
  print_center (PACKAGE_VERSION, -1, w.ws_col);
  print_center (_("Terminal Emulator"), -1, w.ws_col);
  print_center (copyright, -1, w.ws_col);
  print_center (_("GPL 3.0 or later"), -1, w.ws_col);

  for (int i = 0; i < padding; i++) {
    g_print ("\n");
  }
}


void
kgx_about_print_version (void)
{
  /* Translators: The leading # is intentional, the initial %s is the
   * version of KGX itself, the latter format is the VTE version */
  g_print (_("# KGX %s using VTE %u.%u.%u %s\n"),
           PACKAGE_VERSION,
           vte_get_major_version (),
           vte_get_minor_version (),
           vte_get_micro_version (),
           vte_get_features ());
}
