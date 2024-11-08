/* kgx-livery-manager.c
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

#include <glib/gi18n.h>

#include "rgba.h"

#include "kgx-livery-manager.h"


struct _KgxLiveryManager {
  GObject     parent;

  KgxLivery  *fallback;

  GHashTable *custom_liveries;
  GHashTable *builtin_liveries;
};


G_DEFINE_FINAL_TYPE (KgxLiveryManager, kgx_livery_manager, G_TYPE_OBJECT)


enum {
  PROP_0,
  PROP_CUSTOM_LIVERIES,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };


static void
kgx_livery_manager_dispose (GObject *object)
{
  KgxLiveryManager *self = KGX_LIVERY_MANAGER (object);

  g_clear_pointer (&self->fallback, kgx_livery_unref);

  g_clear_pointer (&self->custom_liveries, g_hash_table_unref);
  g_clear_pointer (&self->builtin_liveries, g_hash_table_unref);

  G_OBJECT_CLASS (kgx_livery_manager_parent_class)->dispose (object);
}


static void
kgx_livery_manager_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  KgxLiveryManager *self = KGX_LIVERY_MANAGER (object);

  switch (property_id) {
    case PROP_CUSTOM_LIVERIES:
      kgx_livery_manager_set_custom_liveries (self,
                                              g_value_get_variant (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
kgx_livery_manager_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  KgxLiveryManager *self = KGX_LIVERY_MANAGER (object);

  switch (property_id) {
    case PROP_CUSTOM_LIVERIES:
      g_value_set_variant (value,
                           kgx_livery_manager_get_custom_liveries (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
kgx_livery_manager_class_init (KgxLiveryManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS   (klass);

  object_class->dispose = kgx_livery_manager_dispose;
  object_class->set_property = kgx_livery_manager_set_property;
  object_class->get_property = kgx_livery_manager_get_property;

  pspecs[PROP_CUSTOM_LIVERIES] =
    g_param_spec_variant ("custom-liveries", NULL, NULL,
                          G_VARIANT_TYPE_VARDICT,
                          NULL,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);
}


static inline KgxLivery *
standard_livery (void)
{
  g_autoptr (KgxPalette) night = NULL;
  g_autoptr (KgxPalette) day = NULL;
  GdkRGBA night_colours[16] = {
    GDK_RGBA ("241f31"), /* Black          */
    GDK_RGBA ("c01c28"), /* Red            */
    GDK_RGBA ("2ec27e"), /* Green          */
    GDK_RGBA ("f5c211"), /* Yellow         */
    GDK_RGBA ("1e78e4"), /* Blue           */
    GDK_RGBA ("9841bb"), /* Magenta        */
    GDK_RGBA ("0ab9dc"), /* Cyan           */
    GDK_RGBA ("c0bfbc"), /* White          */
    GDK_RGBA ("5e5c64"), /* Bright Black   */
    GDK_RGBA ("ed333b"), /* Bright Red     */
    GDK_RGBA ("57e389"), /* Bright Green   */
    GDK_RGBA ("f8e45c"), /* Bright Yellow  */
    GDK_RGBA ("51a1ff"), /* Bright Blue    */
    GDK_RGBA ("c061cb"), /* Bright Magenta */
    GDK_RGBA ("4fd2fd"), /* Bright Cyan    */
    GDK_RGBA ("f6f5f4"), /* Bright White   */
  };

  night = kgx_palette_new (&((GdkRGBA) { 1.0, 1.0, 1.0, 1.0 }),
                           &((GdkRGBA) { 0.11, 0.11, 0.12, 1.0 }),
                           0.04,
                           G_N_ELEMENTS (night_colours),
                           night_colours);

  day = kgx_palette_new (&((GdkRGBA) { 0.0, 0.0, 0.0, 1.0 }),
                         &((GdkRGBA) { 1.0, 1.0, 1.0, 1.0 }),
                         0.01,
                         /* TODO: Have some day colours */
                         G_N_ELEMENTS (night_colours),
                         night_colours);

  return kgx_livery_new (KGX_LIVERY_UUID_KGX, NULL, night, day);
}


static inline KgxLivery *
linux_livery (void)
{
  g_autoptr (KgxPalette) night = NULL;
  GdkRGBA colours[16] = {
    GDK_RGBA ("000000"), /* Black          */
    GDK_RGBA ("aa0000"), /* Red            */
    GDK_RGBA ("00aa00"), /* Green          */
    GDK_RGBA ("aa5500"), /* Yellow         */
    GDK_RGBA ("0000aa"), /* Blue           */
    GDK_RGBA ("aa00aa"), /* Magenta        */
    GDK_RGBA ("00aaaa"), /* Cyan           */
    GDK_RGBA ("aaaaaa"), /* White          */
    GDK_RGBA ("555555"), /* Bright Black   */
    GDK_RGBA ("ff5555"), /* Bright Red     */
    GDK_RGBA ("55ff55"), /* Bright Green   */
    GDK_RGBA ("ffff55"), /* Bright Yellow  */
    GDK_RGBA ("5555ff"), /* Bright Blue    */
    GDK_RGBA ("ff55ff"), /* Bright Magenta */
    GDK_RGBA ("55ffff"), /* Bright Cyan    */
    GDK_RGBA ("ffffff"), /* Bright White   */
  };

  night = kgx_palette_new (&((GdkRGBA) { 1.0, 1.0, 1.0, 1.0 }),
                           &((GdkRGBA) { 0.0, 0.0, 0.0, 1.0 }),
                           0.0,
                           G_N_ELEMENTS (colours),
                           colours);

  return kgx_livery_new (KGX_LIVERY_UUID_LINUX,
                         C_("livery-name", "Linux"),
                         night,
                         NULL);
}


static inline KgxLivery *
xterm_livery (void)
{
  g_autoptr (KgxPalette) night = NULL;
  g_autoptr (KgxPalette) day = NULL;
  GdkRGBA night_colours[16] = {
    GDK_RGBA ("000000"), /* Black          */
    GDK_RGBA ("cd0000"), /* Red            */
    GDK_RGBA ("00cd00"), /* Green          */
    GDK_RGBA ("cdcd00"), /* Yellow         */
    GDK_RGBA ("0000ee"), /* Blue           */
    GDK_RGBA ("cd00cd"), /* Magenta        */
    GDK_RGBA ("00cdcd"), /* Cyan           */
    GDK_RGBA ("e5e5e5"), /* White          */
    GDK_RGBA ("7f7f7f"), /* Bright Black   */
    GDK_RGBA ("ff0000"), /* Bright Red     */
    GDK_RGBA ("00ff00"), /* Bright Green   */
    GDK_RGBA ("ffff00"), /* Bright Yellow  */
    GDK_RGBA ("5c5cff"), /* Bright Blue    */
    GDK_RGBA ("ff00ff"), /* Bright Magenta */
    GDK_RGBA ("00ffff"), /* Bright Cyan    */
    GDK_RGBA ("ffffff"), /* Bright White   */
  };

  night = kgx_palette_new (&((GdkRGBA) { 1.0, 1.0, 1.0, 1.0 }),
                           &((GdkRGBA) { 0.0, 0.0, 0.0, 1.0 }),
                           0.0,
                           G_N_ELEMENTS (night_colours),
                           night_colours);

  return kgx_livery_new (KGX_LIVERY_UUID_XTERM,
                         C_("livery-name", "XTerm"),
                         night,
                         day);
}


static inline gboolean
table_add_livery (GHashTable *table, KgxLivery *livery)
{
  return g_hash_table_insert (table,
                              g_strdup (kgx_livery_get_uuid (livery)),
                              kgx_livery_ref (livery));
}


static void
kgx_livery_manager_init (KgxLiveryManager *self)
{
  g_autoptr (KgxLivery) linux = linux_livery ();
  g_autoptr (KgxLivery) xterm = xterm_livery ();

  self->fallback = standard_livery ();

  self->custom_liveries = g_hash_table_new_full (g_str_hash,
                                                 g_str_equal,
                                                 g_free,
                                                 (GDestroyNotify) kgx_livery_unref);
  self->builtin_liveries = g_hash_table_new_full (g_str_hash,
                                                  g_str_equal,
                                                  g_free,
                                                  (GDestroyNotify) kgx_livery_unref);

  table_add_livery (self->builtin_liveries, self->fallback);
  table_add_livery (self->builtin_liveries, linux);
  table_add_livery (self->builtin_liveries, xterm);
}


GVariant *
kgx_livery_manager_get_custom_liveries (KgxLiveryManager *self)
{
  g_auto (GVariantBuilder) builder = G_VARIANT_BUILDER_INIT (G_VARIANT_TYPE_VARDICT);
  GHashTableIter iter;
  const char *key;
  KgxLivery *value;

  g_return_val_if_fail (KGX_IS_LIVERY_MANAGER (self), NULL);

  g_hash_table_iter_init (&iter, self->custom_liveries);
  while (g_hash_table_iter_next (&iter,
                                 (gpointer *) &key,
                                 (gpointer *) &value)) {
    g_variant_builder_open (&builder, G_VARIANT_TYPE ("{sv}"));
    g_variant_builder_add (&builder, "s", key);
    kgx_livery_serialise_to (value, &builder);
    g_variant_builder_close (&builder);
  }

  return g_variant_builder_end (&builder);
}


void
kgx_livery_manager_set_custom_liveries (KgxLiveryManager *self,
                                        GVariant         *variant)
{
  g_autoptr (GVariantIter) iter = NULL;
  g_autoptr (GVariant) value = NULL;
  const char *uuid = NULL;

  g_return_if_fail (KGX_IS_LIVERY_MANAGER (self));
  g_return_if_fail (variant != NULL);

  iter = g_variant_iter_new (variant);

  g_hash_table_remove_all (self->custom_liveries);

  while (g_variant_iter_next (iter, "(&sv)", &uuid, &value)) {
    g_autoptr (KgxLivery) livery = kgx_livery_deserialise (value);

    if (g_strcmp0 (kgx_livery_get_uuid (livery), uuid) != 0) {
      g_warning ("livery-manager: conflicted uuid: expected ‘%s’, found ‘%s’",
                uuid,
                kgx_livery_get_uuid (livery));
      continue;
    }

    if (!table_add_livery (self->custom_liveries, livery)) {
      g_warning ("livery-manager: duplicated uuid: ‘%s’",
                 kgx_livery_get_uuid (livery));
    }

    g_clear_pointer (&value, g_variant_unref);
  }

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_CUSTOM_LIVERIES]);
}


KgxLivery *
kgx_livery_manager_resolve (KgxLiveryManager *self,
                            const char       *uuid)
{
  KgxLivery *livery = NULL;

  g_return_val_if_fail (KGX_IS_LIVERY_MANAGER (self), NULL);

  livery = g_hash_table_lookup (self->custom_liveries, uuid);
  if (livery) {
    return kgx_livery_ref (livery);
  }

  livery = g_hash_table_lookup (self->builtin_liveries, uuid);
  if (livery) {
    return kgx_livery_ref (livery);
  }

  return NULL;
}


KgxLivery *
kgx_livery_manager_dup_fallback (KgxLiveryManager *self)
{
  g_return_val_if_fail (KGX_IS_LIVERY_MANAGER (self), NULL);

  return kgx_livery_ref (self->fallback);
}
