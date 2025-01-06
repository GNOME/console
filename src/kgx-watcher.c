/* kgx-watcher.c
 *
 * Copyright 2022-2024 Zander Brown
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

#include "kgx-utils.h"

#include "kgx-watcher.h"


/**
 * KgxWatcher:
 * @watching: (element-type GLib.Pid ProcessWatch) the shells running in windows
 * @children: (element-type GLib.Pid ProcessWatch) the processes running in shells
 * @timeout: the current #GSource id of the watcher
 *
 * Used to monitor processes running in pages
 */
struct _KgxWatcher {
  GObject                   parent_instance;

  gboolean                  in_background;

  GTree                    *watching;
  GTree                    *children;

  guint                     timeout;
};


G_DEFINE_TYPE (KgxWatcher, kgx_watcher, G_TYPE_OBJECT)


enum {
  PROP_0,
  PROP_IN_BACKGROUND,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };


static void
kgx_watcher_dispose (GObject *object)
{
  KgxWatcher *self = KGX_WATCHER (object);

  g_clear_handle_id (&self->timeout, g_source_remove);

  g_clear_pointer (&self->watching, g_tree_unref);
  g_clear_pointer (&self->children, g_tree_unref);

  G_OBJECT_CLASS (kgx_watcher_parent_class)->dispose (object);
}


struct _ProcessWatch {
  KgxTrain /*weak*/ *train;
  KgxProcess *process;
};


KGX_DEFINE_DATA (ProcessWatch, process_watch)


static void
process_watch_cleanup (ProcessWatch *watch)
{
  g_clear_pointer (&watch->process, kgx_process_unref);
  g_clear_weak_pointer (&watch->train);
}


static gboolean
handle_watch_iter (gpointer pid,
                   gpointer val,
                   gpointer user_data)
{
  KgxProcess *process = val;
  KgxWatcher *self = user_data;
  GPid parent = kgx_process_get_parent (process);
  ProcessWatch *watch = NULL;

  watch = g_tree_lookup (self->watching, GINT_TO_POINTER (parent));

  // There are far more processes on the system than there are children
  // of watches, thus lookup are unlikly
  if (G_UNLIKELY (watch != NULL)) {

    /* If the page died we stop caring about its processes */
    if (G_UNLIKELY (watch->train == NULL)) {
      g_tree_remove (self->watching, GINT_TO_POINTER (parent));
      g_tree_remove (self->children, pid);

      return FALSE;
    }

    if (!g_tree_lookup (self->children, pid)) {
      ProcessWatch *child_watch = process_watch_alloc ();

      child_watch->process = g_rc_box_acquire (process);
      g_set_weak_pointer (&child_watch->train, watch->train);

      g_debug ("watcher: Hello %i!", GPOINTER_TO_INT (pid));

      g_tree_insert (self->children, pid, child_watch);
    }

    kgx_train_push_child (watch->train, process);
  }

  return FALSE;
}


struct RemoveDead {
  GTree     *plist;
  GPtrArray *dead;
};


static gboolean
remove_dead (gpointer pid,
             gpointer val,
             gpointer user_data)
{
  struct RemoveDead *data = user_data;
  ProcessWatch *watch = val;

  if (!g_tree_lookup (data->plist, pid)) {
    g_debug ("watcher: %i marked as dead", GPOINTER_TO_INT (pid));

    if (G_LIKELY (watch->train)) {
      kgx_train_pop_child (watch->train, watch->process);
    }

    g_ptr_array_add (data->dead, pid);
  }

  return FALSE;
}


static gboolean
watch (gpointer data)
{
  KgxWatcher *self = KGX_WATCHER (data);
  g_autoptr (GTree) plist = NULL;
  struct RemoveDead dead;

  plist = kgx_process_get_list ();

  g_tree_foreach (plist, handle_watch_iter, self);

  dead.plist = plist;
  dead.dead = g_ptr_array_new_full (1, NULL);

  g_tree_foreach (self->children, remove_dead, &dead);

  // We can't modify self->chilren whilst walking it
  for (int i = 0; i < dead.dead->len; i++) {
    g_tree_remove (self->children, g_ptr_array_index (dead.dead, i));
  }

  g_ptr_array_unref (dead.dead);

  return G_SOURCE_CONTINUE;
}


static inline gboolean
update_watcher (KgxWatcher *self, gboolean in_background)
{
  if (self->in_background == in_background) {
    return FALSE;
  }

  self->in_background = in_background;

  g_debug ("watcher: in_background? %s", in_background ? "yes" : "no");

  g_clear_handle_id (&self->timeout, g_source_remove);

  /* Slow down polling when in the background */
  self->timeout = g_timeout_add_full (G_PRIORITY_DEFAULT_IDLE,
                                      in_background ? 2000 : 500,
                                      watch,
                                      g_object_ref (self),
                                      g_object_unref);
  g_source_set_name_by_id (self->timeout, "[kgx] watcher");

  return TRUE;
}


static void
kgx_watcher_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  KgxWatcher *self = KGX_WATCHER (object);

  switch (property_id) {
    case PROP_IN_BACKGROUND:
      if (update_watcher (self, g_value_get_boolean (value))) {
        g_object_notify_by_pspec (object, pspec);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
kgx_watcher_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  KgxWatcher *self = KGX_WATCHER (object);

  switch (property_id) {
    case PROP_IN_BACKGROUND:
      g_value_set_boolean (value, self->in_background);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
kgx_watcher_class_init (KgxWatcherClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = kgx_watcher_dispose;
  object_class->set_property = kgx_watcher_set_property;
  object_class->get_property = kgx_watcher_get_property;

  pspecs[PROP_IN_BACKGROUND] = g_param_spec_boolean ("in-background", NULL, NULL,
                                                     FALSE,
                                                     G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);
}


static void
kgx_watcher_init (KgxWatcher *self)
{
  self->watching = g_tree_new_full (kgx_pid_cmp,
                                    NULL,
                                    NULL,
                                    process_watch_free);
  self->children = g_tree_new_full (kgx_pid_cmp,
                                    NULL,
                                    NULL,
                                    process_watch_free);
}


/**
 * kgx_watcher_watch:
 * @self: the #KgxWatcher
 * @train: the #KgxTrain to watch
 *
 * Registers a new shell process with the pid watcher
 */
void
kgx_watcher_watch (KgxWatcher *self,
                   KgxTrain   *train)
{
  ProcessWatch *watch;
  GPid pid;

  g_return_if_fail (KGX_IS_WATCHER (self));
  g_return_if_fail (KGX_IS_TRAIN (train));

  pid = kgx_train_get_pid (train);

  watch = process_watch_alloc ();
  watch->process = kgx_process_new (pid);
  g_set_weak_pointer (&watch->train, train);

  g_debug ("watcher: tracking %i", pid);

  g_tree_insert (self->watching, GINT_TO_POINTER (pid), watch);
}
