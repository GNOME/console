/* kgx-about.c
 *
 * Copyright 2024-2025 Zander Brown
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

#include "kgx-application.h"
#include "kgx-git-tag.h"

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
  g_autofree char *version = kgx_about_dup_version_string ();
  g_autofree char *copyright = kgx_about_dup_copyright_string ();
  struct winsize w;
  int padding = 0;

  ioctl (STDOUT_FILENO, TIOCGWINSZ, &w);

  padding = ((w.ws_row -1) - (LOGO_ROW_SIZE + 5)) / 2;

  for (int i = 0; i < padding; i++) {
    g_print ("\n");
  }

  print_logo (w.ws_col);
  print_center ("KGX", -1, w.ws_col);
  print_center (version, -1, w.ws_col);
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
  g_autofree char *version = kgx_about_dup_version_string ();

  /* Translators: The leading # is intentional, the initial %s is the
   * version of KGX itself, the latter format is the VTE version */
  g_print (_("# KGX %s using VTE %u.%u.%u %s\n"),
           version,
           vte_get_major_version (),
           vte_get_minor_version (),
           vte_get_micro_version (),
           vte_get_features ());
}


char *
kgx_about_dup_version_string (void)
{

  #ifdef IS_DEVEL
  if (g_str_equal (KGX_GIT_TAG, KGX_GIT_TAG_FALLBACK)) {
    return g_strconcat (PACKAGE_VERSION, " (Devel)", NULL);
  } else {
    return g_strconcat (PACKAGE_VERSION, " (", KGX_GIT_TAG, ")", NULL);
  }
  #else
  return g_strdup (PACKAGE_VERSION);
  #endif
}


char *
kgx_about_dup_copyright_string (void)
{
  /* Translators: %s is the year range */
  return g_strdup_printf (_("© %s Zander Brown"), KGX_COPYRIGHT_RANGE);
}


void
kgx_about_present_dialogue (GtkWidget *parent)
{
  g_autofree char *version = kgx_about_dup_version_string ();
  g_autofree char *copyright = kgx_about_dup_copyright_string ();
  g_autoptr (GString) buf = g_string_new (NULL);
  const char *developers[] = { "Zander Brown <zbrown@gnome.org>", NULL };
  const char *designers[] = { "Tobias Bernard", NULL };

  kgx_about_append_sys_info (buf, gtk_widget_get_root (parent));

  adw_show_about_dialog (parent,
                         "application-name", KGX_DISPLAY_NAME,
                         "application-icon", KGX_APPLICATION_ID,
                         "developer-name", _("The GNOME Project"),
                         "issue-url", KGX_ISSUE_URL,
                         "website", KGX_HOMEPAGE_URL,
                         "version", version,
                         "copyright", copyright,
                         "debug-info", buf->str,
                         "debug-info-filename", "kgx-info.txt",
                         "developers", developers,
                         "designers", designers,
                         /* Translators: Credit yourself here */
                         "translator-credits", _("translator-credits"),
                         "license-type", GTK_LICENSE_GPL_3_0,
                         NULL);
}


static inline void
add_lib (GString    *buf,
         const char *name,
         guint       build_maj,
         guint       build_min,
         guint       build_mic,
         guint       run_maj,
         guint       run_min,
         guint       run_mic)
{
  g_string_append_printf (buf,
                          "%s: %u.%u.%u",
                          name,
                          build_maj,
                          build_min,
                          build_mic);
  if (build_mic != run_mic || build_min != run_min || build_maj != run_maj) {
    g_string_append_printf (buf, " (%u.%u.%u)\n", run_maj, run_min, run_mic);
  } else {
    g_string_append_c (buf, '\n');
  }
}


static inline void
add_env_maybe (GString *buf, const char *name)
{
  g_autofree char *utf_value = NULL;
  const char *value = g_getenv (name);

  if (!value) {
    return;
  }

  utf_value = g_filename_display_name (value);

  g_string_append_printf (buf, "  %s: %s\n", name, utf_value);
}


void
kgx_about_append_sys_info (GString *buf, GtkRoot *root)
{
  g_autofree char *version = kgx_about_dup_version_string ();
  g_autofree char *os_name = NULL;
  g_autofree char *os_version = NULL;

  g_string_append_printf (buf, "KGX: %s\n", version);

  add_lib (buf,
           "Adw",
           ADW_MAJOR_VERSION,
           ADW_MINOR_VERSION,
           ADW_MICRO_VERSION,
           adw_get_major_version (),
           adw_get_minor_version (),
           adw_get_micro_version ());
  add_env_maybe (buf, "ADW_DISABLE_PORTAL");

  add_lib (buf,
           "Vte",
           VTE_MAJOR_VERSION,
           VTE_MINOR_VERSION,
           VTE_MICRO_VERSION,
           vte_get_major_version (),
           vte_get_minor_version (),
           vte_get_micro_version ());
  g_string_append_printf (buf, "  Features: %s\n", vte_get_features ());

  add_lib (buf,
           "Gtk",
           GTK_MAJOR_VERSION,
           GTK_MINOR_VERSION,
           GTK_MICRO_VERSION,
           gtk_get_major_version (),
           gtk_get_minor_version (),
           gtk_get_micro_version ());
  if (root) {
    g_string_append_printf (buf,
                            "  Display: %s\n",
                            G_OBJECT_TYPE_NAME (gtk_root_get_display (root)));
    g_string_append_printf (buf,
                            "  Surface: %s\n",
                            G_OBJECT_TYPE_NAME (gtk_native_get_surface (GTK_NATIVE (root))));
    g_string_append_printf (buf,
                            "  Renderer: %s\n",
                            G_OBJECT_TYPE_NAME (gtk_native_get_renderer (GTK_NATIVE (root))));
  }
  add_env_maybe (buf, "GTK_DEBUG");
  add_env_maybe (buf, "GTK_THEME");
  add_env_maybe (buf, "GTK_USE_PORTAL");

  add_lib (buf,
           "GLib",
           GLIB_MAJOR_VERSION,
           GLIB_MINOR_VERSION,
           GLIB_MICRO_VERSION,
           glib_major_version,
           glib_minor_version,
           glib_micro_version);
  add_env_maybe (buf, "G_DEBUG");

  os_name = g_get_os_info (G_OS_INFO_KEY_NAME);
  os_version = g_get_os_info (G_OS_INFO_KEY_VERSION);

  g_string_append_printf (buf, "OS: %s (%s)\n", os_name, os_version);
  add_env_maybe (buf, "XDG_CURRENT_DESKTOP");
  add_env_maybe (buf, "XDG_SESSION_DESKTOP");
  add_env_maybe (buf, "XDG_SESSION_TYPE");
  add_env_maybe (buf, "LANG");
}
