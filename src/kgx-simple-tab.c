/* kgx-local-page.c
 *
 * Copyright 2019-2020 Zander Brown
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
 *
 * Since: 0.3.0
 */

#include "kgx-config.h"

#include <glib/gi18n.h>

#include "kgx-terminal.h"
#include "kgx-proxy-info.h"
#include "kgx-simple-tab.h"
#include "fp-vte-util.h"


G_DEFINE_TYPE (KgxSimpleTab, kgx_simple_tab, KGX_TYPE_TAB)

enum {
  PROP_0,
  PROP_INITIAL_WORK_DIR,
  PROP_COMMAND,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };


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
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
kgx_simple_tab_finalize (GObject *object)
{
  KgxSimpleTab *self = KGX_SIMPLE_TAB (object);

  g_clear_pointer (&self->initial_work_dir, g_free);
  g_clear_pointer (&self->command, g_strfreev);

  G_OBJECT_CLASS (kgx_simple_tab_parent_class)->finalize (object);
}


static void
wait_cb (GPid     pid,
         gint     status,
         gpointer user_data)

{
  KgxSimpleTab *self = user_data;
  g_autoptr (GError) error = NULL;

  g_return_if_fail (KGX_SIMPLE_TAB (self));

  /* wait_check will set @error if it got a signal/non-zero exit */
  if (!g_spawn_check_exit_status (status, &error)) {
    g_autofree char *message = NULL;

    // translators: <b> </b> marks the text as bold, ensure they are
    // matched please!
    message = g_strdup_printf (_("<b>Read Only</b> — Command exited with code %i"),
                               status);

    kgx_tab_died (KGX_TAB (self), GTK_MESSAGE_ERROR, message, TRUE);
  } else {
    kgx_tab_died (KGX_TAB (self),
                  GTK_MESSAGE_INFO,
    // translators: <b> </b> marks the text as bold, ensure they are
    // matched please!
                  _("<b>Read Only</b> — Command exited"),
                  TRUE);
  }
}


struct StartData {
  KgxSimpleTab *self;
  GTask *task;
};


static void
spawned (VtePty       *pty,
         GAsyncResult *res,
         gpointer      udata)

{
  struct StartData *data = udata;
  g_autoptr (GError) error = NULL;
  GPid pid;

  g_return_if_fail (VTE_IS_PTY (pty));
  g_return_if_fail (G_IS_ASYNC_RESULT (res));

  fp_vte_pty_spawn_finish (pty, res, &pid, &error);

  if (error) {
    g_autofree char *message = NULL;

    // translators: <b> </b> marks the text as bold, ensure they are
    // matched please!
    message = g_strdup_printf (_("<b>Failed to start</b> — %s"),
                               error->message);

    kgx_tab_died (KGX_TAB (data->self), GTK_MESSAGE_ERROR, message, FALSE);

    g_task_return_error (data->task, g_steal_pointer (&error));

    g_object_unref (data->self);
    g_free (data);

    return;
  }

  g_task_return_int (G_TASK (data->task), pid);

  g_child_watch_add (pid, wait_cb, data->self);

  g_object_unref (data->self);
  g_free (data);
}


static void
kgx_simple_tab_start (KgxTab              *page,
                      GAsyncReadyCallback  callback,
                      gpointer             callback_data)
{
  KgxSimpleTab       *self;
  g_autoptr (VtePty)  pty = NULL;
  g_autoptr (GError)  error = NULL;
  g_auto (GStrv)      env = NULL;
  struct StartData   *data;
  GTask              *task;

  g_return_if_fail (KGX_IS_SIMPLE_TAB (page));

  self = KGX_SIMPLE_TAB (page);

  pty = vte_pty_new_sync (VTE_PTY_DEFAULT, NULL, &error);

  env = g_environ_setenv (env, "TERM", "xterm-256color", TRUE);

  vte_terminal_set_pty (VTE_TERMINAL (self->terminal), pty);

  task = g_task_new (self, NULL, callback, callback_data);

  data = g_new (struct StartData, 1);
  data->self = g_object_ref (self);
  data->task = task;

  kgx_proxy_info_apply_to_environ (kgx_proxy_info_get_default (), &env);

  fp_vte_pty_spawn_async (pty,
                          self->initial_work_dir,
                          (const char *const *) self->command,
                          (const char *const *) env,
                          -1,
                          NULL,
                          (GAsyncReadyCallback) spawned,
                          data);
}


static GPid
kgx_simple_tab_start_finish (KgxTab        *page,
                             GAsyncResult  *res,
                             GError       **error)
{
  g_return_val_if_fail (g_task_is_valid (res, page), 0);

  return g_task_propagate_int (G_TASK (res), error);
}


static void
path_changed (GObject *object, GParamSpec *pspec, KgxSimpleTab *self)
{
  g_autofree char *desc = NULL;
  g_autoptr (GFile) path = NULL;

  g_object_get (self->terminal, "path", &path, NULL);

  if (path) {
    desc = g_strdup_printf ("%s", g_file_get_path (path));
  }

  g_object_set (self, "tab-tooltip", desc, NULL);
}


static void
kgx_simple_tab_class_init (KgxSimpleTabClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS   (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  KgxTabClass    *page_class   = KGX_TAB_CLASS    (klass);

  object_class->set_property = kgx_simple_tab_set_property;
  object_class->get_property = kgx_simple_tab_get_property;
  object_class->finalize = kgx_simple_tab_finalize;

  page_class->start = kgx_simple_tab_start;
  page_class->start_finish = kgx_simple_tab_start_finish;

  /**
   * KgxSimpleTab:initial-work-dir:
   *
   * Used to handle --working-dir
   *
   * Since: 0.3.0
   */
  pspecs[PROP_INITIAL_WORK_DIR] =
    g_param_spec_string ("initial-work-dir", "Initial directory",
                         "Initial working directory",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  /**
   * KgxSimpleTab:command:
   *
   * Used to handle -e
   *
   * Since: 0.3.0
   */
  pspecs[PROP_COMMAND] =
    g_param_spec_boxed ("command", "Command",
                        "Command to run",
                        G_TYPE_STRV,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               KGX_APPLICATION_PATH "kgx-simple-tab.ui");

  gtk_widget_class_bind_template_child (widget_class, KgxSimpleTab, terminal);

  gtk_widget_class_bind_template_callback (widget_class, path_changed);
}


static void
kgx_simple_tab_init (KgxSimpleTab *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  kgx_tab_connect_terminal (KGX_TAB (self), KGX_TERMINAL (self->terminal));
}
