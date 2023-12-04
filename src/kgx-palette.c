/* kgx-palette.c
 *
 * Copyright 2023-2024 Zander Brown
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

#include "kgx-colour-utils.h"
#include "kgx-enums.h"
#include "kgx-palette.h"
#include "kgx-settings.h"

#define PERCENT(v) (CLAMP ((v), 0.0, 1.0))
#define TRANS_AS_ALPHA(t) (PERCENT (1.0 - (t)))
#define ALPHA_AS_TRANS(a) (PERCENT (ABS ((a) - 1.0)))
#define RGB_TYPE "(ddd)"
#define RGB_COLOUR(r, g, b) \
  ((GdkRGBA) { .red = PERCENT (r), .green = PERCENT (g), .blue = PERCENT (b), .alpha = 1.0 })

/* Keyfile Keys */
#define KEY_FOREGROUND "Foreground"
#define KEY_BACKGROUND "Background"
#define KEY_TRANSPARENCY "Transparency"
#define KEY_COLOURS "Colours"


struct _KgxPalette {
  GdkRGBA foreground;
  GdkRGBA background;
  size_t  n_colours;
  GdkRGBA colours[];
};


/**
 * kgx_palette_new:
 * @foreground: (transfer none):
 * @background: (transfer none):
 * @transparency:
 * @n_colours:
 * @colours: (transfer none) (array length=n_colours):
 *
 * Returns: (transfer full): a new #KgxPalette
 */
KgxPalette *
kgx_palette_new (const GdkRGBA *const foreground,
                 const GdkRGBA *const background,
                 double               transparency,
                 size_t               n_colours,
                 const GdkRGBA        colours[const n_colours])
{
  size_t colours_size = sizeof (GdkRGBA) * n_colours;
  KgxPalette *self = g_rc_box_alloc0 (sizeof (KgxPalette) + colours_size);

  self->foreground = *foreground;
  self->background = *background;
  self->background.alpha = TRANS_AS_ALPHA (transparency);
  self->n_colours = n_colours;
  memcpy (&self->colours, colours, colours_size);

  return self;
}


KgxPalette *
kgx_palette_ref (KgxPalette *self)
{
  return g_rc_box_acquire (self);
}


void
kgx_palette_unref (KgxPalette *self)
{
  g_rc_box_release_full (self, NULL);
}


G_DEFINE_BOXED_TYPE (KgxPalette,
                     kgx_palette,
                     kgx_palette_ref,
                     kgx_palette_unref)


KgxPalette *
kgx_palette_deserialise (GVariant *variant)
{
  g_autoptr (GArray) colours = g_array_new (FALSE, FALSE, sizeof (GdkRGBA));
  g_auto (GVariantDict) dict = G_VARIANT_DICT_INIT (variant);
  g_autoptr (GVariantIter) colours_iter = NULL;
  GdkRGBA foreground, background;
  double r, g, b, transparency = 0.0;

  if (G_LIKELY (g_variant_dict_lookup (&dict, "foreground", RGB_TYPE, &r, &g, &b))) {
    foreground = RGB_COLOUR (r, g, b);
  } else {
    foreground = RGB_COLOUR (1.0, 1.0, 1.0);
    g_warning ("palette: (%p) missing foreground", variant);
  }

  if (G_LIKELY (g_variant_dict_lookup (&dict, "background", RGB_TYPE, &r, &g, &b))) {
    background = RGB_COLOUR (r, g, b);
  } else {
    background = RGB_COLOUR (0.12, 0.12, 0.12);
    g_warning ("palette: (%p) missing background", variant);
  }

  if (G_LIKELY (g_variant_dict_lookup (&dict, "colours", "a(ddd)", &colours_iter))) {
    while (g_variant_iter_loop (colours_iter, RGB_TYPE, &r, &g, &b)) {
      g_array_append_val (colours, RGB_COLOUR (r, g, b));
    }
  } else {
    g_warning ("palette: (%p) missing colours", variant);
  }

  g_variant_dict_lookup (&dict, "transparency", "d", &transparency);

  return kgx_palette_new (&foreground,
                          &background,
                          PERCENT (transparency),
                          colours->len,
                          (GdkRGBA *) colours->data);
}


void
kgx_palette_serialise_to (KgxPalette *self, GVariantBuilder *builder)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (builder != NULL);

  g_variant_builder_open (builder, G_VARIANT_TYPE_VARDICT);

  {
    g_variant_builder_open (builder, G_VARIANT_TYPE ("{sv}"));
    g_variant_builder_add (builder, "s", "colours");
    {
      g_variant_builder_open (builder, G_VARIANT_TYPE_VARIANT);
      {
        g_variant_builder_open (builder, G_VARIANT_TYPE ("a" RGB_TYPE));

        /* Add the colour tuples */
        for (size_t i = 0; i < self->n_colours; i++) {
          g_variant_builder_add (builder,
                                 RGB_TYPE,
                                 self->colours[i].red,
                                 self->colours[i].green,
                                 self->colours[i].blue);
        }

        g_variant_builder_close (builder);
      }
      g_variant_builder_close (builder);
    }
    g_variant_builder_close (builder);
  }

  g_variant_builder_add_parsed (builder,
                                "{'foreground', <(%d, %d, %d)>}",
                                self->foreground.red,
                                self->foreground.green,
                                self->foreground.red);
  g_variant_builder_add_parsed (builder,
                                "{'background', <(%d, %d, %d)>}",
                                self->background.red,
                                self->background.green,
                                self->background.red);
  g_variant_builder_add_parsed (builder,
                                "{'transparency', <%d>}",
                                ALPHA_AS_TRANS (self->background.alpha));

  g_variant_builder_close (builder);
}


GVariant *
kgx_palette_serialise (KgxPalette *self)
{
  g_autoptr (GVariant) variant = NULL;
  g_auto (GVariantBuilder) builder =
    G_VARIANT_BUILDER_INIT (G_VARIANT_TYPE_VARIANT);

  g_return_val_if_fail (self != NULL, NULL);

  kgx_palette_serialise_to (self, &builder);

  variant = g_variant_builder_end (&builder);

  return g_variant_get_child_value (variant, 0);
}


static inline void
get_colour (GKeyFile           *key_file,
            const char *const   group,
            const char *const   key,
            GdkRGBA            *colour,
            GError            **error)
{
  g_autoptr (GError) local_error = NULL;
  g_autofree char *string = g_key_file_get_string (key_file,
                                                   group,
                                                   key,
                                                   &local_error);

  if (local_error) {
    g_propagate_error (error, g_steal_pointer (&local_error));
    return;
  }

  kgx_colour_from_string (string, colour, error);
}


static inline double
get_double (GKeyFile           *key_file,
            const char *const   group,
            const char *const   key,
            gboolean            required,
            GError            **error)
{
  g_autoptr (GError) local_error = NULL;
  double value = g_key_file_get_double (key_file,
                                        group,
                                        key,
                                        &local_error);

  /* We didn't acutally need this key, so …just move on */
  if (local_error && !required) {
    return 0.0;
  } else if (local_error) {
    g_propagate_error (error, g_steal_pointer (&local_error));
    return 0.0;
  }

  return value;
}


static inline GdkRGBA *
get_colours (GKeyFile           *key_file,
             const char *const   group,
             const char *const   key,
             size_t             *n_colours,
             GError            **error)
{
  g_autoptr (GError) local_error = NULL;
  g_autoptr (GArray) colours = NULL;
  g_auto (GStrv) strings = g_key_file_get_string_list (key_file,
                                                       group,
                                                       key,
                                                       n_colours,
                                                       error);

  if (local_error) {
    g_propagate_error (error, g_steal_pointer (&local_error));
    return NULL;
  }

  colours = g_array_sized_new (FALSE, FALSE, sizeof (GdkRGBA), *n_colours);

  for (size_t i = 0; i < *n_colours; i++) {
    GdkRGBA colour = {};

    kgx_colour_from_string (strings[i], &colour, &local_error);
    if (*error) {
      g_propagate_error (error, g_steal_pointer (&local_error));
      return NULL;
    }

    g_array_append_val (colours, colour);
  }

  return g_array_steal (colours, n_colours);
}


KgxPalette *
kgx_palette_new_from_group (GKeyFile           *key_file,
                            const char *const   group,
                            GError            **error)
{
  g_autoptr (GError) local_error = NULL;
  g_autofree GdkRGBA *colours = NULL;
  GdkRGBA foreground, background;
  double transparency = 0.0;
  size_t n_colours = 0;

  g_return_val_if_fail (key_file != NULL, NULL);
  g_return_val_if_fail (group != NULL, NULL);
  g_return_val_if_fail (error != NULL, NULL);

  get_colour (key_file, group, KEY_FOREGROUND, &foreground, &local_error);
  if (local_error) {
    g_propagate_error (error, g_steal_pointer (&local_error));
    return NULL;
  }

  get_colour (key_file, group, KEY_BACKGROUND, &background, &local_error);
  if (local_error) {
    g_propagate_error (error, g_steal_pointer (&local_error));
    return NULL;
  }

  transparency = get_double (key_file, group, KEY_TRANSPARENCY, FALSE, &local_error);
  if (local_error) {
    g_propagate_error (error, g_steal_pointer (&local_error));
    return NULL;
  }

  colours = get_colours (key_file, group, KEY_COLOURS, &n_colours, &local_error);
  if (local_error) {
    g_propagate_error (error, g_steal_pointer (&local_error));
    return NULL;
  }

  return kgx_palette_new (&foreground,
                          &background,
                          PERCENT (transparency),
                          n_colours,
                          colours);
}


void
kgx_palette_export_to_group (KgxPalette       *self,
                             GKeyFile         *key_file,
                             const char *const group)
{
  g_autoptr (GStrvBuilder) builder = g_strv_builder_new ();
  g_auto (GStrv) colours = NULL;
  g_autofree char *foreground_string = NULL;
  g_autofree char *background_string = NULL;
  size_t n_colours = 0;

  g_return_if_fail (self != NULL);
  g_return_if_fail (key_file != NULL);
  g_return_if_fail (group != NULL);

  foreground_string = kgx_colour_to_string (&self->foreground);
  g_key_file_set_string (key_file, group, KEY_FOREGROUND, foreground_string);

  background_string = kgx_colour_to_string (&self->foreground);
  g_key_file_set_string (key_file, group, KEY_BACKGROUND, background_string);

  g_key_file_set_double (key_file,
                         group,
                         KEY_TRANSPARENCY,
                         ALPHA_AS_TRANS (self->background.alpha));

  for (size_t i = 0; i < self->n_colours; i++) {
    g_autofree char *string = kgx_colour_to_string (&self->colours[i]);

    g_strv_builder_take (builder, g_steal_pointer (&string));
  }

  colours = g_strv_builder_end (builder);
  n_colours = g_strv_length (colours);

  g_key_file_set_string_list (key_file,
                              group,
                              KEY_COLOURS,
                              (const char *const *) colours,
                              n_colours);
}


/**
 * kgx_palette_as_opaque:
 * @self: a #KgxPalette
 *
 * Returns: (transfer full): a new #KgxPalette without transparency, or @self
 * if it was already opaque
 */
KgxPalette *
kgx_palette_as_opaque (KgxPalette *self)
{
  KgxPalette *modified;
  size_t colours_size;

  g_return_val_if_fail (self != NULL, NULL);

  if (G_APPROX_VALUE (self->background.alpha, 1.0, FLT_EPSILON)) {
    return kgx_palette_ref (self);
  }

  colours_size = sizeof (GdkRGBA) * self->n_colours;
  modified = g_rc_box_dup (sizeof (KgxPalette) + colours_size, self);
  modified->background.alpha = 1.0;

  return modified;
}


double
kgx_palette_get_transparency (KgxPalette *self)
{
  g_return_val_if_fail (self != NULL, 0.0);

  return ALPHA_AS_TRANS (self->background.alpha);
}


void
kgx_palette_get_colours (KgxPalette    *self,
                         GdkRGBA       *const foreground,
                         GdkRGBA       *const background,
                         size_t        *const n_colours,
                         const GdkRGBA *colours[const *n_colours])
{
  g_return_if_fail (self != NULL);

  *foreground = self->foreground;
  *background = self->background;
  *n_colours = self->n_colours;
  *colours = self->colours;
}
