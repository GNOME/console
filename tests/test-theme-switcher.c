/* test-theme-switcher.c
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

#include "kgx-locale.h"
#include "kgx-settings.h"

#include "kgx-theme-switcher.h"


static void
test_theme_switcher_new_destroy (void)
{
  KgxThemeSwitcher *spad =
    g_object_new (KGX_TYPE_THEME_SWITCHER, NULL);

  g_object_ref_sink (spad);

  g_assert_finalize_object (spad);
}


static void
test_theme_switcher_get_set (void)
{
  g_autoptr (KgxThemeSwitcher) switcher =
    g_object_new (KGX_TYPE_THEME_SWITCHER, NULL);
  KgxTheme themes[] = {
    KGX_THEME_NIGHT, KGX_THEME_DAY, KGX_THEME_AUTO, KGX_THEME_NIGHT,
    KGX_THEME_AUTO, KGX_THEME_DAY, KGX_THEME_DAY, KGX_THEME_NIGHT,
  };
  KgxTheme result = KGX_THEME_AUTO;

  g_object_ref_sink (switcher);

  for (size_t i = 0; i < G_N_ELEMENTS (themes); i++) {
    g_object_set (switcher, "theme", themes[i], NULL);
    g_object_get (switcher, "theme", &result, NULL);

    g_assert_cmpuint (result, ==, themes[i]);
  }
}


int
main (int argc, char *argv[])
{
  kgx_locale_init (KGX_LOCALE_TEST);

  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func ("/kgx/theme-switcher/new-destroy", test_theme_switcher_new_destroy);
  g_test_add_func ("/kgx/theme-switcher/get-set", test_theme_switcher_get_set);

  return g_test_run ();
}
