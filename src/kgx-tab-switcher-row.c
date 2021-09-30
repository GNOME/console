/* kgx-tab-switcher-row.c
 *
 * Copyright 2021 Purism SPC
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
#include "kgx-tab-switcher-row.h"


struct _KgxTabSwitcherRow {
  GtkListBoxRow parent_instance;

  GtkRevealer *revealer;
  GtkStack *icon_stack;
  GtkImage *icon;
  GtkSpinner *spinner;
  GtkLabel *title;
  GtkWidget *indicator_btn;
  GtkImage *indicator_icon;
  GtkWidget *close_btn;

  HdyTabPage *page;
  HdyTabView *view;
};


G_DEFINE_TYPE (KgxTabSwitcherRow, kgx_tab_switcher_row, GTK_TYPE_LIST_BOX_ROW)


enum {
  PROP_0,
  PROP_PAGE,
  PROP_VIEW,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };


static inline void
set_style_class (GtkWidget  *widget,
                 const char *style_class,
                 gboolean    enabled)
{
  GtkStyleContext *context = gtk_widget_get_style_context (widget);

  if (enabled) {
    gtk_style_context_add_class (context, style_class);
  } else {
    gtk_style_context_remove_class (context, style_class);
  }
}


static void
update_pinned (KgxTabSwitcherRow *self)
{
  set_style_class (GTK_WIDGET (self), "pinned",
                   hdy_tab_page_get_pinned (self->page));
}


static void
update_icon (KgxTabSwitcherRow *self)
{
  GIcon *gicon = hdy_tab_page_get_icon (self->page);
  gboolean loading = hdy_tab_page_get_loading (self->page);
  const char *name = loading ? "spinner" : "icon";

  if (!gicon) {
    gicon = hdy_tab_view_get_default_icon (self->view);
  }

  gtk_image_set_from_gicon (self->icon, gicon, GTK_ICON_SIZE_BUTTON);
  gtk_stack_set_visible_child_name (self->icon_stack, name);
}


static void
update_spinner (KgxTabSwitcherRow *self)
{
  gboolean loading = self->page && hdy_tab_page_get_loading (self->page);
  gboolean mapped = gtk_widget_get_mapped (GTK_WIDGET (self));

  /* Don't use CPU when not needed */
  if (loading && mapped) {
    gtk_spinner_start (self->spinner);
  } else if (gtk_widget_get_realized (GTK_WIDGET (self))) {
    gtk_spinner_stop (self->spinner);
  }
}


static void
update_loading (KgxTabSwitcherRow *self)
{
  update_icon (self);
  update_spinner (self);
  set_style_class (GTK_WIDGET (self), "loading",
                   hdy_tab_page_get_loading (self->page));
}


static void
update_indicator (KgxTabSwitcherRow *self)
{
  GIcon *indicator = hdy_tab_page_get_indicator_icon (self->page);
  gboolean activatable = hdy_tab_page_get_indicator_activatable (self->page);

  gtk_image_set_from_gicon (self->indicator_icon, indicator, GTK_ICON_SIZE_BUTTON);
  gtk_widget_set_visible (GTK_WIDGET (self->indicator_btn), indicator != NULL);
  gtk_widget_set_sensitive (GTK_WIDGET (self->indicator_btn), activatable);
}


static void
update_needs_attention (KgxTabSwitcherRow *self)
{
  set_style_class (GTK_WIDGET (self), "needs-attention",
                   hdy_tab_page_get_needs_attention (self->page));
}


static void
indicator_clicked_cb (KgxTabSwitcherRow *self)
{
  if (!self->page) {
    return;
  }

  g_signal_emit_by_name (self->view, "indicator-activated", self->page);
}


static void
close_clicked_cb (KgxTabSwitcherRow *self)
{
  if (!self->page) {
    return;
  }

  hdy_tab_view_close_page (self->view, self->page);
}


static void
kgx_tab_switcher_row_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  KgxTabSwitcherRow *self = KGX_TAB_SWITCHER_ROW (object);

  switch (prop_id) {
    case PROP_PAGE:
      g_value_set_object (value, self->page);
      break;
    case PROP_VIEW:
      g_value_set_object (value, self->view);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}


static void
kgx_tab_switcher_row_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  KgxTabSwitcherRow *self = KGX_TAB_SWITCHER_ROW (object);

  switch (prop_id) {
    case PROP_PAGE:
      self->page = g_value_get_object (value);
      break;
    case PROP_VIEW:
      self->view = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}


static void
kgx_tab_switcher_row_constructed (GObject *object)
{
  KgxTabSwitcherRow *self = KGX_TAB_SWITCHER_ROW (object);

  G_OBJECT_CLASS (kgx_tab_switcher_row_parent_class)->constructed (object);

  g_object_bind_property (self->page, "title",
                          self->title, "label",
                          G_BINDING_SYNC_CREATE);
  g_object_bind_property (self->page, "pinned",
                          self->close_btn, "visible",
                          G_BINDING_SYNC_CREATE | G_BINDING_INVERT_BOOLEAN);

  g_signal_connect_object (self->page, "notify::pinned",
                           G_CALLBACK (update_pinned), self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (self->page, "notify::icon",
                           G_CALLBACK (update_icon), self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (self->page, "notify::loading",
                           G_CALLBACK (update_loading), self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (self->page, "notify::indicator-icon",
                           G_CALLBACK (update_indicator), self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (self->page, "notify::indicator-activatable",
                           G_CALLBACK (update_indicator), self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (self->page, "notify::needs-attention",
                           G_CALLBACK (update_needs_attention), self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (self->view, "notify::default-icon",
                           G_CALLBACK (update_icon), self,
                           G_CONNECT_SWAPPED);

  update_pinned (self);
  update_loading (self);
  update_indicator (self);
  update_needs_attention (self);
}


static void
kgx_tab_switcher_row_map (GtkWidget *widget)
{
  KgxTabSwitcherRow *self = KGX_TAB_SWITCHER_ROW (widget);

  GTK_WIDGET_CLASS (kgx_tab_switcher_row_parent_class)->map (widget);

  update_spinner (self);
}


static void
kgx_tab_switcher_row_unmap (GtkWidget *widget)
{
  KgxTabSwitcherRow *self = KGX_TAB_SWITCHER_ROW (widget);

  GTK_WIDGET_CLASS (kgx_tab_switcher_row_parent_class)->unmap (widget);

  update_spinner (self);
}


static void
kgx_tab_switcher_row_class_init (KgxTabSwitcherRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = kgx_tab_switcher_row_get_property;
  object_class->set_property = kgx_tab_switcher_row_set_property;
  object_class->constructed = kgx_tab_switcher_row_constructed;

  widget_class->map = kgx_tab_switcher_row_map;
  widget_class->unmap = kgx_tab_switcher_row_unmap;

  pspecs[PROP_PAGE] =
    g_param_spec_object ("page",
                         "Page",
                         "The page the row displays.",
                         HDY_TYPE_TAB_PAGE,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  pspecs[PROP_VIEW] =
    g_param_spec_object ("view",
                         "View",
                         "The view containing the page.",
                         HDY_TYPE_TAB_VIEW,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               KGX_APPLICATION_PATH "kgx-tab-switcher-row.ui");

  gtk_widget_class_bind_template_child (widget_class, KgxTabSwitcherRow, revealer);
  gtk_widget_class_bind_template_child (widget_class, KgxTabSwitcherRow, icon_stack);
  gtk_widget_class_bind_template_child (widget_class, KgxTabSwitcherRow, icon);
  gtk_widget_class_bind_template_child (widget_class, KgxTabSwitcherRow, spinner);
  gtk_widget_class_bind_template_child (widget_class, KgxTabSwitcherRow, title);
  gtk_widget_class_bind_template_child (widget_class, KgxTabSwitcherRow, indicator_btn);
  gtk_widget_class_bind_template_child (widget_class, KgxTabSwitcherRow, indicator_icon);
  gtk_widget_class_bind_template_child (widget_class, KgxTabSwitcherRow, close_btn);

  gtk_widget_class_bind_template_callback (widget_class, indicator_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, close_clicked_cb);
}


static void
kgx_tab_switcher_row_init (KgxTabSwitcherRow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


GtkWidget *
kgx_tab_switcher_row_new (HdyTabPage *page,
                          HdyTabView *view)
{
  g_return_val_if_fail (HDY_IS_TAB_PAGE (page), NULL);
  g_return_val_if_fail (HDY_IS_TAB_VIEW (view), NULL);

  return g_object_new (KGX_TYPE_TAB_SWITCHER_ROW,
                       "page", page,
                       "view", view,
                       NULL);
}


HdyTabPage *
kgx_tab_switcher_row_get_page (KgxTabSwitcherRow *self)
{
  g_return_val_if_fail (KGX_IS_TAB_SWITCHER_ROW (self), NULL);

  return self->page;
}


void
kgx_tab_switcher_row_animate_close (KgxTabSwitcherRow *self)
{
  g_return_if_fail (KGX_IS_TAB_SWITCHER_ROW (self));

  if (!self->page) {
    return;
  }

  g_signal_handlers_disconnect_by_func (self->view, G_CALLBACK (update_icon), self);

  g_signal_handlers_disconnect_by_func (self->page, G_CALLBACK (update_pinned), self);
  g_signal_handlers_disconnect_by_func (self->page, G_CALLBACK (update_icon), self);
  g_signal_handlers_disconnect_by_func (self->page, G_CALLBACK (update_loading), self);
  g_signal_handlers_disconnect_by_func (self->page, G_CALLBACK (update_indicator), self);
  g_signal_handlers_disconnect_by_func (self->page, G_CALLBACK (update_needs_attention), self);

  self->page = NULL;

  g_signal_connect_swapped (self->revealer, "notify::child-revealed",
                            G_CALLBACK (gtk_widget_destroy), self);

  gtk_revealer_set_reveal_child (self->revealer, FALSE);
}


void
kgx_tab_switcher_row_animate_open (KgxTabSwitcherRow *self)
{
  g_return_if_fail (KGX_IS_TAB_SWITCHER_ROW (self));

  if (!self->page) {
    return;
  }

  gtk_widget_show (GTK_WIDGET (self));
  gtk_revealer_set_reveal_child (self->revealer, TRUE);
}


gboolean
kgx_tab_switcher_row_is_animating (KgxTabSwitcherRow *self)
{
  g_return_val_if_fail (KGX_IS_TAB_SWITCHER_ROW (self), FALSE);

  return self->page == NULL;
}
