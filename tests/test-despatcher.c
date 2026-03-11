/* test-despatcher.c
 *
 * Copyright 2026 Zander Brown
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

#include "kgx-locale.h"

#include "kgx-despatcher.h"


static void
test_despatcher_new_destroy (void)
{
  KgxDespatcher *despatcher = g_object_new (KGX_TYPE_DESPATCHER, NULL);

  g_assert_finalize_object (despatcher);
}


int
main (int argc, char *argv[])
{
  kgx_locale_init (KGX_LOCALE_TEST);

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/kgx/despatcher/new-destroy", test_despatcher_new_destroy);

  return g_test_run ();
}
