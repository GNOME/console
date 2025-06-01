/* test-livery.c
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

#include "kgx-livery.h"


static void
test_livery_type (void)
{
  g_assert_true (G_TYPE_IS_BOXED (KGX_TYPE_LIVERY));
  g_assert_cmpstr ("KgxLivery", ==, g_type_name (KGX_TYPE_LIVERY));
}


static void
test_livery_new (void)
{
  GdkRGBA fg = {};
  GdkRGBA bg = {};
  g_autoptr (KgxPalette) palette = kgx_palette_new (&fg,
                                                    &bg,
                                                    0.0,
                                                    0,
                                                    ((GdkRGBA []) { 0, }));
  g_autoptr (KgxLivery) livery = kgx_livery_new (KGX_LIVERY_UUID_KGX, "Test", NULL, NULL);

  kgx_livery_ref (livery);
  kgx_livery_unref (livery);
}


static void
assert_colour (const GdkRGBA *a, const GdkRGBA *b)
{
  g_assert_cmpfloat (a->red, ==, b->red);
  g_assert_cmpfloat (a->green, ==, b->green);
  g_assert_cmpfloat (a->blue, ==, b->blue);
  g_assert_cmpfloat (a->alpha, ==, b->alpha);
}


static void
test_livery_roundtrip (void)
{
  g_autoptr (KgxPalette) palette = NULL;
  g_autoptr (KgxLivery) livery = NULL;
  g_autoptr (GVariant) variant = NULL;
  g_autoptr (KgxLivery) livery_after = NULL;
  g_autoptr (KgxPalette) palette_after = NULL;
  GdkRGBA fg_in = {.alpha = 1.0}, fg_out = {}, bg_in = { .alpha = 1.0 }, bg_out = {};
  GdkRGBA colours_in[] = { {.alpha = 1.0}, };
  const GdkRGBA *colours_out = NULL;
  size_t n_colours_in = G_N_ELEMENTS (colours_in), n_colours_out = 0;

  palette = kgx_palette_new (&fg_in,
                             &bg_in,
                             0.0,
                             n_colours_in,
                             colours_in);
  livery = kgx_livery_new (KGX_LIVERY_UUID_KGX, "Test", palette, NULL);
  variant = kgx_livery_serialise (livery);

  livery_after = kgx_livery_deserialise (variant);

  g_assert_cmpstr (kgx_livery_get_uuid (livery),
                   ==,
                   kgx_livery_get_uuid (livery_after));
  g_assert_cmpstr (kgx_livery_get_name (livery),
                   ==,
                   kgx_livery_get_name (livery_after));

  palette_after = kgx_livery_resolve (livery_after, FALSE, TRUE);
  kgx_palette_get_colours (palette_after, &fg_out, &bg_out, &n_colours_out, &colours_out);

  g_assert_cmpint (n_colours_in, ==, n_colours_out);

  assert_colour (&fg_in, &fg_out);
  assert_colour (&bg_in, &bg_out);

  for (size_t i = 0; i < n_colours_out; i++) {
    assert_colour (&colours_in[i], &colours_out[i]);
  }
}


static void
test_livery_set_livery (void)
{
  g_autoptr (KgxPalette) palette = kgx_palette_new (&((GdkRGBA) { 0, }),
                                                    &((GdkRGBA) { 0, }),
                                                    0.0,
                                                    0,
                                                    ((GdkRGBA []) { 0, }));
  g_autoptr (KgxLivery) livery_a = kgx_livery_new (KGX_LIVERY_UUID_KGX,
                                                   "Test",
                                                   palette,
                                                   palette);
  g_autoptr (KgxLivery) livery_b = kgx_livery_new (KGX_LIVERY_UUID_KGX,
                                                   "Test",
                                                   palette,
                                                   palette);

  g_assert_true (kgx_set_livery (&livery_a, livery_b));
  g_assert_true (livery_a == livery_b);
}


static void
test_livery_set_livery_same (void)
{
  g_autoptr (KgxPalette) palette = kgx_palette_new (&((GdkRGBA) { 0, }),
                                                    &((GdkRGBA) { 0, }),
                                                    0.0,
                                                    0,
                                                    ((GdkRGBA []) { 0, }));
  g_autoptr (KgxLivery) livery = kgx_livery_new (KGX_LIVERY_UUID_KGX,
                                                 "Test",
                                                 palette,
                                                 palette);

  g_assert_false (kgx_set_palette (&palette, palette));
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/kgx/livery/type", test_livery_type);
  g_test_add_func ("/kgx/livery/new", test_livery_new);
  g_test_add_func ("/kgx/livery/roundtrip", test_livery_roundtrip);
  g_test_add_func ("/kgx/livery/set_palette", test_livery_set_livery);
  g_test_add_func ("/kgx/livery/set_palette_same", test_livery_set_livery_same);

  return g_test_run ();
}
