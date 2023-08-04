/* kgx-drop-target.c
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

#include <gtk/gtk.h>

#include "kgx-drop-target.h"
#include "kgx-marshals.h"

#define PORTAL "application/vnd.portal.filetransfer"
#define PORTAL_OLD "application/vnd.portal.files"
#define URIS "text/uri-list"
#define TEXT "text/plain;charset=utf-8"
#define DROP_PRIORITY G_PRIORITY_HIGH_IDLE
#define DROP_REJECT (0)


struct _KgxDropTarget {
  GObject             parent_instance;

  GtkDropTargetAsync *target;
  gboolean            active;
};


G_DEFINE_TYPE (KgxDropTarget, kgx_drop_target, G_TYPE_OBJECT)


enum {
  PROP_0,
  PROP_ACTIVE,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };


enum {
  DROP,
  N_SIGNALS
};
static guint signals[N_SIGNALS];


static void
kgx_drop_target_dispose (GObject *object)
{
  KgxDropTarget *self = KGX_DROP_TARGET (object);

  g_clear_object (&self->target);

  G_OBJECT_CLASS (kgx_drop_target_parent_class)->dispose (object);
}


static void
kgx_drop_target_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  KgxDropTarget *self = KGX_DROP_TARGET (object);

  switch (property_id) {
    case PROP_ACTIVE:
      g_value_set_boolean (value, self->active);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
kgx_drop_target_class_init (KgxDropTargetClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = kgx_drop_target_dispose;
  object_class->get_property = kgx_drop_target_get_property;

  pspecs[PROP_ACTIVE] =
    g_param_spec_boolean ("active", NULL, NULL,
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);


  signals[DROP] = g_signal_new ("drop",
                                G_TYPE_FROM_CLASS (klass),
                                G_SIGNAL_RUN_LAST,
                                0,
                                NULL, NULL,
                                kgx_marshals_VOID__STRING,
                                G_TYPE_NONE,
                                1, G_TYPE_STRING);
}


typedef struct {
  KgxDropTarget *self;
  GdkDrop       *drop;
  GStrvBuilder  *builder;
} ReadUrisData;


static void
read_uris_data_free (gpointer data)
{
  ReadUrisData *self = data;

  if (!self) {
    return;
  }

  g_clear_object (&self->self);
  g_clear_object (&self->drop);
  g_clear_pointer (&self->builder, g_strv_builder_unref);

  g_free (self);
}


G_DEFINE_AUTOPTR_CLEANUP_FUNC (ReadUrisData, read_uris_data_free)


static void
emit_list (KgxDropTarget *self, GStrvBuilder *builder)
{
  g_auto (GStrv) items = g_strv_builder_end (builder);
  g_autofree char *text = g_strjoinv (" ", items);

  g_signal_emit (self, signals[DROP], 0, text);
}


static void
add_file (GStrvBuilder *builder, GFile *file, const char *line)
{
  g_autofree char *path = g_file_get_path (file);
  g_autofree char *uri = g_file_get_uri (file);
  const char *raw = line ? line : uri;

  if (G_LIKELY (path)) {
    g_autofree char *quoted = g_shell_quote (path);

    g_debug ("drop-target: will drop %s as %s", raw, quoted);

    g_strv_builder_add (builder, quoted);
  } else {
    g_debug ("drop-target: will drop %s as-is", raw);

    g_strv_builder_add (builder, raw);
  }
}


static void
got_line (GObject      *source,
          GAsyncResult *res,
          gpointer      data)
{
  g_autoptr (GError) error = NULL;
  g_autoptr (GFile) file = NULL;
  g_autoptr (ReadUrisData) state = data;
  g_autofree char *line = NULL;

  line = g_data_input_stream_read_line_finish_utf8 (G_DATA_INPUT_STREAM (source),
                                                    res,
                                                    NULL,
                                                    &error);

  /* we've reached the end */
  if (G_UNLIKELY (!line)) {
    emit_list (state->self, state->builder);
    gdk_drop_finish (state->drop, GDK_ACTION_COPY);
    return;
  }

  /* url lists can have comments, apparently */
  if (G_LIKELY (!g_str_has_prefix (line, "#"))) {
    file = g_file_new_for_uri (line);

    add_file (state->builder, file, line);
  }

  g_data_input_stream_read_line_async (G_DATA_INPUT_STREAM (source),
                                       DROP_PRIORITY,
                                       NULL,
                                       got_line,
                                       g_steal_pointer (&state));
}


static void
got_uris (GObject      *source,
          GAsyncResult *res,
          gpointer      data)
{
  KgxDropTarget *self = data;
  g_autoptr (GError) error = NULL;
  g_autoptr (GInputStream) stream = NULL;
  g_autoptr (GDataInputStream) reader = NULL;
  g_autoptr (ReadUrisData) state = NULL;
  GdkDrop *drop = GDK_DROP (source);

  stream = gdk_drop_read_finish (drop, res, NULL, &error);

  if (error) {
    g_warning ("drop-target: failed to receive uri list: %s", error->message);
    gdk_drop_finish (drop, DROP_REJECT);
    return;
  }

  reader = g_data_input_stream_new (stream);
  g_data_input_stream_set_newline_type (reader,
                                        G_DATA_STREAM_NEWLINE_TYPE_CR_LF);

  state = g_new0 (ReadUrisData, 1);
  g_set_object (&state->self, self);
  g_set_object (&state->drop, drop);
  state->builder = g_strv_builder_new ();

  g_data_input_stream_read_line_async (reader,
                                       DROP_PRIORITY,
                                       NULL,
                                       got_line,
                                       g_steal_pointer (&state));
}


static void
handle_list (KgxDropTarget *self,
             GdkDrop       *drop,
             const GValue  *value)
{
  g_autoptr (GStrvBuilder) builder = g_strv_builder_new ();

  for (GSList *l = g_value_get_boxed (value); l; l = g_slist_next (l)) {
    add_file (builder, l->data, NULL);
  }

  emit_list (self, builder);
  if (G_LIKELY (drop)) {
    gdk_drop_finish (drop, GDK_ACTION_COPY);
  }
}


static void
get_uris (KgxDropTarget *self, GdkDrop *drop)
{
  gdk_drop_read_async (drop,
                       (const char *[]) { URIS, NULL },
                       DROP_PRIORITY,
                       NULL,
                       got_uris,
                       g_object_ref (self));
}


static void
got_files (GObject      *source,
           GAsyncResult *res,
           gpointer      data)
{
  KgxDropTarget *self = data;
  g_autoptr (GError) error = NULL;
  GdkDrop *drop = GDK_DROP (source);
  const GValue *value;

  value = gdk_drop_read_value_finish (drop, res, &error);

  if (error) {
    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND)) {
      /* This happens when gtk tried to send a directory via the portal */
      g_debug ("drop-target: assuming we are using a broken portal…");
      get_uris (self, drop);
    } else if (g_error_matches (error, G_DBUS_ERROR, G_DBUS_ERROR_ACCESS_DENIED)) {
      g_debug ("drop-target: assuming we are using a broken portal (of the other kind)…");
      get_uris (self, drop);
    } else {
      g_warning ("drop-target: failed to receive file list: %s", error->message);
      gdk_drop_finish (drop, DROP_REJECT);
    }

    return;
  }

  handle_list (self, drop, value);
}


static void
got_text (GObject      *source,
          GAsyncResult *res,
          gpointer      data)
{
  g_autoptr (KgxDropTarget) self = data;
  g_autoptr (GError) error = NULL;
  g_autoptr (GStrvBuilder) builder = NULL;
  GdkDrop *drop = GDK_DROP (source);
  const GValue *value;

  value = gdk_drop_read_value_finish (drop, res, &error);

  if (error) {
    g_warning ("drop-target: failed to receive text: %s", error->message);
    gdk_drop_finish (drop, DROP_REJECT);
    return;
  }

  g_signal_emit (self, signals[DROP], 0, g_value_get_string (value));
  gdk_drop_finish (drop, GDK_ACTION_COPY);
}


static gboolean
drop (GtkDropTargetAsync *target,
      GdkDrop            *drop,
      double              x,
      double              y,
      gpointer            user_data)
{
  KgxDropTarget *self = user_data;
  GdkContentFormats *formats = gdk_drop_get_formats (drop);
  const char *const *mimes = NULL;

  mimes = gdk_content_formats_get_mime_types (formats, NULL);

  if (G_LIKELY (g_strv_contains (mimes, PORTAL)) || g_strv_contains (mimes, PORTAL_OLD)) {
    /* this is the standard case where a file list was dropped from new apps */
    gdk_drop_read_value_async (drop,
                               GDK_TYPE_FILE_LIST,
                               DROP_PRIORITY,
                               NULL,
                               got_files,
                               user_data);
    return TRUE;
  } else if (g_strv_contains (mimes, URIS)) {
    /* a file drop from an older application, or a regular uri list */
    get_uris (self, drop);
    return TRUE;
  } else if (G_LIKELY (g_strv_contains (mimes, TEXT))) {
    gdk_drop_read_value_async (drop,
                               G_TYPE_STRING,
                               DROP_PRIORITY,
                               NULL,
                               got_text,
                               user_data);
    return TRUE;
  } else {
    return FALSE;
  }
}


static GdkDragAction
enter (GtkDropTargetAsync *target,
       GdkDrop            *drop,
       double              x,
       double              y,
       gpointer            user_data)
{
  KgxDropTarget *self = user_data;

  self->active = TRUE;
  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_ACTIVE]);

  return GDK_ACTION_COPY;
}


static void
leave (GtkDropTargetAsync *target,
       GdkDrop            *drop,
       gpointer            user_data)
{
  KgxDropTarget *self = user_data;

  self->active = FALSE;
  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_ACTIVE]);
}


static void
kgx_drop_target_init (KgxDropTarget *self)
{
  static const char *types[] = { PORTAL, PORTAL_OLD, URIS, TEXT };
  GdkContentFormats *formats = gdk_content_formats_new ((const char **) types,
                                                        G_N_ELEMENTS (types));
  self->target = gtk_drop_target_async_new (formats, GDK_ACTION_COPY);

  g_signal_connect_object (self->target,
                           "drop", G_CALLBACK (drop), self,
                           G_CONNECT_DEFAULT);
  g_signal_connect_object (self->target,
                           "drag-enter", G_CALLBACK (enter), self,
                           G_CONNECT_DEFAULT);
  g_signal_connect_object (self->target,
                           "drag-leave", G_CALLBACK (leave), self,
                           G_CONNECT_DEFAULT);
}


void
kgx_drop_target_extra_drop (KgxDropTarget *self,
                            const GValue  *value)
{
  g_return_if_fail (KGX_IS_DROP_TARGET (self));

  if (G_VALUE_HOLDS (value, GDK_TYPE_FILE_LIST)) {
    handle_list (self, NULL, value);
  } else if (G_VALUE_HOLDS_STRING (value)) {
    g_signal_emit (self, signals[DROP], 0, g_value_get_string (value));
  } else {
    g_critical ("drop-target: unexpected value: %s",
                G_VALUE_TYPE_NAME (value));
  }
}


void
kgx_drop_target_mount_on (KgxDropTarget *self,
                          GtkWidget     *widget)
{
  g_return_if_fail (KGX_IS_DROP_TARGET (self));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  gtk_widget_add_controller (widget,
                             g_object_ref (GTK_EVENT_CONTROLLER (self->target)));
}
