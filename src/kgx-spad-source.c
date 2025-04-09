/* kgx-spad-source.c
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

#include <glib/gi18n.h>

#include "kgx-spad.h"
#include "kgx-utils.h"
#include "kgx-marshals.h"

#include "kgx-spad-source.h"


G_DEFINE_INTERFACE (KgxSpadSource, kgx_spad_source, G_TYPE_OBJECT)


enum {
  SPAD_THROWN,
  N_SIGNALS
};
static guint signals[N_SIGNALS];


static void
kgx_spad_source_default_init (KgxSpadSourceInterface *iface)
{
  signals[SPAD_THROWN] = g_signal_new ("spad-thrown",
                                       G_TYPE_FROM_INTERFACE (iface),
                                       G_SIGNAL_RUN_LAST,
                                       0,
                                       g_signal_accumulator_true_handled,
                                       NULL,
                                       kgx_marshals_BOOLEAN__STRING_VARIANT,
                                       G_TYPE_BOOLEAN,
                                       2,
                                       G_TYPE_STRING | G_SIGNAL_TYPE_STATIC_SCOPE,
                                       G_TYPE_VARIANT | G_SIGNAL_TYPE_STATIC_SCOPE);
  g_signal_set_va_marshaller (signals[SPAD_THROWN],
                              G_TYPE_FROM_INTERFACE (iface),
                              kgx_marshals_BOOLEAN__STRING_VARIANTv);
}


void
kgx_spad_source_throw (KgxSpadSource *source,
                       const char    *title,
                       KgxSpadFlags   flags,
                       const char    *error_body,
                       const char    *error_content,
                       GError        *error)
{
  g_autoptr (GVariant) bundle = kgx_spad_build_bundle (title,
                                                       flags,
                                                       error_body,
                                                       error_content,
                                                       error);
  gboolean handled = FALSE;

  g_return_if_fail (kgx_str_non_empty (error_body));

  g_debug ("raising spad: %s", title);

  g_signal_emit (source, signals[SPAD_THROWN], 0, title, bundle, &handled);

  if (G_UNLIKELY (!handled)) {
    g_log_structured (G_LOG_DOMAIN,
                      G_LOG_LEVEL_CRITICAL,
                      "CODE_FILE", __FILE__,
                      "CODE_LINE", G_STRINGIFY (__LINE__),
                      "CODE_FUNC", G_STRFUNC,
                      "KGX_SPAD_BODY", error_body,
                      "KGX_SPAD_CONTENT", kgx_str_non_empty (error_content) ?
                                            error_content : "None",
                      "KGX_SPAD_ERROR", error ? error->message : "None",
                      "MESSAGE", "unhandled-spad: ‘%s’",
                      title);
  }
}
