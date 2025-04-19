/* kgx-empty.c
 *
 * Copyright 2024-2025 Zander Brown
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
#include <adwaita.h>

#include "kgx-utils.h"

#include "kgx-empty.h"


struct _KgxEmpty {
  GtkBox       parent_instance;

  gboolean     working;

  gboolean     logo_visible;
  guint        logo_timeout;

  gboolean     spinner_visible;
  guint        spinner_timeout;

  gboolean     had_first_map;
};


G_DEFINE_FINAL_TYPE (KgxEmpty, kgx_empty, GTK_TYPE_BOX)


enum {
  PROP_0,
  PROP_WORKING,
  PROP_LOGO_VISIBLE,
  PROP_SPINNER_VISIBLE,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };


static void
kgx_empty_dispose (GObject *object)
{
  KgxEmpty *self = KGX_EMPTY (object);

  g_clear_handle_id (&self->logo_timeout, g_source_remove);
  g_clear_handle_id (&self->spinner_timeout, g_source_remove);

  G_OBJECT_CLASS (kgx_empty_parent_class)->dispose (object);
}


static void
kgx_empty_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  KgxEmpty *self = KGX_EMPTY (object);

  switch (property_id) {
    case PROP_WORKING:
      g_value_set_boolean (value, self->working);
      break;
    case PROP_LOGO_VISIBLE:
      g_value_set_boolean (value, self->logo_visible);
      break;
    case PROP_SPINNER_VISIBLE:
      g_value_set_boolean (value, self->spinner_visible);
      break;
    KGX_INVALID_PROP (object, property_id, pspec);
  }
}


static void
show_spinner (gpointer user_data)
{
  KgxEmpty *self = KGX_EMPTY (user_data);

  self->spinner_visible = TRUE;
  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_SPINNER_VISIBLE]);

  self->spinner_timeout = 0;
}


static inline void
update_spinner (KgxEmpty *self)
{
  g_clear_handle_id (&self->spinner_timeout, g_source_remove);

  if (self->working) {
    self->spinner_timeout = g_timeout_add_once (100, show_spinner, self);
  } else {
    self->spinner_visible = FALSE;
    g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_SPINNER_VISIBLE]);
  }
}


static void
kgx_empty_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  KgxEmpty *self = KGX_EMPTY (object);

  switch (property_id) {
    case PROP_WORKING:
      if (kgx_set_boolean_prop (object, pspec, &self->working, value)) {
        update_spinner (self);
      }
      break;
    KGX_INVALID_PROP (object, property_id, pspec);
  }
}


static void
show_logo (gpointer user_data)
{
  KgxEmpty *self = KGX_EMPTY (user_data);

  self->logo_visible = TRUE;
  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_LOGO_VISIBLE]);

  self->logo_timeout = 0;
}


static void
kgx_empty_map (GtkWidget *widget)
{
  KgxEmpty *self = KGX_EMPTY (widget);

  if (!self->had_first_map) {
    self->had_first_map = TRUE;
    self->logo_timeout = g_timeout_add_once (80, show_logo, self);
  }

  GTK_WIDGET_CLASS (kgx_empty_parent_class)->map (widget);
}


static void
kgx_empty_unmap (GtkWidget *widget)
{
  KgxEmpty *self = KGX_EMPTY (widget);

  g_clear_handle_id (&self->logo_timeout, g_source_remove);
  g_clear_handle_id (&self->spinner_timeout, g_source_remove);

  GTK_WIDGET_CLASS (kgx_empty_parent_class)->unmap (widget);
}


static void
kgx_empty_class_init (KgxEmptyClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = kgx_empty_dispose;
  object_class->get_property = kgx_empty_get_property;
  object_class->set_property = kgx_empty_set_property;

  widget_class->map = kgx_empty_map;
  widget_class->unmap = kgx_empty_unmap;

  pspecs[PROP_WORKING] =
    g_param_spec_boolean ("working", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_LOGO_VISIBLE] =
    g_param_spec_boolean ("logo-visible", NULL, NULL,
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_SPINNER_VISIBLE] =
    g_param_spec_boolean ("spinner-visible", NULL, NULL,
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);

  g_type_ensure (ADW_TYPE_SPINNER);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               KGX_APPLICATION_PATH "kgx-empty.ui");

  gtk_widget_class_set_css_name (widget_class, "kgx-empty");
}


static void
kgx_empty_init (KgxEmpty *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
