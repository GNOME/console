/* fp-vte-util.h
 *
 * Copyright 2019 Christian Hergert <chergert@redhat.com>
 *
 * Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
 * https://www.apache.org/licenses/LICENSE-2.0> or the MIT License
 * <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your
 * option. This file may not be copied, modified, or distributed
 * except according to those terms.
 *
 * SPDX-License-Identifier: (MIT OR Apache-2.0)
 */

#pragma once

#include <vte/vte.h>

G_BEGIN_DECLS

void         fp_vte_pty_spawn_async   (VtePty               *pty,
                                       const char           *working_directory,
                                       const char *const    *argv,
                                       const char *const    *env,
                                       int                   timeout,
                                       GCancellable         *cancellable,
                                       GAsyncReadyCallback   callback,
                                       gpointer              user_data);
gboolean     fp_vte_pty_spawn_finish  (VtePty               *pty,
                                       GAsyncResult         *result,
                                       GPid                 *child_pid,
                                       GError              **error);

G_END_DECLS
