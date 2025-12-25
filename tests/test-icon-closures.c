/* test-icon-closures.c
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

#include "kgx-config.h"

#include "kgx-enums.h"
#include "kgx-train.h"

#include "kgx-icon-closures.h"


struct status_as_icon_test {
  KgxStatus status;
  const char *expected;
} status_as_icon_cases[] = {
  { KGX_REMOTE, "status-remote-symbolic" },
  { KGX_PLAYBOX, "status-playbox-symbolic" },
  { KGX_PRIVILEGED, "status-privileged-symbolic" },
  { KGX_REMOTE | KGX_PRIVILEGED, "status-remote-symbolic" },
  { KGX_NONE, NULL },
};


static void
test_status_as_icon (gconstpointer user_data)
{
  const struct status_as_icon_test *test = user_data;
  g_autoptr (GIcon) icon = kgx_status_as_icon (NULL, test->status);

  if (test->expected) {
    g_assert_nonnull (icon);
    g_assert_true (g_strv_contains (g_themed_icon_get_names (G_THEMED_ICON (icon)),
                                    test->expected));
  } else {
    g_assert_null (icon);
  }
}


static void
test_ringing_as_icon (void)
{
  g_autoptr (GIcon) icon = kgx_ringing_as_icon (NULL, TRUE);

  g_assert_nonnull (icon);
  g_assert_true (G_IS_THEMED_ICON (icon));
  g_assert_true (g_strv_contains (g_themed_icon_get_names (G_THEMED_ICON (icon)),
                                  "bell-outline-symbolic"));

  g_assert_null (kgx_ringing_as_icon (NULL, FALSE));
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  for (size_t i = 0; i < G_N_ELEMENTS (status_as_icon_cases); i++) {
    g_autofree char *name =
      g_flags_to_string (KGX_TYPE_STATUS, status_as_icon_cases[i].status);
    g_autofree char *path =
      g_strconcat ("/kgx/icon-closures/status_as_icon/", name, NULL);

    g_test_add_data_func (path, &status_as_icon_cases[i], test_status_as_icon);
  }
  g_test_add_func ("/kgx/icon-closures/ringing_as_icon", test_ringing_as_icon);

  return g_test_run ();
}
