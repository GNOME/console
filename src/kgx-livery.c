/* kgx-livery.c
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

#include "kgx-livery.h"
#include "kgx-settings.h"
#include "kgx-enums.h"

/* Keyfile Keys */
#define META_GROUP "Livery"
#define KEY_UUID "UUID"
#define KEY_NAME "Name"


struct _KgxLivery {
  char       *uuid;
  char       *name;
  KgxPalette *night;
  KgxPalette *night_opaque;
  KgxPalette *day;
  KgxPalette *day_opaque;
};

G_DEFINE_BOXED_TYPE (KgxLivery,
                     kgx_livery,
                     kgx_livery_ref,
                     kgx_livery_unref)


/**
 * kgx_livery_new:
 * @uuid: (transfer none) (optional): the uuid of this livery, or %NULL if
 *                                    this is a new livery
 * @night: (transfer none): a #KgxPalette for night mode
 * @day: (transfer none) (optional): an optional alternate #KgxPalette for
 *                                   day mode
 *
 * Returns: (transfer full): a new #KgxLivery
*/
KgxLivery *
kgx_livery_new (const char *uuid,
                const char *name,
                KgxPalette *night,
                KgxPalette *day)
{
  KgxLivery *self = g_rc_box_new0 (KgxLivery);

  if (!g_set_str (&self->uuid, uuid)) {
    self->uuid = g_uuid_string_random ();
  }
  g_set_str (&self->name, name);
  kgx_set_palette (&self->night, night);
  kgx_set_palette (&self->day, day);

  return self;
}


KgxLivery *
kgx_livery_ref (KgxLivery *self)
{
  return g_rc_box_acquire (self);
}


static void
clear_livery (gpointer livery)
{
  KgxLivery *self = livery;

  g_clear_pointer (&self->uuid, g_free);
  g_clear_pointer (&self->name, g_free);
  g_clear_pointer (&self->night, kgx_palette_unref);
  g_clear_pointer (&self->night_opaque, kgx_palette_unref);
  g_clear_pointer (&self->day, kgx_palette_unref);
  g_clear_pointer (&self->day_opaque, kgx_palette_unref);
}


void
kgx_livery_unref (KgxLivery *self)
{
  g_rc_box_release_full (self, clear_livery);
}


const char *
kgx_livery_get_uuid (KgxLivery *self)
{
  g_return_val_if_fail (self != NULL, NULL);

  return self->uuid;
}


const char *
kgx_livery_get_name (KgxLivery *self)
{
  g_return_val_if_fail (self != NULL, NULL);

  return self->name;
}


KgxPalette *
kgx_livery_get_night (KgxLivery *self)
{
  g_return_val_if_fail (self != NULL, NULL);

  return self->night;
}


KgxPalette *
kgx_livery_get_day (KgxLivery *self)
{
  g_return_val_if_fail (self != NULL, NULL);

  return self->day;
}


KgxLivery *
kgx_livery_deserialise (GVariant *variant)
{
  g_auto (GVariantDict) dict = G_VARIANT_DICT_INIT (variant);
  g_autoptr (GVariant) night_variant = NULL;
  g_autoptr (GVariant) day_variant = NULL;
  g_autoptr (KgxPalette) night = NULL;
  g_autoptr (KgxPalette) day = NULL;
  const char *uuid = NULL;
  const char *name = NULL;

  if (G_UNLIKELY (!g_variant_dict_lookup (&dict,
                                          "uuid",
                                          "&s", &uuid))) {
    g_warning ("livery: (%p) missing uuid", variant);
    return NULL;
  }

  g_variant_dict_lookup (&dict, "name", "&s", &name);

  if (G_UNLIKELY (!g_variant_dict_lookup (&dict,
                                          "night",
                                          "@*", &night_variant))) {
    g_warning ("livery: (%p) missing night", variant);
    return NULL;
  }

  night = kgx_palette_deserialise (night_variant);

  if (g_variant_dict_lookup (&dict, "day", "@*", &day_variant)) {
    day = kgx_palette_deserialise (day_variant);
  }

  return kgx_livery_new (uuid, name, night, day);
}


void
kgx_livery_serialise_to (KgxLivery *self, GVariantBuilder *builder)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (builder != NULL);

  g_variant_builder_open (builder, G_VARIANT_TYPE_VARDICT);

  g_variant_builder_add_parsed (builder, "{'uuid', <%s>}", self->uuid);
  g_variant_builder_add_parsed (builder, "{'name', <%s>}", self->name);

  {
    g_variant_builder_open (builder, G_VARIANT_TYPE ("{sv}"));
    g_variant_builder_add (builder, "s", "night");

    {
      g_variant_builder_open (builder, G_VARIANT_TYPE_VARIANT);
      kgx_palette_serialise_to (self->night, builder);
      g_variant_builder_close (builder);
    }

    g_variant_builder_close (builder);
  }

  if (self->day) {
    g_variant_builder_open (builder, G_VARIANT_TYPE ("{sv}"));
    g_variant_builder_add (builder, "s", "day");

    {
      g_variant_builder_open (builder, G_VARIANT_TYPE_VARIANT);
      kgx_palette_serialise_to (self->day, builder);
      g_variant_builder_close (builder);
    }

    g_variant_builder_close (builder);
  }

  g_variant_builder_close (builder);
}


GVariant *
kgx_livery_serialise (KgxLivery *self)
{
  g_autoptr (GVariant) variant = NULL;
  g_auto (GVariantBuilder) builder =
    G_VARIANT_BUILDER_INIT (G_VARIANT_TYPE_VARIANT);

  g_return_val_if_fail (self != NULL, NULL);

  kgx_livery_serialise_to (self, &builder);

  variant = g_variant_builder_end (&builder);

  return g_variant_get_child_value (variant, 0);
}


static void
got_content (GObject *source, GAsyncResult *res, gpointer data)
{
  g_autoptr (GError) error = NULL;
  g_autoptr (GTask) task = data;
  g_autoptr (GKeyFile) key_file = g_key_file_new ();
  g_autoptr (GBytes) bytes = NULL;
  g_autoptr (KgxPalette) night = NULL;
  g_autoptr (KgxPalette) day = NULL;
  g_autofree char *uuid = NULL;
  g_autofree char *name = NULL;

  bytes = g_file_load_bytes_finish (G_FILE (source), res, NULL, &error);
  if (error) {
    g_task_return_error (task, g_steal_pointer (&error));
    return;
  }

  g_key_file_load_from_bytes (key_file, bytes, G_KEY_FILE_NONE, &error);
  if (error) {
    g_task_return_error (task, g_steal_pointer (&error));
    return;
  }

  uuid = g_key_file_get_string (key_file, META_GROUP, KEY_UUID, &error);
  if (error) {
    g_task_return_error (task, g_steal_pointer (&error));
    return;
  }

  name = g_key_file_get_string (key_file, META_GROUP, KEY_NAME, &error);
  if (g_error_matches (error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_KEY_NOT_FOUND)) {
    g_clear_error (&error);
  } else if (error) {
    g_task_return_error (task, g_steal_pointer (&error));
    return;
  }

  night = kgx_palette_new_from_group (key_file, "Night", &error);
  if (error) {
    g_task_return_error (task, g_steal_pointer (&error));
    return;
  }

  if (g_key_file_has_group (key_file, "Day")) {
    day = kgx_palette_new_from_group (key_file, "Day", &error);
    if (error) {
      g_task_return_error (task, g_steal_pointer (&error));
      return;
    }
  }

  g_task_return_pointer (task,
                         kgx_livery_new (uuid, name, night, day),
                         (GDestroyNotify) kgx_livery_unref);
}


void
kgx_livery_import_from (GFile               *file,
                        GCancellable        *cancellable,
                        GAsyncReadyCallback  callback,
                        gpointer             user_data)
{
  g_autoptr (GTask) task = NULL;
  GCancellable *task_cancellable;

  task = g_task_new (NULL, cancellable, callback, user_data);
  g_task_set_source_tag (task, kgx_livery_import_from);

  task_cancellable = g_task_get_cancellable (task);

  g_file_load_bytes_async (file,
                           task_cancellable,
                           got_content,
                           g_steal_pointer (&task));
}


KgxLivery *
kgx_livery_import_from_finish (GAsyncResult *res, GError **error)
{
  return g_task_propagate_pointer (G_TASK (res), error);
}


static void
did_write (GObject *source, GAsyncResult *res, gpointer data)
{
  g_autoptr (GError) error = NULL;
  g_autoptr (GTask) task = data;

  g_file_replace_contents_finish (G_FILE (source), res, NULL, &error);

  if (error) {
    g_task_return_error (task, g_steal_pointer (&error));
    return;
  }

  g_task_return_int (task, 0);
}


void
kgx_livery_export_to (KgxLivery           *self,
                      GFile               *file,
                      GCancellable        *cancellable,
                      GAsyncReadyCallback  callback,
                      gpointer             user_data)
{
  g_autoptr (GError) error = NULL;
  g_autoptr (GKeyFile) key_file = g_key_file_new ();
  g_autoptr (GBytes) bytes = NULL;
  g_autoptr (GTask) task = NULL;
  g_autofree char *data = NULL;
  GCancellable *task_cancellable;
  size_t length = 0;

  task = g_task_new (NULL, cancellable, callback, user_data);
  g_task_set_source_tag (task, kgx_livery_export_to);

  task_cancellable = g_task_get_cancellable (task);

  g_key_file_set_comment (key_file, NULL, NULL, "KGX Livery", &error);

  g_key_file_set_string (key_file, META_GROUP, KEY_UUID, self->uuid);
  if (G_LIKELY (self->name)) {
    g_key_file_set_string (key_file, META_GROUP, KEY_NAME, self->name);
  }

  kgx_palette_export_to_group (self->night, key_file, "Night");
  if (G_LIKELY (self->day)) {
    kgx_palette_export_to_group (self->day, key_file, "Day");
  }

  data = g_key_file_to_data (key_file, &length, &error);
  bytes = g_bytes_new_take (g_steal_pointer (&data), length);

  g_file_replace_contents_bytes_async (file,
                                       bytes,
                                       NULL,
                                       TRUE,
                                       G_FILE_CREATE_REPLACE_DESTINATION | G_FILE_CREATE_PRIVATE,
                                       task_cancellable,
                                       did_write,
                                       g_steal_pointer (&task));
}


void
kgx_livery_export_to_finish (KgxLivery     *self,
                             GAsyncResult  *res,
                             GError       **error)
{
  g_task_propagate_int (G_TASK (res), error);
}


static inline KgxPalette *
maybe_as_opaque (KgxPalette  *base,
                 KgxPalette **cache,
                 gboolean     translucency)
{
  if (G_LIKELY (!translucency)) {
    if (G_UNLIKELY (*cache == NULL)) {
      *cache = kgx_palette_as_opaque (base);
    }
    return kgx_palette_ref (*cache);
  } else {
    return kgx_palette_ref (base);
  }
}


/**
 * kgx_livery_resolve:
 *
 * Returns: (transfer full): a #KgxPalette for the given arguments
 */
KgxPalette *
kgx_livery_resolve (KgxLivery     *self,
                    gboolean       is_day,
                    gboolean       translucency)
{
  g_return_val_if_fail (self != NULL, NULL);

  if (is_day && self->day) {
    return maybe_as_opaque (self->day, &self->day_opaque, translucency);
  } else {
    return maybe_as_opaque (self->night, &self->night_opaque, translucency);
  }
}
