/* kgx-fullscreen-box.c
 *
 * Copyright 2025 Zander Brown
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
 *
 * Based on ephy-fullscreen-box.c:
 *      Copyright 2021 Purism SPC
 */

#include "kgx-config.h"

#include <adwaita.h>

#include "kgx-utils.h"

#include "kgx-fullscreen-box.h"


#define FULLSCREEN_HIDE_DELAY 300
#define SHOW_HEADERBAR_DISTANCE_PX 5


struct _KgxFullscreenBox {
  AdwBin          parent_instance;

  AdwToolbarView *toolbar_view;

  gboolean        fullscreen;
  gboolean        autohide;
  GtkWidget      *content;
  GtkWidget      *last_focus;

  guint           timeout_id;
  double          last_y;
  gboolean        is_touch;
  GList          *headers;
};


static void kgx_fullscreen_box_buildable_init (GtkBuildableIface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (KgxFullscreenBox, kgx_fullscreen_box, ADW_TYPE_BIN,
                               G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE,
                                                      kgx_fullscreen_box_buildable_init))


static GtkBuildableIface *parent_buildable_iface;


enum {
  PROP_0,
  PROP_FULLSCREEN,
  PROP_AUTOHIDE,
  PROP_CONTENT,
  PROP_LAST_FOCUS,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP];


static void
kgx_fullscreen_box_dispose (GObject *object)
{
  KgxFullscreenBox *self = KGX_FULLSCREEN_BOX (object);

  g_clear_weak_pointer (&self->last_focus);
  g_clear_weak_pointer (&self->content);

  g_clear_handle_id (&self->timeout_id, g_source_remove);
  g_clear_pointer (&self->headers, g_list_free);

  G_OBJECT_CLASS (kgx_fullscreen_box_parent_class)->dispose (object);
}


static inline double
get_titlebar_area_height (KgxFullscreenBox *self)
{
  double height = adw_toolbar_view_get_top_bar_height (self->toolbar_view);

  return MAX (height, SHOW_HEADERBAR_DISTANCE_PX);
}


static inline gboolean
is_descendant_of (GtkWidget *widget,
                  GtkWidget *target)
{
  GtkWidget *parent;

  if (!widget) {
    return FALSE;
  }

  if (widget == target) {
    return TRUE;
  }

  parent = widget;

  while (parent && parent != target) {
    parent = gtk_widget_get_parent (parent);
  }

  return parent == target;
}


static inline gboolean
is_focusing_titlebar (KgxFullscreenBox *self)
{
  for (GList *l = self->headers; l; l = l->next) {
    if (is_descendant_of (self->last_focus, l->data)) {
      return TRUE;
    }
  }

  return FALSE;
}


static void
show_ui (KgxFullscreenBox *self)
{
  g_clear_handle_id (&self->timeout_id, g_source_remove);

  adw_toolbar_view_set_reveal_top_bars (self->toolbar_view, TRUE);
  adw_toolbar_view_set_reveal_bottom_bars (self->toolbar_view, TRUE);
}


static void
hide_ui (KgxFullscreenBox *self)
{
  g_clear_handle_id (&self->timeout_id, g_source_remove);

  if (!self->fullscreen) {
    return;
  }

  adw_toolbar_view_set_reveal_top_bars (self->toolbar_view, FALSE);
  adw_toolbar_view_set_reveal_bottom_bars (self->toolbar_view, FALSE);

  gtk_widget_grab_focus (GTK_WIDGET (self->content));
}


static void
hide_timeout (gpointer user_data)
{
  KgxFullscreenBox *self = user_data;

  self->timeout_id = 0;

  hide_ui (self);
}


static inline void
start_hide_timeout (KgxFullscreenBox *self)
{
  if (!adw_toolbar_view_get_reveal_top_bars (self->toolbar_view)) {
    return;
  }

  if (self->timeout_id) {
    return;
  }

  self->timeout_id = g_timeout_add_once (FULLSCREEN_HIDE_DELAY,
                                         hide_timeout,
                                         self);
}


static void
update (KgxFullscreenBox *self,
        gboolean          hide_immediately)
{
  if (!self->autohide || !self->fullscreen) {
    return;
  }

  if (!self->is_touch && self->last_y <= get_titlebar_area_height (self)) {
    show_ui (self);
    return;
  }

  if (self->last_focus && is_focusing_titlebar (self)) {
    show_ui (self);
  } else if (hide_immediately) {
    hide_ui (self);
  } else {
    start_hide_timeout (self);
  }
}


static void
kgx_fullscreen_box_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  KgxFullscreenBox *self = KGX_FULLSCREEN_BOX (object);

  switch (prop_id) {
    case PROP_FULLSCREEN:
      if (kgx_set_boolean_prop (object, pspec, &self->fullscreen, value) &&
          self->autohide) {
        adw_toolbar_view_set_extend_content_to_top_edge (self->toolbar_view,
                                                         self->fullscreen);

        if (self->fullscreen) {
          update (self, FALSE);
        } else {
          show_ui (self);
        }
      }
      break;
    case PROP_AUTOHIDE:
      if (kgx_set_boolean_prop (object, pspec, &self->autohide, value) &&
          self->fullscreen) {
        if (self->autohide) {
          hide_ui (self);
        } else {
          show_ui (self);
        }
      }
      break;
    case PROP_CONTENT:
      if (g_set_weak_pointer (&self->content, g_value_get_object (value))) {
        g_object_notify_by_pspec (object, pspec);
      }
      break;
    case PROP_LAST_FOCUS:
      if (g_set_weak_pointer (&self->last_focus,
                              g_value_get_object (value))) {
        update (self, TRUE);
        g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_LAST_FOCUS]);
      }
      break;
    KGX_INVALID_PROP (object, prop_id, pspec);
  }
}


static void
kgx_fullscreen_box_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  KgxFullscreenBox *self = KGX_FULLSCREEN_BOX (object);

  switch (prop_id) {
    case PROP_FULLSCREEN:
      g_value_set_boolean (value, self->fullscreen);
      break;
    case PROP_AUTOHIDE:
      g_value_set_boolean (value, self->autohide);
      break;
    case PROP_CONTENT:
      g_value_set_object (value, self->content);
      break;
    case PROP_LAST_FOCUS:
      g_value_set_object (value, self->last_focus);
      break;
    KGX_INVALID_PROP (object, prop_id, pspec);
  }
}


static void
motion (KgxFullscreenBox *self,
        double            x,
        double            y)
{
  self->is_touch = FALSE;
  self->last_y = y;

  update (self, TRUE);
}


static void
enter (KgxFullscreenBox *self,
       double            x,
       double            y)
{
  motion (self, x, y);
}


static void
pressed (KgxFullscreenBox *self,
         int               n_press,
         double            x,
         double            y,
         GtkGesture       *gesture)
{
  gtk_gesture_set_state (gesture, GTK_EVENT_SEQUENCE_DENIED);

  self->is_touch = TRUE;

  if (y > get_titlebar_area_height (self)) {
    update (self, TRUE);
  }
}


static void
kgx_fullscreen_box_class_init (KgxFullscreenBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = kgx_fullscreen_box_dispose;
  object_class->set_property = kgx_fullscreen_box_set_property;
  object_class->get_property = kgx_fullscreen_box_get_property;

  pspecs[PROP_FULLSCREEN] =
    g_param_spec_boolean ("fullscreen", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_AUTOHIDE] =
    g_param_spec_boolean ("autohide", NULL, NULL,
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_CONTENT] =
    g_param_spec_object ("content", NULL, NULL,
                         GTK_TYPE_WIDGET,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_LAST_FOCUS] =
    g_param_spec_object ("last-focus", NULL, NULL,
                         GTK_TYPE_WIDGET,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               KGX_APPLICATION_PATH "kgx-fullscreen-box.ui");

  gtk_widget_class_bind_template_child (widget_class, KgxFullscreenBox, toolbar_view);

  gtk_widget_class_bind_template_callback (widget_class, motion);
  gtk_widget_class_bind_template_callback (widget_class, enter);
  gtk_widget_class_bind_template_callback (widget_class, pressed);

  gtk_widget_class_set_css_name (widget_class, "fullscreenbox");
}


static void
kgx_fullscreen_box_init (KgxFullscreenBox *self)
{
  self->autohide = TRUE;

  gtk_widget_init_template (GTK_WIDGET (self));
}


static void
kgx_fullscreen_box_buildable_add_child (GtkBuildable *buildable,
                                        GtkBuilder   *builder,
                                        GObject      *child,
                                        const char   *type)
{
  KgxFullscreenBox *self = KGX_FULLSCREEN_BOX (buildable);

  if (type && g_str_equal (type, "top")) {
    adw_toolbar_view_add_top_bar (self->toolbar_view, GTK_WIDGET (child));

    self->headers = g_list_prepend (self->headers, child);
  } else {
    parent_buildable_iface->add_child (buildable, builder, child, type);
  }
}


static void
kgx_fullscreen_box_buildable_init (GtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = kgx_fullscreen_box_buildable_add_child;
}
