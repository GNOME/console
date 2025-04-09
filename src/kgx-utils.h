/* kgx-utils.h
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

#pragma once

#include <glib-object.h>
#include <stdint.h>

G_BEGIN_DECLS


/**
 * KGX_DEFINE_DATA:
 * @TypeName: the struct name
 * @type_name: the method prefix
 *
 * Defines a autocleanup'd struct, methods to allocate and free it, and some
 * utilities for using it with a GTask.
 *
 * Requires a struct defined as `struct _TypeName` and a `type_name_cleanup`
 * function that clears the content of the struct (but not free the struct
 * itself).
 */
#define KGX_DEFINE_DATA(TypeName, type_name)                                 \
  typedef struct _##TypeName TypeName;                                       \
  G_GNUC_UNUSED                                                              \
  static inline void type_name##_cleanup (TypeName *self);                   \
  G_GNUC_UNUSED                                                              \
  static inline void                                                         \
  type_name##_free (gpointer data)                                           \
  {                                                                          \
    type_name##_cleanup (data);                                              \
    g_free (data);                                                           \
  }                                                                          \
  G_GNUC_UNUSED                                                              \
  static inline TypeName *type_name##_alloc (void) G_GNUC_MALLOC;            \
  static inline TypeName *                                                   \
  type_name##_alloc (void)                                                   \
  {                                                                          \
    return g_new0 (TypeName, 1);                                             \
  }                                                                          \
  G_GNUC_UNUSED                                                              \
  static inline void                                                         \
  kgx_task_set_##type_name (GTask *restrict task, TypeName *restrict data)   \
  {                                                                          \
    g_task_set_task_data (task, data, type_name##_free);                     \
  }                                                                          \
  G_GNUC_UNUSED                                                              \
  static inline TypeName *                                                   \
  kgx_task_get_##type_name (GTask *task)                                     \
  {                                                                          \
    return g_task_get_task_data (task);                                      \
  }                                                                          \
  G_GNUC_UNUSED                                                              \
  static inline void                                                         \
  clear_##type_name (TypeName **ptr)                                         \
  {                                                                          \
    g_clear_pointer (ptr, type_name##_free);                                 \
  }                                                                          \
  G_DEFINE_AUTOPTR_CLEANUP_FUNC (TypeName, type_name##_free)


/**
 * KGX_INVALID_PROP:
 * @object: the object the property was set on
 * @property_id: the attempted property id
 * @pspec: the @pspec of @property_id
 *
 * This is simple wrapper around G_OBJECT_WARN_INVALID_PROPERTY_ID that lets
 * us easily ignore this in coverage reports.
 *
 * Whilst we could ‘test’ these lines, it's rather fiddly for really no
 * reward, but if we can't easily exclude them they stand up in coverage like
 * sore thumbs.
 */
#define KGX_INVALID_PROP(object, property_id, pspec)                         \
    default:                                                                 \
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);        \
      break;


/**
 * kgx_set_boolean_prop:
 * @object: the #GObject the property is on
 * @pspec: the #GParamSpec being set
 * @target: the storage on @object for @pspec
 * @value: the potential new value for @target
 *
 * Update a boolean property, notifying if the value changed
 *
 * Returns: %TRUE if the value changed, otherwise %FALSE
 */
static inline gboolean
kgx_set_boolean_prop (GObject      *restrict object,
                      GParamSpec   *restrict pspec,
                      gboolean     *restrict target,
                      const GValue *restrict value)
{
  gboolean new_value = g_value_get_boolean (value);

  if (new_value == *target) {
    return FALSE;
  }

  *target = new_value;
  g_object_notify_by_pspec (object, pspec);

  return TRUE;
}


/**
 * kgx_set_int64_prop:
 * @object: the #GObject the property is on
 * @pspec: the #GParamSpec being set
 * @target: the storage on @object for @pspec
 * @value: the potential new value for @target
 *
 * Update an int64 property, notifying if the value changed
 *
 * Returns: %TRUE if the value changed, otherwise %FALSE
 */
static inline gboolean
kgx_set_int64_prop (GObject      *restrict object,
                    GParamSpec   *restrict pspec,
                    int64_t      *restrict target,
                    const GValue *restrict value)
{
  int64_t new_value = g_value_get_int64 (value);

  if (new_value == *target) {
    return FALSE;
  }

  *target = new_value;
  g_object_notify_by_pspec (object, pspec);

  return TRUE;
}


/**
 * kgx_set_str_prop:
 * @object: the #GObject the property is on
 * @pspec: the #GParamSpec being set
 * @target: the storage on @object for @pspec
 * @value: the potential new value for @target
 *
 * Update a string property, notifying if the value changed
 *
 * Returns: %TRUE if the value changed, otherwise %FALSE
 */
static inline gboolean
kgx_set_str_prop (GObject       *restrict object,
                  GParamSpec    *restrict pspec,
                  char         **restrict target,
                  const GValue  *restrict value)
{
  const char *new_value = g_value_get_string (value);

  if (!g_set_str (target, new_value)) {
    return FALSE;
  }

  g_object_notify_by_pspec (object, pspec);

  return TRUE;
}


/**
 * kgx_set_flags_prop:
 * @object: the #GObject the property is on
 * @pspec: the #GParamSpec being set
 * @target: the storage on @object for @pspec
 * @value: the potential new value for @target
 *
 * Update an flag property, notifying if the value changed
 *
 * Returns: %TRUE if the value changed, otherwise %FALSE
 */
static inline gboolean
kgx_set_flags_prop (GObject      *restrict object,
                    GParamSpec   *restrict pspec,
                    guint        *restrict target,
                    const GValue *restrict value)
{
  guint new_value = g_value_get_flags (value);

  if (new_value == *target) {
    return FALSE;
  }

  *target = new_value;
  g_object_notify_by_pspec (object, pspec);

  return TRUE;
}


/**
 * kgx_str_constrained_append:
 * @buffer: a #GString to append to
 * @source: the text to read from
 * @max_len: the maximum length @buffer should expand to
 *
 * Writes @source to @buffer, but stopping if @buffer reachs @max_len bytes,
 * at which point an elipsis is added and we bail out. UTF-8 aware.
 *
 * Returns: %TRUE if @max_len was reached, otherwise %FALSE
 */
static inline gboolean
kgx_str_constrained_append (GString    *buffer,
                            const char *source,
                            size_t      max_len)
{
  size_t source_len = strlen (source);

  if (G_UNLIKELY (buffer->len + source_len > max_len)) {
    for (const char *iter = source;
          *iter && buffer->len < max_len;
          iter = g_utf8_next_char (iter)) {
      g_string_append_unichar (buffer, g_utf8_get_char (iter));
    }
    g_string_append (buffer, "…");

    return TRUE;
  } else {
    g_string_append_len (buffer, source, source_len);

    return FALSE;
  }
}


/**
 * kgx_str_constrained_dup:
 * @source: a string to duplicate
 * @max_len: the maximum number of bytes to duplicate
 *
 * See kgx_str_constrained_append()
 *
 * Returns: (transfer full): a newly allocated, possibly truncated, string
 */
static inline char *
kgx_str_constrained_dup (const char *source, size_t max_len)
{
  g_autoptr (GString) buffer = NULL;
  size_t source_len = strlen (source);

  if (G_LIKELY (source_len <= max_len)) {
    return g_strdup (source);
  }

  buffer = g_string_sized_new (max_len);

  kgx_str_constrained_append (buffer, source, max_len);

  return g_string_free (g_steal_pointer (&buffer), FALSE);
}


static inline gboolean
kgx_str_non_empty (const char *str)
{
  const char *first_non_space = str;

  if (first_non_space == NULL) {
    return FALSE;
  }

  while (*first_non_space &&
         g_unichar_isspace (g_utf8_get_char (first_non_space))) {
    first_non_space = g_utf8_next_char (first_non_space);
  }

  return *first_non_space;
}


/**
 * kgx_uri_is_non_local_file:
 * @uri: the uri to inspect
 *
 * Returns: %TRUE when this is both a file-uri *and* one that points to a
 *          remote machine. All other uris are %FALSE.
 */
static inline gboolean
kgx_uri_is_non_local_file (GUri *uri)
{
  return (g_strcmp0 (g_uri_get_scheme (uri), "file") == 0) &&
         (kgx_str_non_empty (g_uri_get_host (uri)) &&
          g_strcmp0 (g_uri_get_host (uri), "localhost") != 0 &&
          g_strcmp0 (g_uri_get_host (uri), g_get_host_name ()) != 0);
}


typedef enum /*< enum,prefix=KGX >*/ {
  KGX_ARGUMENT_ERROR_BOTH,
  KGX_ARGUMENT_ERROR_MISSING,
} KgxArgumentError;


#define KGX_ARGUMENT_ERROR (kgx_argument_error_quark ())


GQuark          kgx_argument_error_quark   (void);
void            kgx_filter_arguments       (GStrv             *arguments,
                                            GStrv             *command,
                                            GError           **error);
gboolean        kgx_parse_percentage       (const char *const  text,
                                            double     *const  value);


G_END_DECLS
