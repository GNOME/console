/* kgx-livery.h
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

#include <gio/gio.h>

#include "kgx-palette.h"

G_BEGIN_DECLS

typedef struct _KgxLivery KgxLivery;

#define KGX_TYPE_LIVERY (kgx_livery_get_type ())

KgxLivery     *kgx_livery_new                (const char           *uuid,
                                              const char           *name,
                                              KgxPalette           *night,
                                              KgxPalette           *day);
GType          kgx_livery_get_type           (void);
KgxLivery     *kgx_livery_ref                (KgxLivery            *self);
void           kgx_livery_unref              (KgxLivery            *self);
const char    *kgx_livery_get_uuid           (KgxLivery            *self);
const char    *kgx_livery_get_name           (KgxLivery            *self);
KgxPalette    *kgx_livery_get_night          (KgxLivery            *self);
KgxPalette    *kgx_livery_get_day            (KgxLivery            *self);
KgxLivery     *kgx_livery_deserialise        (GVariant             *variant);
void           kgx_livery_serialise_to       (KgxLivery            *self,
                                              GVariantBuilder      *builder);
GVariant      *kgx_livery_serialise          (KgxLivery            *self);
void           kgx_livery_import_from        (GFile                *file,
                                              GCancellable         *cancellable,
                                              GAsyncReadyCallback   callback,
                                              gpointer              user_data);
KgxLivery     *kgx_livery_import_from_finish (GAsyncResult         *res,
                                              GError              **error);
void           kgx_livery_export_to          (KgxLivery            *self,
                                              GFile                *file,
                                              GCancellable         *cancellable,
                                              GAsyncReadyCallback   callback,
                                              gpointer              user_data);
void           kgx_livery_export_to_finish   (KgxLivery            *self,
                                              GAsyncResult         *res,
                                              GError              **error);
KgxPalette    *kgx_livery_resolve            (KgxLivery            *self,
                                              gboolean              is_day,
                                              gboolean              translucency);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (KgxLivery, kgx_livery_unref)

static inline gboolean
kgx_set_livery (KgxLivery **livery_ptr,
                KgxLivery  *new_livery)
{
  KgxLivery *old_livery = *livery_ptr;

  if (old_livery == new_livery) {
    return FALSE;
  }

  *livery_ptr = new_livery ? kgx_livery_ref (new_livery) : NULL;

  if (old_livery != NULL) {
    kgx_livery_unref (old_livery);
  }

  return TRUE;
}

G_END_DECLS
