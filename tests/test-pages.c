/* test-pages.c
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

#include "kgx-pages.h"


static void
test_pages_new_destroy (void)
{
  KgxPages *obj = g_object_new (KGX_TYPE_PAGES, NULL);

  g_object_ref_sink (obj);

  g_assert_finalize_object (obj);
}


static void
test_pages_initial_state (void)
{
  g_autoptr (KgxPages) obj = g_object_new (KGX_TYPE_PAGES, NULL);
  g_autoptr (GPtrArray) children = NULL;

  g_object_ref_sink (obj);

  g_assert_cmpint (kgx_pages_count (obj), ==, 0);

  children = kgx_pages_get_children (obj);

  g_assert_nonnull (children);
  g_assert_cmpint (children->len, ==, 0);

  g_assert_cmpint (kgx_pages_current_status (obj), ==, KGX_NONE);

  g_assert_false (kgx_pages_is_ringing (obj));

  g_assert_null (kgx_pages_get_selected_page (obj));
}


int
main (int argc, char *argv[])
{
  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func ("/kgx/pages/new-destroy", test_pages_new_destroy);
  g_test_add_func ("/kgx/pages/initial-state", test_pages_initial_state);

  return g_test_run ();
}
