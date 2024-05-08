/* kgx-local-tab.c
 *
 * Copyright 2019-2023 Zander Brown
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

/**
 * SECTION:kgx-simple-tab
 * @title: KgxSimpleTab
 * @short_description: #KgxTab for an old fashioned local terminal
 */

#include "kgx-config.h"

#include <glib/gi18n.h>

#include "fp-vte-util.h"

#include "kgx-proxy-info.h"
#include "kgx-utils.h"

#include "kgx-simple-tab.h"


struct _KgxSimpleTab {
  KgxTab        parent_instance;

  char         *title;
  GFile        *path;

  char         *initial_work_dir;
  GStrv         command;
  GStrv         environ;

  GtkWidget    *terminal;
  GCancellable *spawn_cancellable;
};


G_DEFINE_TYPE (KgxSimpleTab, kgx_simple_tab, KGX_TYPE_TAB)

enum {
  PROP_0,
  PROP_INITIAL_WORK_DIR,
  PROP_COMMAND,
  PROP_ENVIRON,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };


static void
kgx_simple_tab_dispose (GObject *object)
{
  KgxSimpleTab *self = KGX_SIMPLE_TAB (object);

  g_clear_pointer (&self->initial_work_dir, g_free);
  g_clear_pointer (&self->command, g_strfreev);
  g_clear_pointer (&self->environ, g_strfreev);

  g_cancellable_cancel (self->spawn_cancellable);
  g_clear_object (&self->spawn_cancellable);

  G_OBJECT_CLASS (kgx_simple_tab_parent_class)->dispose (object);
}


static void
kgx_simple_tab_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  KgxSimpleTab *self = KGX_SIMPLE_TAB (object);

  switch (property_id) {
    case PROP_INITIAL_WORK_DIR:
      self->initial_work_dir = g_value_dup_string (value);
      break;
    case PROP_COMMAND:
      self->command = g_value_dup_boxed (value);
      break;
    case PROP_ENVIRON:
      self->environ = g_value_dup_boxed (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
kgx_simple_tab_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  KgxSimpleTab *self = KGX_SIMPLE_TAB (object);

  switch (property_id) {
    case PROP_INITIAL_WORK_DIR:
      g_value_set_string (value, self->initial_work_dir);
      break;
    case PROP_COMMAND:
      g_value_set_boxed (value, self->command);
      break;
    case PROP_ENVIRON:
      g_value_set_boxed (value, self->environ);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


struct _StartData {
  KgxSimpleTab *self;
};


KGX_DEFINE_DATA (StartData, start_data)


static void
start_data_cleanup (StartData *self)
{
  g_clear_weak_pointer (&self->self);
}


struct _WaitData {
  KgxSimpleTab *self;
};


KGX_DEFINE_DATA (WaitData, wait_data)


static void
wait_data_cleanup (WaitData *self)
{
  g_clear_weak_pointer (&self->self);
}


static void
wait_cb (GPid     pid,
         int      status,
         gpointer user_data)

{
  g_autoptr (WaitData) data = user_data;
  g_autoptr (GError) error = NULL;

  if (!data->self) {
    return; /* Tab destroyed before the process actually died */
  }

  g_return_if_fail (KGX_SIMPLE_TAB (data->self));

  /* wait_check will set @error if it got a signal/non-zero exit */
  if (!g_spawn_check_wait_status (status, &error)) {
    g_autofree char *message = NULL;

    // translators: <b> </b> marks the text as bold, ensure they are
    // matched please!
    message = g_strdup_printf (_("<b>Read Only</b> — Command exited with code %i"),
                               status);

    kgx_tab_died (KGX_TAB (data->self), GTK_MESSAGE_ERROR, message, TRUE);
  } else {
    kgx_tab_died (KGX_TAB (data->self),
                  GTK_MESSAGE_INFO,
    // translators: <b> </b> marks the text as bold, ensure they are
    // matched please!
                  _("<b>Read Only</b> — Command exited"),
                  TRUE);
  }
}


static void
spawned (VtePty       *pty,
         GAsyncResult *res,
         gpointer      user_data)

{
  g_autoptr (GTask) task = user_data;
  g_autoptr (WaitData) wait_data = wait_data_alloc ();
  g_autoptr (GError) error = NULL;
  StartData *start_data = kgx_task_get_start_data (task);
  GPid pid;

  g_return_if_fail (VTE_IS_PTY (pty));
  g_return_if_fail (G_IS_ASYNC_RESULT (res));

  fp_vte_pty_spawn_finish (pty, res, &pid, &error);

  if (!start_data->self) {
    return; /* The tab went away whilst we were spawning */
  }

  g_return_if_fail (KGX_SIMPLE_TAB (start_data->self));

  if (error) {
    g_autofree char *message = NULL;

    // translators: <b> </b> marks the text as bold, ensure they are
    // matched please!
    message = g_strdup_printf (_("<b>Failed to start</b> — %s"),
                               error->message);

    kgx_tab_died (KGX_TAB (start_data->self),
                  GTK_MESSAGE_ERROR,
                  message,
                  FALSE);

    g_task_return_error (task, g_steal_pointer (&error));

    return;
  }

  g_set_weak_pointer (&wait_data->self, start_data->self);

  g_child_watch_add (pid, wait_cb, g_steal_pointer (&wait_data));

  g_task_return_int (task, pid);
}


static void
kgx_simple_tab_start (KgxTab              *page,
                      GAsyncReadyCallback  callback,
                      gpointer             callback_data)
{
  g_autoptr (VtePty) pty = NULL;
  g_autoptr (GError) error = NULL;
  g_auto (GStrv) env = NULL;
  g_autoptr (StartData) data = start_data_alloc ();
  g_autoptr (GTask) task = NULL;
  KgxSimpleTab *self;

  g_return_if_fail (KGX_IS_SIMPLE_TAB (page));

  self = KGX_SIMPLE_TAB (page);

  if (self->spawn_cancellable) {
    g_cancellable_reset (self->spawn_cancellable);
  } else {
    self->spawn_cancellable = g_cancellable_new ();
  }

  g_set_weak_pointer (&data->self, self);

  task = g_task_new (self, self->spawn_cancellable, callback, callback_data);
  g_task_set_source_tag (task, kgx_simple_tab_start);
  kgx_task_set_start_data (task, g_steal_pointer (&data));

  pty = vte_pty_new_sync (VTE_PTY_DEFAULT, self->spawn_cancellable, &error);
  if (error) {
    g_task_return_error (task, g_steal_pointer (&error));
    g_clear_object (&self->spawn_cancellable);

    return;
  }

  env = g_strdupv (self->environ);
  env = g_environ_setenv (env, "TERM", "xterm-256color", TRUE);
  env = g_environ_setenv (env, "TERM_PROGRAM", "kgx", TRUE);
  env = g_environ_setenv (env, "TERM_PROGRAM_VERSION", PACKAGE_VERSION, TRUE);

  vte_terminal_set_pty (VTE_TERMINAL (self->terminal), pty);

  kgx_proxy_info_apply_to_environ (kgx_proxy_info_get_default (), &env);

  fp_vte_pty_spawn_async (pty,
                          self->initial_work_dir,
                          (const char *const *) self->command,
                          (const char *const *) env,
                          -1,
                          self->spawn_cancellable,
                          (GAsyncReadyCallback) spawned,
                          g_steal_pointer (&task));
}


static GPid
kgx_simple_tab_start_finish (KgxTab        *page,
                             GAsyncResult  *res,
                             GError       **error)
{
  g_return_val_if_fail (g_task_is_valid (res, page), 0);

  return g_task_propagate_int (G_TASK (res), error);
}


static char *
format_tooltip (GObject *object, GFile *current_path)
{
  g_autofree char *path_raw = NULL;
  g_autofree char *path_utf8 = NULL;
  g_autoptr (GError) error = NULL;

  if (!current_path) {
    return NULL;
  }

  path_raw = g_file_get_path (current_path);
  if (G_UNLIKELY (!path_raw)) {
    return g_file_get_uri (current_path);
  }

  path_utf8 = g_filename_to_utf8 (path_raw, -1, NULL, NULL, &error);
  if (G_UNLIKELY (error)) {
    g_debug ("simple-tab: path had unexpected encoding (%s)", error->message);
    return g_file_get_uri (current_path);
  }

  return g_steal_pointer (&path_utf8);
}


static void
kgx_simple_tab_class_init (KgxSimpleTabClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS   (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  KgxTabClass    *tab_class    = KGX_TAB_CLASS    (klass);

  object_class->dispose = kgx_simple_tab_dispose;
  object_class->set_property = kgx_simple_tab_set_property;
  object_class->get_property = kgx_simple_tab_get_property;

  tab_class->start = kgx_simple_tab_start;
  tab_class->start_finish = kgx_simple_tab_start_finish;

  /**
   * KgxSimpleTab:initial-work-dir:
   *
   * Used to handle --working-dir
   */
  pspecs[PROP_INITIAL_WORK_DIR] =
    g_param_spec_string ("initial-work-dir", NULL, NULL,
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  /**
   * KgxSimpleTab:command:
   *
   * Used to handle -e
   */
  pspecs[PROP_COMMAND] =
    g_param_spec_boxed ("command", NULL, NULL,
                        G_TYPE_STRV,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_ENVIRON] =
    g_param_spec_boxed ("environ", NULL, NULL,
                        G_TYPE_STRV,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               KGX_APPLICATION_PATH "kgx-simple-tab.ui");

  gtk_widget_class_bind_template_child (widget_class, KgxSimpleTab, terminal);

  gtk_widget_class_bind_template_callback (widget_class, format_tooltip);
}


static void
kgx_simple_tab_init (KgxSimpleTab *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
