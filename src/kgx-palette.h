/* kgx-palette.h
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

#pragma once

#include <gdk/gdk.h>

G_BEGIN_DECLS

#define KGX_TYPE_PALETTE (kgx_palette_get_type ())

typedef struct _KgxPalette KgxPalette;

KgxPalette    *kgx_palette_new               (const GdkRGBA   *const foreground,
                                              const GdkRGBA   *const background,
                                              double                 transparency,
                                              size_t                 n_colours,
                                              const GdkRGBA          colours[const n_colours]);
GType          kgx_palette_get_type          (void);
KgxPalette    *kgx_palette_ref               (KgxPalette            *self);
void           kgx_palette_unref             (KgxPalette            *self);
KgxPalette    *kgx_palette_deserialise       (GVariant              *variant);
void           kgx_palette_serialise_to      (KgxPalette            *self,
                                              GVariantBuilder       *builder);
GVariant      *kgx_palette_serialise         (KgxPalette            *self);
KgxPalette    *kgx_palette_new_from_group    (GKeyFile              *key_file,
                                              const char      *const group,
                                              GError               **error);
void           kgx_palette_export_to_group   (KgxPalette            *self,
                                              GKeyFile              *key_file,
                                              const char      *const group);
KgxPalette    *kgx_palette_as_opaque         (KgxPalette            *self);
double         kgx_palette_get_transparency  (KgxPalette            *self);
void           kgx_palette_get_colours       (KgxPalette            *self,
                                              GdkRGBA         *const foreground,
                                              GdkRGBA         *const background,
                                              size_t          *const n_colours,
                                              const GdkRGBA         *colours[const *n_colours]);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (KgxPalette, kgx_palette_unref)

static inline gboolean
kgx_set_palette (KgxPalette **palette_ptr,
                 KgxPalette  *new_palette)
{
  KgxPalette *old_palette = *palette_ptr;

  if (old_palette == new_palette) {
    return FALSE;
  }

  *palette_ptr = new_palette ? kgx_palette_ref (new_palette) : NULL;

  if (old_palette != NULL) {
    kgx_palette_unref (old_palette);
  }

  return TRUE;
}

G_END_DECLS
