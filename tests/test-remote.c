/* test-remote.c
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


#include "kgx-remote.h"


struct remote_test {
  const char *basename;
  const char **argv;
  gboolean expected;
} remote_cases[] = {
  { "ssh", (const char *[]) { "/usr/bin/ssh", NULL }, TRUE },
  { "telnet", (const char *[]) { "/usr/bin/telnet", NULL }, TRUE },
  { "mosh-client", (const char *[]) { "/usr/bin/mosh-client", NULL }, TRUE },
  { "mosh", (const char *[]) { "/usr/bin/mosh", NULL }, TRUE },
  { "et", (const char *[]) { "/usr/bin/et", NULL }, TRUE },
  { "kgx", (const char *[]) { "/usr/bin/kgx", NULL }, FALSE },
  { "waypipe", (const char *[]) { "/usr/bin/waypipe", NULL }, FALSE },
  { "waypipe", (const char *[]) { "/usr/bin/waypipe", "ssh", NULL }, TRUE },
  { "waypipe", (const char *[]) { "/usr/bin/waypipe", "telnet", NULL }, TRUE },
  { "waypipe", (const char *[]) { "/usr/bin/waypipe", "kgx", NULL }, FALSE },
};


static void
test_is_remote (gconstpointer user_data)
{
  const struct remote_test *test = user_data;

  if (test->expected) {
    g_assert_true (kgx_is_remote (test->basename, (GStrv) test->argv));
  } else {
    g_assert_false (kgx_is_remote (test->basename, (GStrv) test->argv));
  }
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  for (size_t i = 0; i < G_N_ELEMENTS (remote_cases); i++) {
    g_autofree char *path =
      g_strdup_printf ("/kgx/remote/is_remote/case_%" G_GSIZE_FORMAT, i);

    g_test_add_data_func (path, &remote_cases[i], test_is_remote);
  }

  return g_test_run ();
}
