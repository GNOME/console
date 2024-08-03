/* test-colour-utils.c
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


#include "kgx-colour-utils.h"


static void
test_colour_error_quark (void)
{
  g_assert_nonnull (g_quark_to_string (kgx_colour_parse_error_quark ()));
}


static void
test_colour_to_string (void)
{
  GdkRGBA colour = { .red = 1.0, .green = 0.75, .blue = 0.5 };
  g_autofree char *string = kgx_colour_to_string (&colour);

  g_assert_cmpstr ("FFBF7F", ==, string);
}


static void
assert_colour (const GdkRGBA *a, const GdkRGBA *b)
{
  g_assert_cmpfloat_with_epsilon (a->red, b->red, 0.0095);
  g_assert_cmpfloat_with_epsilon (a->green, b->green, 0.0095);
  g_assert_cmpfloat_with_epsilon (a->blue, b->blue, 0.0095);
  g_assert_cmpfloat_with_epsilon (a->alpha, b->alpha, 0.0095);
}


static void
test_colour_from_string (void)
{
  g_autoptr (GError) error = NULL;
  GdkRGBA colour = { .red = 1.0, .green = 0.75, .blue = 0.5 };

  kgx_colour_from_string ("FFBF7F", &colour, &error);

  g_assert_no_error (error);
  assert_colour (&((GdkRGBA) { .red = 1.0, .green = 0.75, .blue = 0.5 }),
                 &colour);

  kgx_colour_from_string ("ABC", &colour, &error);

  g_assert_error (error,
                  KGX_COLOUR_PARSE_ERROR,
                  KGX_COLOUR_PARSE_ERROR_WRONG_LENGTH);
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/kgx/colour-utils/error_quark", test_colour_error_quark);
  g_test_add_func ("/kgx/colour-utils/to_string", test_colour_to_string);
  g_test_add_func ("/kgx/colour-utils/from_string", test_colour_from_string);

  return g_test_run ();
}
