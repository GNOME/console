/* kgx-livery-manager.h
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

#include "kgx-livery.h"

G_BEGIN_DECLS

#define KGX_TYPE_LIVERY_MANAGER (kgx_livery_manager_get_type ())

G_DECLARE_FINAL_TYPE (KgxLiveryManager, kgx_livery_manager, KGX, LIVERY_MANAGER, GObject)

GVariant     *kgx_livery_manager_get_custom_liveries         (KgxLiveryManager *self);
void          kgx_livery_manager_set_custom_liveries         (KgxLiveryManager *self,
                                                              GVariant         *variant);
KgxLivery    *kgx_livery_manager_resolve                     (KgxLiveryManager *self,
                                                              const char       *uuid);
KgxLivery    *kgx_livery_manager_dup_fallback                (KgxLiveryManager *self);

G_END_DECLS
