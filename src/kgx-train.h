/* kgx-train.h
 *
 * Copyright 2021-2024 Zander Brown
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

#include "kgx-process.h"

G_BEGIN_DECLS


/**
 * KgxStatus:
 * @KGX_NONE: It's a regular #KgxTab
 * @KGX_REMOTE: The #KgxTab is connected to a "remote" session
 * @KGX_PRIVILEGED: The #KgxTab is running as someone other than the current
 *                  user
 *
 * Indicates the status of the session the #KgxTab represents
 */
typedef enum /*< flags,prefix=KGX >*/ {
  KGX_NONE = 0,              /*< nick=none >*/
  KGX_REMOTE = (1 << 0),     /*< nick=remote >*/
  KGX_PRIVILEGED = (1 << 1), /*< nick=privileged >*/
} KgxStatus;


struct _KgxTrainClass {
  GObjectClass parent;
};

#define KGX_TYPE_TRAIN (kgx_train_get_type ())

G_DECLARE_DERIVABLE_TYPE (KgxTrain, kgx_train, KGX, TRAIN, GObject)

const char        *kgx_train_get_uuid        (KgxTrain     *self);
const char        *kgx_train_get_tag         (KgxTrain     *self);
GPid               kgx_train_get_pid         (KgxTrain     *self);
GPtrArray         *kgx_train_get_children    (KgxTrain     *self);
void               kgx_train_push_child      (KgxTrain     *self,
                                              KgxProcess   *process);
void               kgx_train_pop_child       (KgxTrain     *self,
                                              KgxProcess   *process);

G_END_DECLS
