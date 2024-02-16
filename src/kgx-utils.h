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

#include <glib.h>

G_BEGIN_DECLS


/**
 * kgx_str_constrained_append:
 * @buffer: a #GString to append to
 * @source: the text to read from
 * @max_len: the maximum length @buffer should expand to
 *
 * Writes @source to @buffer, but stopping if we reach @max_len, at which point
 * an elipsis is added and we bail out. UTF-8 aware.
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

  if (G_LIKELY (source_len < max_len - 1)) {
    return g_strdup (source);
  }

  buffer = g_string_sized_new (max_len);

  kgx_str_constrained_append (buffer, source, max_len);

  return g_string_free (g_steal_pointer (&buffer), FALSE);
}

G_END_DECLS
