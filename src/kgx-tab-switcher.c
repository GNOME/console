/* kgx-tab-switcher.c
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

/**
 * SECTION:kgx-tab-switcher
 * @short_description: A mobile tab switcher for HdyTabView
 * @title: KgxTabSwitcher
 *
 * The #KgxTabSwitcher widget is a mobile tab switcher for HdyTabView. It's
 * supposed to be used in conjunction with #KgxTabButton to open it, and a
 * #HdyTabBar to provide UI for larger screens.
 */

#include "kgx-config.h"
#include "kgx-tab-switcher.h"
#include "kgx-tab-switcher-row.h"


struct _KgxTabSwitcher {
  GtkBin parent_instance;

  HdyFlap *flap;
  GtkListBox *list;

  GtkGesture *click_gesture;
  GtkGesture *long_press_gesture;
  GtkMenu *context_menu;
  GtkPopover *touch_menu;

  HdyTabView *view;
  gboolean narrow;
};


G_DEFINE_TYPE (KgxTabSwitcher, kgx_tab_switcher, GTK_TYPE_BIN)


enum {
  PROP_0,
  PROP_VIEW,
  PROP_NARROW,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };

enum {
  NEW_TAB,
  N_SIGNALS,
};
static guint signals[N_SIGNALS];


static gboolean
reset_setup_menu_cb (KgxTabSwitcher *self)
{
  g_signal_emit_by_name (self->view, "setup-menu", NULL);

  return G_SOURCE_REMOVE;
}


static void
touch_menu_notify_visible_cb (KgxTabSwitcher *self)
{
  if (!self->touch_menu || gtk_widget_get_visible (GTK_WIDGET (self->touch_menu))) {
    return;
  }

  g_idle_add (G_SOURCE_FUNC (reset_setup_menu_cb), self);
}


static void
destroy_cb (KgxTabSwitcher *self)
{
  self->touch_menu = NULL;
}


static void
do_touch_popup (KgxTabSwitcher    *self,
                KgxTabSwitcherRow *row)
{
  GMenuModel *model = hdy_tab_view_get_menu_model (self->view);
  HdyTabPage *page = kgx_tab_switcher_row_get_page (row);

  if (!G_IS_MENU_MODEL (model)) {
    return;
  }

  g_signal_emit_by_name (self->view, "setup-menu", page);

  if (!self->touch_menu) {
    self->touch_menu = GTK_POPOVER (gtk_popover_new_from_model (GTK_WIDGET (row), model));
    gtk_popover_set_constrain_to (self->touch_menu, GTK_POPOVER_CONSTRAINT_WINDOW);

    g_signal_connect_object (self->touch_menu, "notify::visible",
                             G_CALLBACK (touch_menu_notify_visible_cb), self,
                             G_CONNECT_AFTER | G_CONNECT_SWAPPED);

    g_signal_connect_object (self->touch_menu, "destroy",
                             G_CALLBACK (destroy_cb), self,
                             G_CONNECT_AFTER | G_CONNECT_SWAPPED);
  } else {
    gtk_popover_set_relative_to (self->touch_menu, GTK_WIDGET (row));
  }

  gtk_popover_popup (self->touch_menu);
}


static void
popup_menu_detach (KgxTabSwitcher *self,
                   GtkMenu        *menu)
{
  self->context_menu = NULL;
}


static void
popup_menu_deactivate_cb (KgxTabSwitcher *self)
{
  g_idle_add (G_SOURCE_FUNC (reset_setup_menu_cb), self);
}


static void
do_popup (KgxTabSwitcher    *self,
          KgxTabSwitcherRow *row,
          GdkEvent          *event)
{
  GMenuModel *model = hdy_tab_view_get_menu_model (self->view);
  HdyTabPage *page = kgx_tab_switcher_row_get_page (row);

  if (!G_IS_MENU_MODEL (model)) {
    return;
  }

  g_signal_emit_by_name (self->view, "setup-menu", page);

  if (!self->context_menu) {
    self->context_menu = GTK_MENU (gtk_menu_new_from_model (model));
    gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (self->context_menu)),
                                 GTK_STYLE_CLASS_CONTEXT_MENU);

    g_signal_connect_object (self->context_menu,
                             "deactivate",
                             G_CALLBACK (popup_menu_deactivate_cb),
                             self,
                             G_CONNECT_SWAPPED);

    gtk_menu_attach_to_widget (self->context_menu, GTK_WIDGET (self),
                               (GtkMenuDetachFunc) popup_menu_detach);
  }

  if (event && gdk_event_triggers_context_menu (event)) {
    gtk_menu_popup_at_pointer (self->context_menu, event);
  } else {
    gtk_menu_popup_at_widget (self->context_menu,
                              GTK_WIDGET (row),
                              GDK_GRAVITY_SOUTH_WEST,
                              GDK_GRAVITY_NORTH_WEST,
                              event);

    gtk_menu_shell_select_first (GTK_MENU_SHELL (self->context_menu), FALSE);
  }
}


static void
reveal_flap_cb (KgxTabSwitcher *self)
{
  HdyTabPage *page;
  GtkWidget *child;

  gtk_widget_set_sensitive (GTK_WIDGET (self->view),
                            !hdy_flap_get_reveal_flap (self->flap));

  if (hdy_flap_get_reveal_flap (self->flap)) {
    gtk_widget_grab_focus (GTK_WIDGET (self->list));
  } else {
    page = hdy_tab_view_get_selected_page (self->view);
    child = hdy_tab_page_get_child (page);

    gtk_widget_grab_focus (GTK_WIDGET (child));
  }
}


static void
collapse_cb (KgxTabSwitcher *self)
{
  kgx_tab_switcher_close (self);
}


static void
new_tab_cb (KgxTabSwitcher *self)
{
  g_signal_emit (self, signals[NEW_TAB], 0);

  kgx_tab_switcher_close (self);
}


static void
row_selected_cb (KgxTabSwitcher    *self,
                 KgxTabSwitcherRow *row)
{
  HdyTabPage *page;

  if (!row) {
    return;
  }

  g_assert (KGX_IS_TAB_SWITCHER_ROW (row));

  if (!self->view) {
    return;
  }

  page = kgx_tab_switcher_row_get_page (row);
  hdy_tab_view_set_selected_page (self->view, page);
}


static void
row_activated_cb (KgxTabSwitcher *self)
{
  kgx_tab_switcher_close (self);
}


static KgxTabSwitcherRow *
find_nth_alive_row (KgxTabSwitcher *self,
                    guint           position)
{
  GtkListBoxRow *row = NULL;
  guint i = 0;

  do {
    row = gtk_list_box_get_row_at_index (self->list, i++);

    if (kgx_tab_switcher_row_is_animating (KGX_TAB_SWITCHER_ROW (row)))
      position++;
  } while (i <= position);

  return KGX_TAB_SWITCHER_ROW (row);
}


static void
pages_changed_cb (KgxTabSwitcher *self,
                  guint           position,
                  guint           removed,
                  guint           added,
                  GListModel     *pages)
{
  guint i;

  while (removed--) {
    KgxTabSwitcherRow *row = find_nth_alive_row (self, position);

    kgx_tab_switcher_row_animate_close (KGX_TAB_SWITCHER_ROW (row));
  }

  for (i = 0; i < added; i++) {
    g_autoptr (HdyTabPage) page = g_list_model_get_item (pages, position + i);
    GtkWidget *row = kgx_tab_switcher_row_new (page, self->view);

    gtk_list_box_insert (self->list, row, position + i);
    kgx_tab_switcher_row_animate_open (KGX_TAB_SWITCHER_ROW (row));
  }
}


static void
notify_selected_page_cb (KgxTabSwitcher *self)
{
  HdyTabPage *page = NULL;

  if (self->view) {
    page = hdy_tab_view_get_selected_page (self->view);
  }

  if (page) {
    int index = hdy_tab_view_get_page_position (self->view, page);
    KgxTabSwitcherRow *row = find_nth_alive_row (self, index);

    gtk_list_box_select_row (self->list, GTK_LIST_BOX_ROW (row));
  } else {
    gtk_list_box_unselect_all (self->list);
  }
}


static void
click_pressed_cb (KgxTabSwitcher *self,
                  int             n_press,
                  double          x,
                  double          y)
{
  g_autoptr (GdkEvent) event = NULL;
  GtkListBoxRow *row;
  HdyTabPage *page;
  guint button;

  if (n_press > 1) {
    gtk_gesture_set_state (self->click_gesture, GTK_EVENT_SEQUENCE_DENIED);

    return;
  }

  row = gtk_list_box_get_row_at_y (self->list, y);

  if (!row) {
    gtk_gesture_set_state (self->click_gesture, GTK_EVENT_SEQUENCE_DENIED);

    return;
  }

  event = gtk_get_current_event ();
  button = gtk_gesture_single_get_current_button (GTK_GESTURE_SINGLE (self->click_gesture));
  page = kgx_tab_switcher_row_get_page (KGX_TAB_SWITCHER_ROW (row));

  if (event && gdk_event_triggers_context_menu (event)) {
    gtk_gesture_set_state (self->click_gesture, GTK_EVENT_SEQUENCE_CLAIMED);
    do_popup (self, KGX_TAB_SWITCHER_ROW (row), event);

    return;
  }

  if (button == GDK_BUTTON_MIDDLE) {
    gtk_gesture_set_state (self->click_gesture, GTK_EVENT_SEQUENCE_CLAIMED);
    hdy_tab_view_close_page (self->view, page);

    return;
  }

  gtk_gesture_set_state (self->click_gesture, GTK_EVENT_SEQUENCE_DENIED);
}


static void
long_press_cb (KgxTabSwitcher *self,
               double          x,
               double          y)
{
  GtkListBoxRow *row = gtk_list_box_get_row_at_y (self->list, y);

  if (!row) {
    gtk_gesture_set_state (self->click_gesture, GTK_EVENT_SEQUENCE_DENIED);

    return;
  }

  do_touch_popup (self, KGX_TAB_SWITCHER_ROW (row));

  gtk_gesture_set_state (self->long_press_gesture, GTK_EVENT_SEQUENCE_CLAIMED);
}


static void
set_narrow (KgxTabSwitcher *self,
            gboolean        narrow)
{
  if (self->narrow == narrow) {
    return;
  }

  self->narrow = narrow;

  if (!narrow && self->flap) {
    kgx_tab_switcher_close (self);
  }

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_NARROW]);
}


static void
kgx_tab_switcher_dispose (GObject *object)
{
  KgxTabSwitcher *self = KGX_TAB_SWITCHER (object);

  kgx_tab_switcher_set_view (self, NULL);

  g_clear_object (&self->click_gesture);
  g_clear_object (&self->long_press_gesture);

  G_OBJECT_CLASS (kgx_tab_switcher_parent_class)->dispose (object);
}


static void
kgx_tab_switcher_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  KgxTabSwitcher *self = KGX_TAB_SWITCHER (object);

  switch (prop_id) {
    case PROP_VIEW:
      g_value_set_object (value, kgx_tab_switcher_get_view (self));
      break;
    case PROP_NARROW:
      g_value_set_boolean (value, kgx_tab_switcher_get_narrow (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}


static void
kgx_tab_switcher_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  KgxTabSwitcher *self = KGX_TAB_SWITCHER (object);

  switch (prop_id) {
    case PROP_VIEW:
      kgx_tab_switcher_set_view (self, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}


static void
kgx_tab_switcher_destroy (GtkWidget *widget)
{
  gtk_container_forall (GTK_CONTAINER (widget), (GtkCallback) gtk_widget_destroy, NULL);

  GTK_WIDGET_CLASS (kgx_tab_switcher_parent_class)->destroy (widget);
}


static gboolean
kgx_tab_switcher_popup_menu (GtkWidget *widget)
{
  KgxTabSwitcher *self = KGX_TAB_SWITCHER (widget);
  GtkListBoxRow *row = gtk_list_box_get_selected_row (self->list);

  if (row) {
    do_popup (self, KGX_TAB_SWITCHER_ROW (row), NULL);

    return GDK_EVENT_STOP;
  }

  return GDK_EVENT_PROPAGATE;
}


static void
kgx_tab_switcher_size_allocate (GtkWidget     *widget,
                                GtkAllocation *alloc)
{
  KgxTabSwitcher *self = KGX_TAB_SWITCHER (widget);

  set_narrow (self, alloc->width < 400);

  GTK_WIDGET_CLASS (kgx_tab_switcher_parent_class)->size_allocate (widget, alloc);
}


static void
kgx_tab_switcher_add (GtkContainer *container,
                      GtkWidget    *widget)
{
  KgxTabSwitcher *self = KGX_TAB_SWITCHER (container);

  if (!self->flap) {
    GTK_CONTAINER_CLASS (kgx_tab_switcher_parent_class)->add (container, widget);

    return;
  }

  hdy_flap_set_content (self->flap, widget);
}


static void
kgx_tab_switcher_remove (GtkContainer *container,
                         GtkWidget    *widget)
{
  KgxTabSwitcher *self = KGX_TAB_SWITCHER (container);

  if (widget == GTK_WIDGET (self->flap)) {
    GTK_CONTAINER_CLASS (kgx_tab_switcher_parent_class)->remove (container, widget);

    return;
  }

  hdy_flap_set_content (self->flap, NULL);
}


static void
kgx_tab_switcher_forall (GtkContainer *container,
                         gboolean      include_internals,
                         GtkCallback   callback,
                         gpointer      callback_data)
{
  KgxTabSwitcher *self = KGX_TAB_SWITCHER (container);
  GtkWidget *content;

  if (include_internals) {
    GTK_CONTAINER_CLASS (kgx_tab_switcher_parent_class)->forall (container,
                                                                 include_internals,
                                                                 callback,
                                                                 callback_data);

    return;
  }

  if (!self->flap) {
    return;
  }

  content = hdy_flap_get_content (self->flap);

  if (content) {
    callback (content, callback_data);
  }
}


static void
kgx_tab_switcher_class_init (KgxTabSwitcherClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  object_class->dispose = kgx_tab_switcher_dispose;
  object_class->get_property = kgx_tab_switcher_get_property;
  object_class->set_property = kgx_tab_switcher_set_property;

  widget_class->destroy = kgx_tab_switcher_destroy;
  widget_class->popup_menu = kgx_tab_switcher_popup_menu;
  widget_class->size_allocate = kgx_tab_switcher_size_allocate;

  container_class->add = kgx_tab_switcher_add;
  container_class->remove = kgx_tab_switcher_remove;
  container_class->forall = kgx_tab_switcher_forall;

  /**
   * KgxTabSwitcher:view:
   *
   * The #HdyTabView the tab switcher controls;
   */
  pspecs[PROP_VIEW] =
    g_param_spec_object ("view",
                         "View",
                         "The view the tab switcher controls.",
                         HDY_TYPE_TAB_VIEW,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  pspecs[PROP_NARROW] =
    g_param_spec_boolean ("narrow",
                          "Narrow",
                          "Narrow",
                          TRUE,
                          G_PARAM_READABLE);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);

  signals[NEW_TAB] =
    g_signal_new ("new-tab",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE,
                  0);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               KGX_APPLICATION_PATH "kgx-tab-switcher.ui");

  gtk_widget_class_bind_template_child (widget_class, KgxTabSwitcher, flap);
  gtk_widget_class_bind_template_child (widget_class, KgxTabSwitcher, list);

  gtk_widget_class_bind_template_callback (widget_class, reveal_flap_cb);
  gtk_widget_class_bind_template_callback (widget_class, collapse_cb);
  gtk_widget_class_bind_template_callback (widget_class, new_tab_cb);
  gtk_widget_class_bind_template_callback (widget_class, row_selected_cb);
  gtk_widget_class_bind_template_callback (widget_class, row_activated_cb);
}


static void
kgx_tab_switcher_init (KgxTabSwitcher *self)
{
  self->narrow = TRUE;

  gtk_widget_init_template (GTK_WIDGET (self));

  self->click_gesture = gtk_gesture_multi_press_new (GTK_WIDGET (self->list));
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (self->click_gesture), 0);
  g_signal_connect_swapped (self->click_gesture, "pressed", G_CALLBACK (click_pressed_cb), self);

  self->long_press_gesture = gtk_gesture_long_press_new (GTK_WIDGET (self->list));
  gtk_gesture_single_set_touch_only (GTK_GESTURE_SINGLE (self->long_press_gesture), TRUE);
  g_signal_connect_swapped (self->long_press_gesture, "pressed", G_CALLBACK (long_press_cb), self);
}


/**
 * kgx_tab_switcher_new:
 *
 * Creates a new #KgxTabSwitcher widget.
 *
 * Returns: a new #KgxTabSwitcher
 */
GtkWidget *
kgx_tab_switcher_new (void)
{
  return g_object_new (KGX_TYPE_TAB_SWITCHER, NULL);
}


/**
 * kgx_tab_switcher_get_view:
 * @self: a #KgxTabSwitcher
 *
 * Gets the #HdyTabView @self controls.
 *
 * Returns: (transfer none) (nullable): the #HdyTabView @self controls
 */
HdyTabView *
kgx_tab_switcher_get_view (KgxTabSwitcher *self)
{
  g_return_val_if_fail (KGX_IS_TAB_SWITCHER (self), NULL);

  return self->view;
}


/**
 * kgx_tab_switcher_set_view:
 * @self: a #KgxTabSwitcher
 * @view: (nullable): a #HdyTabView
 *
 * Sets the #HdyTabView @self controls.
 */
void
kgx_tab_switcher_set_view (KgxTabSwitcher *self,
                           HdyTabView     *view)
{
  g_return_if_fail (KGX_IS_TAB_SWITCHER (self));
  g_return_if_fail (view == NULL || HDY_IS_TAB_VIEW (view));

  if (self->view == view) {
    return;
  }

  if (self->view) {
    GListModel *pages = hdy_tab_view_get_pages (self->view);

    g_signal_handlers_disconnect_by_func (self->view, G_CALLBACK (notify_selected_page_cb), self);
    g_signal_handlers_disconnect_by_func (pages, G_CALLBACK (pages_changed_cb), self);
  }

  g_set_object (&self->view, view);

  if (self->view) {
    GListModel *pages = hdy_tab_view_get_pages (self->view);

    g_signal_connect_object (pages, "items-changed",
                             G_CALLBACK (pages_changed_cb), self,
                             G_CONNECT_SWAPPED);

    g_signal_connect_object (self->view, "notify::selected-page",
                             G_CALLBACK (notify_selected_page_cb), self,
                             G_CONNECT_SWAPPED);
  }

  notify_selected_page_cb (self);

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_VIEW]);
}


void
kgx_tab_switcher_open (KgxTabSwitcher *self)
{
  g_return_if_fail (KGX_IS_TAB_SWITCHER (self));

  hdy_flap_set_reveal_flap (self->flap, TRUE);
}


void
kgx_tab_switcher_close (KgxTabSwitcher *self)
{
  g_return_if_fail (KGX_IS_TAB_SWITCHER (self));

  hdy_flap_set_reveal_flap (self->flap, FALSE);
}


gboolean
kgx_tab_switcher_get_narrow (KgxTabSwitcher *self)
{
  g_return_val_if_fail (KGX_IS_TAB_SWITCHER (self), FALSE);

  return self->narrow;
}
