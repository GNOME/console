#include <gio/gio.h>
#include <glibtop/proclist.h>
#include <glibtop/procuid.h>
#include <glibtop/procargs.h>

#include "kgx-process.h"

struct _KgxProcess {
  GPid    pid;
  GPid    parent;
  gint32  uid;
  char   *exec;
};

static void
clear (KgxProcess *self)
{
  g_free (self->exec);
}

void
kgx_process_unref (KgxProcess *self)
{
  g_return_if_fail (self != NULL);

  g_rc_box_release_full (self, (GDestroyNotify) clear);
}

G_DEFINE_BOXED_TYPE (KgxProcess, kgx_process, g_rc_box_acquire, kgx_process_unref)

KgxProcess *
kgx_process_new (GPid pid)
{
  glibtop_proc_uid    info;
  glibtop_proc_args   args_size;
  char              **args;
  KgxProcess          *self = g_rc_box_new0 (KgxProcess);

  self->pid = pid;

  glibtop_get_proc_uid (&info, pid);

  self->parent = info.ppid;
  self->uid = info.uid;

  args = glibtop_get_proc_argv (&args_size, pid, 0);

  self->exec = g_strdup (args[0]);

  g_strfreev (args);

  return self;
}

GPid
kgx_process_get_pid (KgxProcess *self)
{
  g_return_val_if_fail (self != NULL, 0);

  return self->pid;
}

gint32
kgx_process_get_uid (KgxProcess *self)
{
  g_return_val_if_fail (self != NULL, 0);

  return self->uid;
}

gboolean
kgx_process_get_is_root (KgxProcess *self)
{
  g_return_val_if_fail (self != NULL, 0);

  return self->uid == 0;
}

KgxProcess *
kgx_process_get_parent (KgxProcess *self)
{
  g_return_val_if_fail (self != NULL, 0);

  return kgx_process_new (self->parent);
}

const char *
kgx_process_get_exec (KgxProcess *self)
{
  g_return_val_if_fail (self != NULL, 0);

  return self->exec;
}

// Caller must call g_ptr_array_unref
GPtrArray *
kgx_process_get_list ()
{
  glibtop_proclist pid_list;
  GPid *pids;
  GPtrArray *list;
  
  list = g_ptr_array_new_with_free_func ((GDestroyNotify) kgx_process_unref);

  pids = glibtop_get_proclist (&pid_list, GLIBTOP_KERN_PROC_ALL, 0);

  for (int i = 0; i < pid_list.number; i++) {
    g_ptr_array_add (list, kgx_process_new (pids[i]));
  }

  return list;
}
