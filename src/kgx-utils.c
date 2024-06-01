/* kgx-utils.c
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
#include <math.h>

#include "kgx-utils.h"


#define SINGLE_ARG_LONG "--command"


G_DEFINE_QUARK (kgx-argument-error-quark, kgx_argument_error)


static inline GStrv
process_single_arg (const char *const raw_arg)
{
  g_auto (GStrv) argv = NULL;
  g_autofree char *resolved_bin = NULL;
  const char *bin = raw_arg;
  gboolean can_exec_directly = FALSE;

  if (strchr (bin, '/') != NULL) {
    can_exec_directly = g_file_test (bin, G_FILE_TEST_IS_EXECUTABLE);
  } else {
    resolved_bin = g_find_program_in_path (bin);

    if (resolved_bin != NULL) {
      bin = resolved_bin;
      can_exec_directly = TRUE;
    }
  }

  if (can_exec_directly) {
    argv = g_new0 (char *, 2);
    g_set_str (&argv[0], bin);
  } else {
    argv = g_new0 (char *, 4);
    g_set_str (&argv[0], "/bin/sh");
    g_set_str (&argv[1], "-c");
    g_set_str (&argv[2], bin);
  }

  return g_steal_pointer (&argv);
}


void
kgx_filter_arguments (GStrv   *arguments,
                      GStrv   *command,
                      GError **error)
{
  g_autoptr (GStrvBuilder) filtered = g_strv_builder_new ();
  g_autoptr (GStrvBuilder) extracted = g_strv_builder_new ();
  g_auto (GStrv) original_argv = g_steal_pointer (arguments);
  g_auto (GStrv) extracted_argv = NULL;
  g_auto (GStrv) single_arg_argv = NULL;
  gboolean had_single_arg = FALSE;
  gboolean possibly_xterm_style = FALSE;
  size_t extracted_len = 0;

  if (original_argv == NULL) {
    goto out;
  }

  g_strv_builder_add (filtered, original_argv[0]);
  if (original_argv[0] == NULL) {
    goto out;
  }

  for (size_t i = 1; original_argv[i] != NULL; i++) {
    if (strcmp (original_argv[i], "-e") == 0) {
      possibly_xterm_style = TRUE;

      /* Everything after ‘-e’ should be eaten as a command */
      g_strv_builder_addv (extracted, (const char **) &original_argv[i + 1]);
      break;
    } else if (strcmp (original_argv[i], "--") == 0) {
      /* Everything after ‘--’ should be eaten as a command */
      g_strv_builder_addv (extracted, (const char **) &original_argv[i + 1]);
      break;
    } else if (g_str_has_prefix (original_argv[i], SINGLE_ARG_LONG)) {
      const char *ptr = original_argv[i] + strlen (SINGLE_ARG_LONG);

      had_single_arg = TRUE;

      if (G_LIKELY (ptr[0] == '=')) {
        g_clear_pointer (&single_arg_argv, g_strfreev);
        single_arg_argv = process_single_arg (ptr + 1);
        continue;
      } else if (G_UNLIKELY (ptr[0] == '\0')) {
        g_set_error_literal (error,
                             KGX_ARGUMENT_ERROR,
                             KGX_ARGUMENT_ERROR_MISSING,
                             /* Translators: ‘command’ is the argument name, and shouldn't be translated */
                             _("Missing argument for --command"));
      }
    }

    /* Pass through */
    g_strv_builder_add (filtered, original_argv[i]);
  }

  extracted_argv = g_strv_builder_end (extracted);
  extracted_len = g_strv_length (extracted_argv);

  if (G_UNLIKELY (extracted_len > 0 && had_single_arg)) {
    g_set_error_literal (error,
                         KGX_ARGUMENT_ERROR,
                         KGX_ARGUMENT_ERROR_BOTH,
                         _("Cannot use both --command and positional parameters"));
    return;
  }

out:
  *arguments = g_strv_builder_end (filtered);

  if (possibly_xterm_style && extracted_len == 1) {
    single_arg_argv = process_single_arg (extracted_argv[0]);
  }

  if (single_arg_argv && single_arg_argv[0]) {
    *command = g_steal_pointer (&single_arg_argv);
  } else if (extracted_len > 0) {
    *command = g_steal_pointer (&extracted_argv);
  } else {
    *command = NULL;
  }
}


static inline gboolean
is_space_or_percent (const char *const text)
{
  gunichar c = g_utf8_get_char (text);

  return g_unichar_isspace (c) || c == '%';
}


static inline gboolean
is_decimal_separator (const char *const text)
{
  gunichar c = g_utf8_get_char (text);

  return c == '.' || c == ',';
}


static inline void
skip_space_or_percent (const char **const text)
{
  const char *ptr = *text;

  while (*ptr && is_space_or_percent (ptr)) {
    ptr = g_utf8_next_char (ptr);
  }

  *text = ptr;
}


static inline int
read_part (const char **const text,
           size_t      *const n_digits)
{
  const char *ptr = *text;
  size_t a = 0;
  int parsed = 0;

  while (*ptr) {
    gunichar c = g_utf8_get_char (ptr);
    int c_val = g_unichar_digit_value (c);

    if (c_val == -1) {
      break;
    }

    parsed = (10 * parsed) + c_val;

    ptr = g_utf8_next_char (ptr);
    a++;
  }

  *text = ptr;
  if (n_digits) {
    *n_digits = a;
  }

  return parsed;
}


gboolean
kgx_parse_percentage (const char *const text,
                      double     *const value)
{
  const char *ptr = text;
  int whole = 0, frac = 0;
  size_t n_digits_whole = 0;
  size_t n_digits_frac = 0;

  skip_space_or_percent (&ptr);

  whole = read_part (&ptr, &n_digits_whole);
  if (*ptr && is_decimal_separator (ptr)) {
    ptr = g_utf8_next_char (ptr);
    frac = read_part (&ptr, &n_digits_frac);
  }

  if (n_digits_whole == 0 && n_digits_frac == 0) {
    /* nothing read */
    return FALSE;
  }

  skip_space_or_percent (&ptr);

  if (*ptr) {
    /* garbage at the end */
    return FALSE;
  }

  *value = whole + (frac / pow (10, n_digits_frac));

  return TRUE;
}
