/* kgx-locale.h
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

#include "kgx-config.h"

#include <glib/gi18n.h>

#include <locale.h>

G_BEGIN_DECLS


#define KGX_LOCALE_DYNAMIC ""
#define KGX_LOCALE_TEST "C.UTF-8"


G_GNUC_UNUSED
static inline void
kgx_locale_init (const char *locale)
{
  setlocale (LC_ALL, locale);
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
}


G_END_DECLS
