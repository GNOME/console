/* kgx-despatcher.c
 *
 * Copyright 2023 Zander Brown
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

#include "xdg-fm1.h"

#include "kgx-despatcher.h"
#include "kgx-marshals.h"


struct _KgxDespatcher {
  GObject             parent_instance;
  XdgFileManager1    *file_manager;
};


G_DEFINE_TYPE (KgxDespatcher, kgx_despatcher, G_TYPE_OBJECT)


static void
kgx_despatcher_dispose (GObject *object)
{
  KgxDespatcher *self = KGX_DESPATCHER (object);

  g_clear_object (&self->file_manager);

  G_OBJECT_CLASS (kgx_despatcher_parent_class)->dispose (object);
}


static void
kgx_despatcher_class_init (KgxDespatcherClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = kgx_despatcher_dispose;
}


static void
kgx_despatcher_init (KgxDespatcher *self)
{
}


typedef struct {
  KgxDespatcher *self;
  GtkWindow      *window;
  char           *uri;
} LaunchData;


static void
launch_data_free (gpointer data)
{
  LaunchData *self = data;

  if (!self) {
    return;
  }

  g_clear_object (&self->self);
  g_clear_object (&self->window);
  g_clear_pointer (&self->uri, g_free);

  g_free (self);
}


G_DEFINE_AUTOPTR_CLEANUP_FUNC (LaunchData, launch_data_free)


static void
did_open_after_mount (GObject *source, GAsyncResult *res, gpointer data)
{
  g_autoptr (GError) error = NULL;
  GTask *task = data;

  kgx_despatcher_open_finish (KGX_DESPATCHER (source), res, &error);

  if (error) {
    g_task_return_error (task, g_steal_pointer (&error));

    return;
  }

  g_task_return_boolean (task, FALSE);
}


static void
did_mount (GObject *source, GAsyncResult *res, gpointer data)
{
  g_autoptr (GError) error = NULL;
  GTask *task = data;
  LaunchData *state = g_task_get_task_data (task);

  g_file_mount_enclosing_volume_finish (G_FILE (source), res, &error);

  if (error) {
    g_task_return_error (task, g_steal_pointer (&error));

    return;
  }

  g_debug ("despatcher: try again after mount");

  kgx_despatcher_open (state->self,
                       state->uri,
                       state->window,
                       g_task_get_cancellable (task),
                       did_open_after_mount,
                       task);
}


static void
did_launch (GObject *source, GAsyncResult *res, gpointer data)
{
  g_autoptr (GError) error = NULL;
  GTask *task = data;
  LaunchData *state = g_task_get_task_data (task);
  GCancellable *cancellable = g_task_get_cancellable (task);

  g_app_info_launch_default_for_uri_finish (res, &error);

  if (error) {
    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_MOUNTED)) {
      g_autoptr (GFile) file = g_file_new_for_uri (state->uri);
      g_autoptr (GMountOperation) op = gtk_mount_operation_new (state->window);

      g_debug ("despatcher: try mount: %s", state->uri);

      g_file_mount_enclosing_volume (file,
                                     G_MOUNT_MOUNT_NONE,
                                     op,
                                     cancellable,
                                     did_mount,
                                     task);

      return;
    }

    g_task_return_error (task, g_steal_pointer (&error));

    return;
  }

  g_task_return_boolean (task, FALSE);
}


void
kgx_despatcher_open (KgxDespatcher      *self,
                     const char          *uri,
                     GtkWindow           *window,
                     GCancellable        *cancellable,
                     GAsyncReadyCallback  callback,
                     gpointer             user_data)
{
  g_autoptr (LaunchData) state = g_new0 (LaunchData, 1);
  g_autoptr (GdkAppLaunchContext) context = NULL;
  GTask *task;
  GdkDisplay *display;

  g_debug ("despatcher: open: %s", uri);

  g_return_if_fail (KGX_IS_DESPATCHER (self));
  g_return_if_fail (uri != NULL);
  g_return_if_fail (GTK_IS_WINDOW (window));

  g_set_object (&state->self, self);
  g_set_object (&state->window, window);
  g_set_str (&state->uri, uri);

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_task_data (task, g_steal_pointer (&state), launch_data_free);
  g_task_set_source_tag (task, kgx_despatcher_open);

  display = gtk_widget_get_display (GTK_WIDGET (window));
  context = gdk_display_get_app_launch_context (display);

  g_app_info_launch_default_for_uri_async (uri,
                                           G_APP_LAUNCH_CONTEXT (context),
                                           cancellable,
                                           did_launch,
                                           task);
}


void
kgx_despatcher_open_finish (KgxDespatcher  *self,
                            GAsyncResult   *result,
                            GError        **error)
{
  g_return_if_fail (KGX_IS_DESPATCHER (self));
  g_return_if_fail (G_IS_TASK (result));

  g_task_propagate_boolean (G_TASK (result), error);
}


typedef struct {
  KgxDespatcher *self;
  GStrv          uris;
} ShowData;


static void
show_data_free (gpointer data)
{
  ShowData *self = data;

  g_clear_object (&self->self);
  g_clear_pointer (&self->uris, g_strfreev);

  g_free (self);
}


G_DEFINE_AUTOPTR_CLEANUP_FUNC (ShowData, show_data_free)


static inline ShowData *
show_data_new (KgxDespatcher *despatcher,
               const char    *uri)
{
  g_autoptr (ShowData) self = g_new0 (ShowData, 1);
  g_autoptr (GStrvBuilder) builder = g_strv_builder_new ();

  g_set_object (&self->self, despatcher);
  g_strv_builder_add (builder, uri);
  self->uris = g_strv_builder_end (builder);

  return g_steal_pointer (&self);
}


static void
complete_call (GObject *source, GAsyncResult *res, gpointer data)
{
  g_autoptr (GError) error = NULL;
  GTask *task = data;

  if (G_LIKELY (g_async_result_is_tagged (G_ASYNC_RESULT (task), kgx_despatcher_show_folder))) {
    xdg_file_manager1_call_show_folders_finish (XDG_FILE_MANAGER1 (source),
                                                res,
                                                &error);
  } else {
    xdg_file_manager1_call_show_items_finish (XDG_FILE_MANAGER1 (source),
                                              res,
                                              &error);
  }

  if (error) {
    g_debug ("despatcher: fm: bus call failed %s", error->message);
    g_task_return_error (task, g_steal_pointer (&error));
    return;
  }

  g_task_return_boolean (task, FALSE);
}


static void
make_call (GTask *task)
{
  ShowData *state = g_task_get_task_data (task);

  if (G_LIKELY (g_async_result_is_tagged (G_ASYNC_RESULT (task), kgx_despatcher_show_folder))) {
    xdg_file_manager1_call_show_folders (state->self->file_manager,
                                         (const char *const *) state->uris,
                                         "",
                                         NULL,
                                         complete_call,
                                         task);
  } else {
    xdg_file_manager1_call_show_items (state->self->file_manager,
                                       (const char *const *) state->uris,
                                       "",
                                       NULL,
                                       complete_call,
                                       task);
  }
}


static void
got_proxy (GObject *source, GAsyncResult *res, gpointer data)
{
  g_autoptr (GError) error = NULL;
  g_autoptr (XdgFileManager1) fm = NULL;
  GTask *task = data;
  ShowData *state = g_task_get_task_data (task);

  fm = xdg_file_manager1_proxy_new_finish (res, &error);

  if (error) {
    g_debug ("despatcher: fm: bus connect failed %s", error->message);
    g_task_return_error (task, g_steal_pointer (&error));
    return;
  }

  g_debug ("despatcher: fm: got proxy");

  g_set_object (&state->self->file_manager, fm);

  make_call (task);
}


static void
get_file_manager (GTask *task)
{
  xdg_file_manager1_proxy_new_for_bus (G_BUS_TYPE_SESSION,
                                       G_DBUS_PROXY_FLAGS_NONE,
                                       "org.freedesktop.FileManager1",
                                       "/org/freedesktop/FileManager1",
                                       NULL,
                                       got_proxy,
                                       task);
}


static GTask *
show_task_new (KgxDespatcher       *self,
               const char          *uri,
               GtkWindow           *window,
               GCancellable        *cancellable,
               GAsyncReadyCallback  callback,
               gpointer             user_data)
{
  g_autoptr (GTask) task = NULL;
  g_autoptr (ShowData) state = NULL;

  g_debug ("despatcher: show: %s", uri);

  g_return_val_if_fail (KGX_IS_DESPATCHER (self), NULL);
  g_return_val_if_fail (uri != NULL, NULL);
  g_return_val_if_fail (GTK_IS_WINDOW (window), NULL);

  state = show_data_new (self, uri);

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_task_data (task, g_steal_pointer (&state), show_data_free);

  return g_steal_pointer (&task);
}


void
kgx_despatcher_show_item (KgxDespatcher       *self,
                          const char          *uri,
                          GtkWindow           *window,
                          GCancellable        *cancellable,
                          GAsyncReadyCallback  callback,
                          gpointer             user_data)
{
  GTask *task = show_task_new (self, uri, window, cancellable, callback, user_data);

  g_task_set_source_tag (task, kgx_despatcher_show_item);

  if (self->file_manager) {
    make_call (task);
  } else {
    get_file_manager (task);
  }
}


void
kgx_despatcher_show_item_finish (KgxDespatcher  *self,
                                 GAsyncResult   *result,
                                 GError        **error)
{
  g_return_if_fail (KGX_IS_DESPATCHER (self));
  g_return_if_fail (G_IS_TASK (result));

  g_task_propagate_boolean (G_TASK (result), error);
}


void
kgx_despatcher_show_folder (KgxDespatcher       *self,
                            const char          *uri,
                            GtkWindow           *window,
                            GCancellable        *cancellable,
                            GAsyncReadyCallback  callback,
                            gpointer             user_data)
{
  GTask *task = show_task_new (self, uri, window, cancellable, callback, user_data);

  g_task_set_source_tag (task, kgx_despatcher_show_folder);

  if (self->file_manager) {
    make_call (task);
  } else {
    get_file_manager (task);
  }
}


void
kgx_despatcher_show_folder_finish (KgxDespatcher  *self,
                                   GAsyncResult   *result,
                                   GError        **error)
{
  g_return_if_fail (KGX_IS_DESPATCHER (self));
  g_return_if_fail (G_IS_TASK (result));

  g_task_propagate_boolean (G_TASK (result), error);
}
