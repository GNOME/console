/* kgx-templated.h
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

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS


#define KGX_TYPE_TEMPLATED (kgx_templated_get_type ())

G_DECLARE_INTERFACE (KgxTemplated, kgx_templated, KGX, TEMPLATED, GObject)


struct _KgxTemplatedInterface {
  GTypeInterface g_iface;
};


void     kgx_templated_class_set_template_from_resource  (GObjectClass    *object_class,
                                                          const char      *resource_path);
void     kgx_templated_class_bind_template_child_full    (GObjectClass    *object_class,
                                                          const char      *name,
                                                          size_t           offset);
void     kgx_templated_class_bind_template_callback_full (GObjectClass    *object_class,
                                                          const char      *callback_name,
                                                          GCallback        callback_symbol);

void     kgx_templated_init_template                     (KgxTemplated    *self);
void     kgx_templated_dispose_template                  (KgxTemplated    *self,
                                                          GType            current_type);
void     kgx_templated_apply                             (GObject         *target,
                                                          GtkBuilderScope *scope,
                                                          GBytes          *template);

#define kgx_templated_class_bind_template_child(object_class, TypeName, member_name) \
  kgx_templated_class_bind_template_child_full (object_class, \
                                                #member_name, \
                                                G_STRUCT_OFFSET (TypeName, member_name))

#define kgx_templated_class_bind_template_child_private(object_class, TypeName, member_name) \
  kgx_templated_class_bind_template_child_full (object_class, \
                                                #member_name, \
                                                G_PRIVATE_OFFSET (TypeName, member_name))

#define kgx_templated_class_bind_template_callback(object_class, callback) \
  kgx_templated_class_bind_template_callback_full (object_class, \
                                                   #callback, \
                                                   G_CALLBACK (callback))

G_END_DECLS
