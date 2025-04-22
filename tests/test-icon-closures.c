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

#include "kgx-train.h"

#include "kgx-icon-closures.h"


static void
test_status_as_icon (void)
{
  g_autoptr (GIcon) remote_icon = kgx_status_as_icon (NULL, KGX_REMOTE);
  g_autoptr (GIcon) root_icon = kgx_status_as_icon (NULL, KGX_PRIVILEGED);
  g_autoptr (GIcon) remote_root_icon =
    kgx_status_as_icon (NULL, KGX_REMOTE | KGX_PRIVILEGED);

  g_assert_nonnull (remote_icon);
  g_assert_true (G_IS_THEMED_ICON (remote_icon));
  g_assert_true (g_strv_contains (g_themed_icon_get_names (G_THEMED_ICON (remote_icon)),
                                  "status-remote-symbolic"));

  g_assert_nonnull (root_icon);
  g_assert_true (G_IS_THEMED_ICON (root_icon));
  g_assert_true (g_strv_contains (g_themed_icon_get_names (G_THEMED_ICON (root_icon)),
                                  "status-privileged-symbolic"));

  g_assert_nonnull (remote_root_icon);
  g_assert_true (G_IS_THEMED_ICON (remote_root_icon));
  g_assert_true (g_strv_contains (g_themed_icon_get_names (G_THEMED_ICON (remote_root_icon)),
                                  "status-remote-symbolic"));

  g_assert_null (kgx_status_as_icon (NULL, KGX_NONE));
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

  g_test_add_func ("/kgx/icon-closures/status_as_icon", test_status_as_icon);
  g_test_add_func ("/kgx/icon-closures/ringing_as_icon", test_ringing_as_icon);

  return g_test_run ();
}
