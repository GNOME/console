/* kgx-templated.c
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

#include <gtk/gtk.h>

#include "kgx-utils.h"

#include "kgx-templated.h"


GQuark kgx_templated_data_quark (void);
GQuark kgx_templated_objects_quark (void);


G_DEFINE_QUARK (kgx-templated-data, kgx_templated_data)


G_DEFINE_QUARK (kgx-templated-objects, kgx_templated_objects)


struct _ChildData {
  char   *name;
  size_t  offset;
};


KGX_DEFINE_DATA (ChildData, child_data)


static inline void
child_data_cleanup (ChildData *self)
{
  g_clear_pointer (&self->name, g_free);
}


struct _TemplateData {
  GtkBuilderScope *scope;
  GBytes          *template;
  GPtrArray       *children;
};


KGX_DEFINE_DATA (TemplateData, template_data)


static inline void
template_data_cleanup (TemplateData *self)
{
  g_clear_object (&self->scope);
  g_clear_pointer (&self->template, g_bytes_unref);
  g_clear_pointer (&self->children, g_ptr_array_unref);
}


G_DEFINE_INTERFACE (KgxTemplated, kgx_templated, G_TYPE_OBJECT)


static inline TemplateData *
get_template_data (GObjectClass *object_class)
{
  return g_type_get_qdata (G_OBJECT_CLASS_TYPE (object_class),
                           kgx_templated_data_quark ());
}


void
kgx_templated_class_set_template_from_resource (GObjectClass *object_class,
                                                const char   *resource_path)
{
  g_autoptr (TemplateData) data = template_data_alloc ();
  g_autoptr (GError) error = NULL;
  g_autoptr (GBytes) template =
    g_resources_lookup_data (resource_path, G_RESOURCE_LOOKUP_FLAGS_NONE, &error);

  if (error) {
    g_critical ("templated: %s", error->message);
  }

  data->template = g_steal_pointer (&template);
  data->scope = gtk_builder_cscope_new ();
  data->children = g_ptr_array_new_with_free_func (child_data_free);

  g_type_set_qdata (G_OBJECT_CLASS_TYPE (object_class),
                    kgx_templated_data_quark (),
                    g_steal_pointer (&data));
}


void
kgx_templated_class_bind_template_child_full (GObjectClass *object_class,
                                              const char   *name,
                                              size_t        offset)
{
  TemplateData *data = get_template_data (object_class);
  g_autoptr (ChildData) child = child_data_alloc ();

  g_set_str (&child->name, name);
  child->offset = offset;

  g_ptr_array_add (data->children, g_steal_pointer (&child));
}


void
kgx_templated_class_bind_template_callback_full (GObjectClass *object_class,
                                                 const char   *callback_name,
                                                 GCallback     callback_symbol)
{
  TemplateData *data = get_template_data (object_class);

  gtk_builder_cscope_add_callback_symbol (GTK_BUILDER_CSCOPE (data->scope),
                                          callback_name,
                                          callback_symbol);
}


static void
kgx_templated_default_init (KgxTemplatedInterface *iface)
{
}


static inline GHashTable *
get_object_table (GObject *object, GType current_type)
{
  GHashTable *table_table =
    g_object_get_qdata (object, kgx_templated_objects_quark ());
  GHashTable *table = NULL;

  if (!table_table) {
    table_table =
      g_hash_table_new_full (g_direct_hash,
                             g_direct_equal,
                             NULL,
                             (GDestroyNotify) g_hash_table_unref);

    g_object_set_qdata_full (object,
                             kgx_templated_objects_quark (),
                             table_table,
                             (GDestroyNotify) g_hash_table_unref);
  }

  table = g_hash_table_new_full (g_str_hash,
                                 g_str_equal,
                                 g_free,
                                 g_object_unref);

  g_hash_table_insert (table_table,
                       GTYPE_TO_POINTER (current_type),
                       table);

  return table;
}


static void
apply_internal (GObject         *target,
                GtkBuilderScope *scope,
                GBytes          *template,
                GPtrArray       *children)
{
  g_autoptr (GtkBuilder) builder = gtk_builder_new ();
  g_autoptr (GError) error = NULL;
  GHashTable *objects;
  const char *buffer;
  size_t length;

  g_return_if_fail (G_IS_OBJECT (target));
  g_return_if_fail (GTK_IS_BUILDER_SCOPE (scope));
  g_return_if_fail (template != NULL);

  objects = get_object_table (target, G_OBJECT_TYPE (target));

  gtk_builder_set_current_object (builder, target);
  gtk_builder_set_scope (builder, scope);

  buffer = g_bytes_get_data (template, &length);

  if (!gtk_builder_extend_with_template (builder,
                                         target,
                                         G_OBJECT_TYPE (target),
                                         buffer,
                                         length,
                                         &error)) {
    g_critical ("templated: setup failed: %s", error->message);

    return;
  }

  for (size_t i = 0; children && i < children->len; i++) {
    ChildData *child = g_ptr_array_index (children, i);
    gpointer field = G_STRUCT_MEMBER_P (target, child->offset);
    GObject *object = gtk_builder_get_object (builder, child->name);

    if (!object) {
      g_critical ("templated: object-id ‘%s’ missing", child->name);

      continue;
    }

    if (!g_hash_table_insert (objects,
                              g_strdup (child->name),
                              g_object_ref (object))) {
      g_warning ("templated: object-id ‘%s’ was in parent", child->name);
    }

    (* (gpointer *) field) = object;
  }
}


void
kgx_templated_init_template (KgxTemplated *self)
{
  TemplateData *data = NULL;

  g_return_if_fail (KGX_IS_TEMPLATED (self));

  data = get_template_data (G_OBJECT_GET_CLASS (self));

  if (!data) {
    g_warning ("kgx_templated_init_template called without template set for %s",
               G_OBJECT_TYPE_NAME (self));
    return;
  }

  apply_internal (G_OBJECT (self), data->scope, data->template, data->children);
}


static inline GHashTable *
steal_object_table (GObject *object, GType current_type)
{
  GHashTable *table_table =
    g_object_get_qdata (object, kgx_templated_objects_quark ());
  GHashTable *table = NULL;

  if (!table_table) {
    return NULL;
  }

  table = g_hash_table_lookup (table_table, GTYPE_TO_POINTER (current_type));
  if (table) {
    g_hash_table_steal (table_table, GTYPE_TO_POINTER (current_type));
  }

  if (g_hash_table_size (table_table) < 1) {
    g_object_set_qdata (object,
                        kgx_templated_objects_quark (),
                        NULL);
  }

  return table;
}


void
kgx_templated_dispose_template (KgxTemplated *self, GType current_type)
{
  g_autoptr (GHashTable) objects_table = NULL;
  TemplateData *data = NULL;

  g_return_if_fail (KGX_IS_TEMPLATED (self));

  objects_table = steal_object_table (G_OBJECT (self), current_type);

  data = get_template_data (G_OBJECT_GET_CLASS (self));

  for (size_t i = 0; i < data->children->len; i++) {
    ChildData *child = g_ptr_array_index (data->children, i);
    gpointer field = G_STRUCT_MEMBER_P (self, child->offset);

    (* (gpointer *) field) = NULL;
  }
}


void
kgx_templated_apply (GObject         *target,
                     GtkBuilderScope *scope,
                     GBytes          *template)
{
  apply_internal (target, scope, template, NULL);
}
