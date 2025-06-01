/* test-palette.c
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

#include "rgba.h"

#include "kgx-palette.h"


static void
test_palette_type (void)
{
  g_assert_true (G_TYPE_IS_BOXED (KGX_TYPE_PALETTE));
  g_assert_cmpstr ("KgxPalette", ==, g_type_name (KGX_TYPE_PALETTE));
}


static void
test_palette_new (void)
{
  g_autoptr (KgxPalette) palette = kgx_palette_new (&((GdkRGBA) { 0, }),
                                                    &((GdkRGBA) { 0, }),
                                                    0.0,
                                                    0,
                                                    ((GdkRGBA []) { 0, }));

  kgx_palette_ref (palette);
  kgx_palette_unref (palette);
}


static void
test_palette_transparency (void)
{
  g_autoptr (KgxPalette) palette = kgx_palette_new (&((GdkRGBA) { 0, }),
                                                    &((GdkRGBA) { 0, }),
                                                    0.5,
                                                    0,
                                                    ((GdkRGBA []) { 0, }));
  g_autoptr (KgxPalette) opaque_palette = NULL;

  g_assert_cmpfloat (kgx_palette_get_transparency (palette), ==, 0.5);

  opaque_palette = kgx_palette_as_opaque (palette);
  g_assert_true (palette != opaque_palette);

  g_assert_cmpfloat (kgx_palette_get_transparency (opaque_palette), ==, 0.0);
}


static void
test_palette_opaque (void)
{
  g_autoptr (KgxPalette) palette = kgx_palette_new (&((GdkRGBA) { 0, }),
                                                    &((GdkRGBA) { 0, }),
                                                    0.0,
                                                    0,
                                                    ((GdkRGBA []) { 0, }));
  g_autoptr (KgxPalette) opaque_palette = NULL;

  g_assert_cmpfloat (kgx_palette_get_transparency (palette), ==, 0.0);

  opaque_palette = kgx_palette_as_opaque (palette);
  g_assert_true (palette == opaque_palette);

  g_assert_cmpfloat (kgx_palette_get_transparency (opaque_palette), ==, 0.0);
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
test_palette_colours (void)
{
  GdkRGBA colours[] = {
    GDK_RGBA ("613583"),
    GDK_RGBA ("a51d2d"),
    GDK_RGBA ("26a269"),
  };
  GdkRGBA fg = GDK_RGBA ("deddda");
  GdkRGBA bg = GDK_RGBA ("3d3846");
  g_autoptr (KgxPalette) palette = kgx_palette_new (&fg,
                                                    &bg,
                                                    0.0,
                                                    G_N_ELEMENTS (colours),
                                                    colours);
  GdkRGBA fg_out, bg_out;
  const GdkRGBA *colours_out = NULL;
  size_t n_colours;

  kgx_palette_get_colours (palette, &fg_out, &bg_out, &n_colours, &colours_out);

  assert_colour (&fg, &fg_out);
  assert_colour (&bg, &bg_out);
  g_assert_cmpint (G_N_ELEMENTS (colours), ==, n_colours);

  for (size_t i = 0; i < n_colours; i++) {
    assert_colour (&colours[i], &colours_out[i]);
  }
}


static inline GVariant *
valid_variant (void)
{
  return g_variant_new_parsed ("{'colours': <[(1.0, 0.0, 0.0), (0.0, 0.0, 1.0)]>, "
                               "'foreground': <(1.0, 1.0, 1.0)>, "
                               "'background': <(0.0, 0.0, 0.0)>, "
                               "'transparency': <0.5>}");
}


static void
test_palette_serialise (void)
{
  GdkRGBA colours[] = {
    GDK_RGBA ("FF0000"),
    GDK_RGBA ("0000FF"),
  };
  GdkRGBA fg = GDK_RGBA ("FFFFFF");
  GdkRGBA bg = GDK_RGBA ("000000");
  g_autoptr (KgxPalette) palette = kgx_palette_new (&fg,
                                                    &bg,
                                                    0.5,
                                                    G_N_ELEMENTS (colours),
                                                    colours);
  g_autoptr (GVariant) valid = valid_variant ();
  g_autoptr (GVariant) result = kgx_palette_serialise (palette);

  g_assert_cmpvariant (valid, result);
}


static void
test_palette_deserialise (void)
{
  g_autoptr (KgxPalette) palette = NULL;
  g_autoptr (GVariant) valid = valid_variant ();
  GdkRGBA colours[] = {
    GDK_RGBA ("FF0000"),
    GDK_RGBA ("0000FF"),
  };
  GdkRGBA fg = GDK_RGBA ("FFFFFF");
  GdkRGBA bg = { .red = 0.0, .green = 0.0, .blue = 0.0, .alpha = 0.5 };
  GdkRGBA fg_out, bg_out;
  const GdkRGBA *colours_out = NULL;
  size_t n_colours;

  palette = kgx_palette_deserialise (valid);

  g_assert_cmpfloat (kgx_palette_get_transparency (palette), ==, 0.5);

  kgx_palette_get_colours (palette, &fg_out, &bg_out, &n_colours, &colours_out);

  assert_colour (&fg, &fg_out);
  assert_colour (&bg, &bg_out);
  g_assert_cmpint (G_N_ELEMENTS (colours), ==, n_colours);

  for (size_t i = 0; i < n_colours; i++) {
    assert_colour (&colours[i], &colours_out[i]);
  }
}


static void
test_palette_set_palette (void)
{
  g_autoptr (KgxPalette) palette_a = kgx_palette_new (&((GdkRGBA) { 0, }),
                                                      &((GdkRGBA) { 0, }),
                                                      0.0,
                                                      0,
                                                      ((GdkRGBA []) { 0, }));
  g_autoptr (KgxPalette) palette_b = kgx_palette_new (&((GdkRGBA) { 0, }),
                                                      &((GdkRGBA) { 0, }),
                                                      0.0,
                                                      0,
                                                      ((GdkRGBA []) { 0, }));


  g_assert_true (kgx_set_palette (&palette_a, palette_b));
  g_assert_true (palette_a == palette_b);
}


static void
test_palette_set_palette_same (void)
{
  g_autoptr (KgxPalette) palette = kgx_palette_new (&((GdkRGBA) { 0, }),
                                                    &((GdkRGBA) { 0, }),
                                                    0.0,
                                                    0,
                                                    ((GdkRGBA []) { 0, }));

  g_assert_false (kgx_set_palette (&palette, palette));
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/kgx/palette/type", test_palette_type);
  g_test_add_func ("/kgx/palette/new", test_palette_new);
  g_test_add_func ("/kgx/palette/transparency", test_palette_transparency);
  g_test_add_func ("/kgx/palette/opaque", test_palette_opaque);
  g_test_add_func ("/kgx/palette/colours", test_palette_colours);
  g_test_add_func ("/kgx/palette/serialise", test_palette_serialise);
  g_test_add_func ("/kgx/palette/deserialise", test_palette_deserialise);
  g_test_add_func ("/kgx/palette/set_palette", test_palette_set_palette);
  g_test_add_func ("/kgx/palette/set_palette_same", test_palette_set_palette_same);

  return g_test_run ();
}
