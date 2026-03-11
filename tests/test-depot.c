/* test-depot.c
 *
 * Copyright 2026 Zander Brown
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

#include "kgx-locale.h"
#include "kgx-proxy-info.h"
#include "kgx-train.h"
#include "kgx-watcher.h"

#include "kgx-depot.h"


static const char *
dummy_process_path (void)
{
  static const char *path = NULL;

  if (g_once_init_enter_pointer (&path)) {
    const char *env = g_getenv ("KGX_DUMMY_PROCESS");

    g_assert_nonnull (env);

    g_debug ("Using ‘%s’ as dummy process", env);

    g_once_init_leave_pointer (&path, env);
  }

  return path;
}


#define KGX_TYPE_BANANA (kgx_banana_get_type ())
G_DECLARE_FINAL_TYPE (KgxBanana, kgx_banana, KGX, BANANA, KgxTrain)


struct _KgxBanana {
  KgxTrain parent;
};


G_DEFINE_TYPE (KgxBanana, kgx_banana, KGX_TYPE_TRAIN)


static void
kgx_banana_class_init (KgxBananaClass *self)
{

}


static void
kgx_banana_init (KgxBanana *self)
{

}


static void
test_depot_new_destroy (void)
{
  KgxDepot *depot = g_object_new (KGX_TYPE_DEPOT,
                                  "train-type", KGX_TYPE_BANANA,
                                  NULL);

  g_assert_finalize_object (depot);
}


static void
test_depot_get (void)
{
  g_autoptr (KgxWatcher) watcher = g_object_new (KGX_TYPE_WATCHER, NULL);
  g_autoptr (KgxDepot) depot =
    g_object_new (KGX_TYPE_DEPOT,
                  "watcher", watcher,
                  "train-type", KGX_TYPE_BANANA,
                  NULL);
  g_autoptr (KgxWatcher) got_watcher = NULL;
  g_autoptr (KgxProxyInfo) got_proxy_info = NULL;
  GType train_type = G_TYPE_INVALID;

  g_object_get (depot,
                "train-type", &train_type,
                "watcher", &got_watcher,
                "proxy-info", &got_proxy_info,
                NULL);

  g_assert_true (g_type_is_a (train_type, KGX_TYPE_TRAIN));
  g_assert_true (g_type_is_a (train_type, KGX_TYPE_BANANA));
  g_assert_nonnull (got_watcher);
  g_assert_true (watcher == got_watcher);
  g_assert_nonnull (got_proxy_info);
}


static void
spawned (GObject      *source,
         GAsyncResult *res,
         gpointer      user_data)

{
  gboolean *done = user_data;
  g_autoptr (KgxTrain) train = NULL;
  g_autoptr (GError) error = NULL;

  train = kgx_depot_spawn_finish (KGX_DEPOT (source), res, &error);
  g_assert_no_error (error);

  *done = TRUE;
}


static void
test_depot_spawn (void)
{
  g_autoptr (GError) error = NULL;
  g_autoptr (GMainContext) context = g_main_context_new ();
  g_autoptr (KgxWatcher) watcher = g_object_new (KGX_TYPE_WATCHER, NULL);
  g_autoptr (KgxSettings) settings = g_object_new (KGX_TYPE_SETTINGS, NULL);
  g_autoptr (KgxDepot) depot =
    g_object_new (KGX_TYPE_DEPOT,
                  "watcher", watcher,
                  "train-type", KGX_TYPE_TRAIN,
                  NULL);
  g_autoptr (VtePty) pty = vte_pty_new_sync (VTE_PTY_DEFAULT, NULL, &error);
  gboolean done = FALSE;

  g_assert_no_error (error);

  g_main_context_push_thread_default (context);

  kgx_depot_spawn (depot,
                   settings,
                   pty,
                   NULL,
                   (const char *[]) { dummy_process_path (), NULL },
                   NULL,
                   NULL,
                   spawned,
                   &done);

  while (!done) {
    g_main_context_iteration (context, TRUE);
  }

  g_main_context_pop_thread_default (context);
}


int
main (int argc, char *argv[])
{
  kgx_locale_init (KGX_LOCALE_TEST);

  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func ("/kgx/depot/new-destroy", test_depot_new_destroy);
  g_test_add_func ("/kgx/depot/get-set", test_depot_get);
  g_test_add_func ("/kgx/depot/spawn", test_depot_spawn);

  return g_test_run ();
}
