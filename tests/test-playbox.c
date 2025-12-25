/* test-playbox.c
 *
 * Copyright 2025 Zander Brown
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


#include "kgx-playbox.h"


struct playbox_test {
  const char *basename;
  const char **argv;
  gboolean expected;
} playbox_cases[] = {
  { "toolbox", (const char *[]) { "/usr/bin/toolbox", "enter", NULL }, TRUE },
  { "toolbox", (const char *[]) { "/usr/bin/toolbox", "list", NULL }, FALSE },
  { "toolbox", (const char *[]) { "/usr/bin/toolbox", NULL }, FALSE },
  { "sh", (const char *[]) { "/usr/bin/sh", "/usr/bin/distrobox", "enter", NULL }, TRUE },
  { "sh", (const char *[]) { "/usr/bin/sh", "/usr/bin/distrobox", "list", NULL }, FALSE },
  { "sh", (const char *[]) { "/usr/bin/sh", "/usr/bin/distrobox", NULL }, FALSE },
  { "kgx", (const char *[]) { "/usr/bin/kgx", NULL }, FALSE },
};


static void
test_is_playbox (gconstpointer user_data)
{
  const struct playbox_test *test = user_data;

  if (test->expected) {
    g_assert_true (kgx_is_playbox (test->basename, (GStrv) test->argv));
  } else {
    g_assert_false (kgx_is_playbox (test->basename, (GStrv) test->argv));
  }
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  for (size_t i = 0; i < G_N_ELEMENTS (playbox_cases); i++) {
    g_autofree char *path =
      g_strdup_printf ("/kgx/playbox/is_playbox/case_%" G_GSIZE_FORMAT, i);

    g_test_add_data_func (path, &playbox_cases[i], test_is_playbox);
  }

  return g_test_run ();
}
