/* test-train.c
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

#include <gio/gio.h>

#include "kgx-locale.h"

#include "kgx-train.h"


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


typedef struct {
  GMainLoop *loop;
  pid_t pid;
  KgxTrain *train;
} Fixture;


static void
got_train (GObject      *source,
           GAsyncResult *result,
           gpointer      user_data)
{
  Fixture *fixture = user_data;
  g_autoptr (GError) error = NULL;

  fixture->train =
    KGX_TRAIN (g_async_initable_new_finish (G_ASYNC_INITABLE (source),
                                            result,
                                            &error));

  g_main_loop_quit (fixture->loop);
}


static void
fixture_setup (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (GError) error = NULL;

  fixture->loop = g_main_loop_new (NULL, FALSE);

  g_spawn_async (NULL,
                 (char *[]) { (char *) dummy_process_path (), NULL },
                 NULL,
                 G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_CLOEXEC_PIPES,
                 NULL,
                 NULL,
                 &fixture->pid,
                 &error);
  g_assert_no_error (error);
  g_assert_cmpint (fixture->pid, >, 0);

  g_async_initable_new_async (KGX_TYPE_TRAIN,
                              G_PRIORITY_HIGH,
                              NULL,
                              got_train,
                              fixture,
                              "pid", fixture->pid,
                              "tag", "dummy",
                              NULL);
  g_main_loop_run (fixture->loop);

  g_assert_nonnull (fixture->train);
}


static void
fixture_tear (Fixture *fixture, gconstpointer unused)
{
  if (fixture->pid) {
    g_spawn_close_pid (fixture->pid);
    fixture->pid = 0;
  }

  g_clear_object (&fixture->train);

  g_clear_pointer (&fixture->loop, g_main_loop_unref);
}


static void
test_train_new_destroy (void)
{
  KgxTrain *train = g_object_new (KGX_TYPE_TRAIN, NULL);

  g_assert_finalize_object (train);
}


static void
test_train_init (Fixture *fixture, gconstpointer unused)
{
  g_assert_cmpint (kgx_train_get_pid (fixture->train), ==, fixture->pid);
  g_assert_cmpstr (kgx_train_get_tag (fixture->train), ==, "dummy");
}


static void
test_train_props (Fixture *fixture, gconstpointer unused)
{
  g_autofree char *got_tag = NULL;
  g_autofree char *got_uuid = NULL;
  pid_t got_pid;
  KgxStatus got_status;

  g_object_get (fixture->train,
                "tag", &got_tag,
                "uuid", &got_uuid,
                "pid", &got_pid,
                "status", &got_status,
                NULL);

  g_assert_cmpint (kgx_train_get_pid (fixture->train), ==, got_pid);
  g_assert_cmpstr (kgx_train_get_tag (fixture->train), ==, got_tag);
  g_assert_cmpstr (kgx_train_get_uuid (fixture->train), ==, got_uuid);

  g_assert_cmpint (got_status, ==, KGX_NONE);
  g_assert_true (g_uuid_string_is_valid (got_uuid));
}


static void
child_added (KgxTrain *train, KgxProcess *process, gpointer user_data)
{
  gboolean *notified = user_data;

  *notified = TRUE;
}


static void
child_removed (KgxTrain *train, KgxProcess *process, gpointer user_data)
{
  gboolean *notified = user_data;

  *notified = TRUE;
}


static void
test_train_children (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (GPtrArray) children = kgx_train_get_children (fixture->train);
  g_autoptr (KgxProcess) process = kgx_process_new (42);
  gboolean notified = FALSE;

  /* Initially no children */
  g_assert_cmpint (children->len, ==, 0);

  g_signal_connect (fixture->train,
                    "child-added", G_CALLBACK (child_added),
                    &notified);

  /* Now add a child */
  g_assert_false (notified);
  kgx_train_push_child (fixture->train, process);
  g_assert_true (notified);

  g_clear_pointer (&children, g_ptr_array_unref);
  children = kgx_train_get_children (fixture->train);
  g_assert_cmpint (children->len, ==, 1);

  notified = FALSE;

  /* Adding an already known child, n children doesn't change */
  g_assert_false (notified);
  kgx_train_push_child (fixture->train, process);
  g_assert_false (notified);

  g_clear_pointer (&children, g_ptr_array_unref);
  children = kgx_train_get_children (fixture->train);
  g_assert_cmpint (children->len, ==, 1);

  g_signal_connect (fixture->train,
                    "child-removed", G_CALLBACK (child_removed),
                    &notified);
  notified = FALSE;

  /* Remove the child, now there are no children again */
  g_assert_false (notified);
  kgx_train_pop_child (fixture->train, process);
  g_assert_true (notified);

  g_clear_pointer (&children, g_ptr_array_unref);
  children = kgx_train_get_children (fixture->train);
  g_assert_cmpint (children->len, ==, 0);

  notified = FALSE;

  /* Remove it again, nothing should happen */
  g_assert_false (notified);
  kgx_train_pop_child (fixture->train, process);
  g_assert_false (notified);

  g_clear_pointer (&children, g_ptr_array_unref);
  children = kgx_train_get_children (fixture->train);
  g_assert_cmpint (children->len, ==, 0);
}


static void
pid_died (KgxTrain *train, int status, gpointer user_data)
{
  Fixture *fixture = user_data;
  g_autoptr (GError) error = NULL;

  g_assert_true (g_spawn_check_wait_status (status, &error));
  g_assert_no_error (error);

  g_main_loop_quit (fixture->loop);
}


static void
test_train_died (Fixture *fixture, gconstpointer unused)
{
  g_signal_connect (fixture->train,
                    "pid-died", G_CALLBACK (pid_died),
                    fixture);

  g_main_loop_run (fixture->loop);
}


int
main (int argc, char *argv[])
{
  kgx_locale_init (KGX_LOCALE_TEST);

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/kgx/train/new-destroy", test_train_new_destroy);
  g_test_add ("/kgx/train/init",
              Fixture, NULL,
              fixture_setup,
              test_train_init,
              fixture_tear);
  g_test_add ("/kgx/train/props",
              Fixture, NULL,
              fixture_setup,
              test_train_props,
              fixture_tear);
  g_test_add ("/kgx/train/children",
              Fixture, NULL,
              fixture_setup,
              test_train_children,
              fixture_tear);
  g_test_add ("/kgx/train/died",
              Fixture, NULL,
              fixture_setup,
              test_train_died,
              fixture_tear);

  return g_test_run ();
}
