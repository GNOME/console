/* kgx-train.c
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

#include "kgx-config.h"

#include <gio/gio.h>

#include "kgx-enums.h"
#include "kgx-marshals.h"
#include "kgx-utils.h"

#include "kgx-train.h"


typedef struct _KgxTrainPrivate KgxTrainPrivate;
struct _KgxTrainPrivate {
  char *uuid;
  char *tag;
  GPid pid;
  KgxStatus status;

  GHashTable *root;
  GHashTable *remote;
  GHashTable *children;
};

static void kgx_train_async_initiable_iface_init (GAsyncInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (KgxTrain, kgx_train, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (KgxTrain)
                         G_IMPLEMENT_INTERFACE (G_TYPE_ASYNC_INITABLE,
                                                kgx_train_async_initiable_iface_init))


enum {
  PROP_0,
  PROP_UUID,
  PROP_TAG,
  PROP_PID,
  PROP_STATUS,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };


enum {
  PID_DIED,
  CHILD_ADDED,
  CHILD_REMOVED,
  N_SIGNALS
};
static guint signals[N_SIGNALS];


static void
kgx_train_dispose (GObject *object)
{
  KgxTrainPrivate *priv = kgx_train_get_instance_private (KGX_TRAIN (object));

  g_clear_pointer (&priv->uuid, g_free);
  g_clear_pointer (&priv->tag, g_free);

  g_clear_pointer (&priv->root, g_hash_table_unref);
  g_clear_pointer (&priv->remote, g_hash_table_unref);
  g_clear_pointer (&priv->children, g_hash_table_unref);

  G_OBJECT_CLASS (kgx_train_parent_class)->dispose (object);
}


static void
kgx_train_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  KgxTrainPrivate *priv = kgx_train_get_instance_private (KGX_TRAIN (object));

  switch (property_id) {
    case PROP_UUID:
      g_value_set_string (value, priv->uuid);
      break;
    case PROP_TAG:
      g_value_set_string (value, priv->tag);
      break;
    case PROP_PID:
      g_value_set_int (value, priv->pid);
      break;
    case PROP_STATUS:
      g_value_set_flags (value, priv->status);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
kgx_train_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  KgxTrainPrivate *priv = kgx_train_get_instance_private (KGX_TRAIN (object));

  switch (property_id) {
    case PROP_TAG:
      if (g_set_str (&priv->tag, g_value_get_string (value))) {
        g_object_notify_by_pspec (object, pspec);
      }
      break;
    case PROP_PID:
      priv->pid = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
kgx_train_class_init (KgxTrainClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = kgx_train_dispose;
  object_class->get_property = kgx_train_get_property;
  object_class->set_property = kgx_train_set_property;

  pspecs[PROP_UUID] =
    g_param_spec_string ("uuid", NULL, NULL,
                         NULL,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_TAG] =
    g_param_spec_string ("tag", NULL, NULL,
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /* We've assumed that GPid fits in GParamSpecInt */
  { G_STATIC_ASSERT (sizeof (GPid) == sizeof (int)); }

  pspecs[PROP_PID] =
    g_param_spec_int ("pid", NULL, NULL,
                      G_MININT, G_MAXINT, 0,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_STATUS] =
    g_param_spec_flags ("status", NULL, NULL,
                        KGX_TYPE_STATUS,
                        KGX_NONE,
                        G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);

  signals[PID_DIED] = g_signal_new ("pid-died",
                                    G_TYPE_FROM_CLASS (klass),
                                    G_SIGNAL_RUN_LAST,
                                    0,
                                    NULL, NULL,
                                    kgx_marshals_VOID__INT,
                                    G_TYPE_NONE,
                                    1, G_TYPE_INT);
  g_signal_set_va_marshaller (signals[PID_DIED],
                              G_TYPE_FROM_CLASS (klass),
                              kgx_marshals_VOID__INTv);

  signals[CHILD_ADDED] = g_signal_new ("child-added",
                                       G_TYPE_FROM_CLASS (klass),
                                       G_SIGNAL_RUN_LAST,
                                       0,
                                       NULL, NULL,
                                       kgx_marshals_VOID__BOXED,
                                       G_TYPE_NONE,
                                       1,
                                       KGX_TYPE_PROCESS | G_SIGNAL_TYPE_STATIC_SCOPE);
  g_signal_set_va_marshaller (signals[CHILD_ADDED],
                              G_TYPE_FROM_CLASS (klass),
                              kgx_marshals_VOID__BOXEDv);

  signals[CHILD_REMOVED] = g_signal_new ("child-removed",
                                         G_TYPE_FROM_CLASS (klass),
                                         G_SIGNAL_RUN_LAST,
                                         0,
                                         NULL, NULL,
                                         kgx_marshals_VOID__BOXED,
                                         G_TYPE_NONE,
                                         1,
                                         KGX_TYPE_PROCESS | G_SIGNAL_TYPE_STATIC_SCOPE);
  g_signal_set_va_marshaller (signals[CHILD_REMOVED],
                              G_TYPE_FROM_CLASS (klass),
                              kgx_marshals_VOID__BOXEDv);
}


struct _WaitData {
  KgxTrain *self;
};


KGX_DEFINE_DATA (WaitData, wait_data)


static void
wait_data_cleanup (WaitData *self)
{
  g_clear_weak_pointer (&self->self);
}


static void
process_died (GPid     pid,
              int      status,
              gpointer user_data)

{
  WaitData *data = user_data;

  if (!data->self) {
    return; /* Train destroyed before the process actually died */
  }

  g_spawn_close_pid (pid);

  g_signal_emit (data->self, signals[PID_DIED], 0, status);
}


static void
kgx_train_async_initiable_init_async (GAsyncInitable      *initable,
                                      int                  io_priority,
                                      GCancellable        *cancellable,
                                      GAsyncReadyCallback  callback,
                                      gpointer             user_data)
{
  g_autoptr (WaitData) wait_data = wait_data_alloc ();
  GTask *task = g_task_new (initable, cancellable, callback, user_data);
  KgxTrain *self = KGX_TRAIN (initable);
  KgxTrainPrivate *priv = kgx_train_get_instance_private (self);

  g_task_set_source_tag (task, kgx_train_async_initiable_init_async);

  g_set_weak_pointer (&wait_data->self, self);

  g_child_watch_add_full (G_PRIORITY_HIGH_IDLE,
                          priv->pid,
                          process_died,
                          g_steal_pointer (&wait_data),
                          wait_data_free);

  g_task_return_boolean (task, TRUE);
}


static gboolean
kgx_train_async_initiable_init_finish (GAsyncInitable  *initable,
                                       GAsyncResult    *res,
                                       GError         **error)
{
  return g_task_propagate_boolean (G_TASK (res), error);
}


static void
kgx_train_async_initiable_iface_init (GAsyncInitableIface *iface)
{
  iface->init_async = kgx_train_async_initiable_init_async;
  iface->init_finish = kgx_train_async_initiable_init_finish;
}


static void
kgx_train_init (KgxTrain *self)
{
  KgxTrainPrivate *priv = kgx_train_get_instance_private (self);

  priv->uuid = g_uuid_string_random ();

  priv->root = g_hash_table_new (g_direct_hash, g_direct_equal);
  priv->remote = g_hash_table_new (g_direct_hash, g_direct_equal);
  priv->children = g_hash_table_new_full (g_direct_hash,
                                          g_direct_equal,
                                          NULL,
                                          (GDestroyNotify) kgx_process_unref);
}


const char *
kgx_train_get_uuid (KgxTrain *self)
{
  KgxTrainPrivate *priv;

  g_return_val_if_fail (KGX_IS_TRAIN (self), NULL);

  priv = kgx_train_get_instance_private (self);

  return priv->uuid;
}


const char *
kgx_train_get_tag (KgxTrain *self)
{
  KgxTrainPrivate *priv;

  g_return_val_if_fail (KGX_IS_TRAIN (self), NULL);

  priv = kgx_train_get_instance_private (self);

  return priv->tag;
}


GPid
kgx_train_get_pid (KgxTrain *self)
{
  KgxTrainPrivate *priv;

  g_return_val_if_fail (KGX_IS_TRAIN (self), -1);

  priv = kgx_train_get_instance_private (self);

  return priv->pid;
}


/**
 * kgx_train_get_children:
 * @self: the #KgxTrain
 *
 * Get a list of child process running in @self
 *
 * NOTE: This doesn't include the shell/root itself
 *
 * Returns: (element-type Kgx.Process) (transfer full): the list of #KgxProcess
 */
GPtrArray *
kgx_train_get_children (KgxTrain *self)
{
  KgxTrainPrivate *priv;
  GPtrArray *children;
  GHashTableIter iter;
  gpointer pid, process;

  g_return_val_if_fail (KGX_IS_TRAIN (self), NULL);

  priv = kgx_train_get_instance_private (self);

  children = g_ptr_array_new_full (3, (GDestroyNotify) kgx_process_unref);

  g_hash_table_iter_init (&iter, priv->children);
  while (g_hash_table_iter_next (&iter, &pid, &process)) {
    g_ptr_array_add (children, g_rc_box_acquire (process));
  }

  return children;
}


static inline void
set_status (KgxTrain  *self,
            KgxStatus  status)
{
  KgxTrainPrivate *priv = kgx_train_get_instance_private (self);

  if (priv->status == status) {
    return;
  }

  priv->status = status;

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_STATUS]);
}


static inline KgxStatus
push_type (GHashTable *table,
           GPid        pid,
           KgxProcess *process,
           KgxStatus   status)
{
  g_hash_table_insert (table,
                       GINT_TO_POINTER (pid),
                       process != NULL ? g_rc_box_acquire (process) : NULL);

  g_debug ("train: Now %i %X", g_hash_table_size (table), status);

  return status;
}


void
kgx_train_push_child (KgxTrain   *self,
                      KgxProcess *process)
{
  GPid pid = 0;
  GStrv argv;
  KgxStatus new_status = KGX_NONE;
  KgxTrainPrivate *priv;

  g_return_if_fail (KGX_IS_TRAIN (self));

  priv = kgx_train_get_instance_private (self);

  pid = kgx_process_get_pid (process);
  argv = kgx_process_get_argv (process);

  if (G_LIKELY (argv[0] != NULL)) {
    g_autofree char *program = g_path_get_basename (argv[0]);

    if (G_UNLIKELY (g_strcmp0 (program, "ssh") == 0 ||
                    g_strcmp0 (program, "telnet") == 0 ||
                    g_strcmp0 (program, "mosh-client") == 0 ||
                    g_strcmp0 (program, "mosh") == 0 ||
                    g_strcmp0 (program, "et") == 0)) {
      new_status |= push_type (priv->remote, pid, NULL, KGX_REMOTE);
    }

    if (G_UNLIKELY (g_strcmp0 (program, "waypipe") == 0)) {
      for (int i = 1; argv[i]; i++) {
        if (G_UNLIKELY (g_strcmp0 (argv[i], "ssh") == 0 ||
                        g_strcmp0 (argv[i], "telnet") == 0)) {
          new_status |= push_type (priv->remote, pid, NULL, KGX_REMOTE);
          break;
        }
      }
    }
  }

  if (G_UNLIKELY (kgx_process_get_is_root (process))) {
    new_status |= push_type (priv->root, pid, NULL, KGX_PRIVILEGED);
  }

  push_type (priv->children, pid, process, KGX_NONE);

  set_status (self, new_status);

  g_signal_emit (self, signals[CHILD_ADDED], 0, process);
}


inline static KgxStatus
pop_type (GHashTable *table,
          GPid        pid,
          KgxStatus   status)
{
  guint size = 0;

  g_hash_table_remove (table, GINT_TO_POINTER (pid));

  size = g_hash_table_size (table);

  if (G_LIKELY (size <= 0)) {
    g_debug ("train: No longer %X", status);

    return KGX_NONE;
  } else {
    g_debug ("train: %i %X remaining", size, status);

    return status;
  }
}


void
kgx_train_pop_child (KgxTrain   *self,
                     KgxProcess *process)
{
  GPid pid = 0;
  KgxStatus new_status = KGX_NONE;
  KgxTrainPrivate *priv;

  g_return_if_fail (KGX_IS_TRAIN (self));

  priv = kgx_train_get_instance_private (self);

  pid = kgx_process_get_pid (process);

  new_status |= pop_type (priv->remote, pid, KGX_REMOTE);
  new_status |= pop_type (priv->root, pid, KGX_PRIVILEGED);
  pop_type (priv->children, pid, KGX_NONE);

  set_status (self, new_status);

  g_signal_emit (self, signals[CHILD_REMOVED], 0, process);
}
