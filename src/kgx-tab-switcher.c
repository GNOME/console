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
 * @short_description: A mobile tab switcher for AdwTabView
 * @title: KgxTabSwitcher
 *
 * The #KgxTabSwitcher widget is a mobile tab switcher for AdwTabView. It's
 * supposed to be used in conjunction with #KgxTabButton to open it, and a
 * #AdwTabBar to provide UI for larger screens.
 */

#include "kgx-config.h"
#include "kgx-tab-switcher.h"
#include "kgx-tab-switcher-row.h"
#include "kgx-marshals.h"


struct _KgxTabSwitcher {
  GtkWidget parent_instance;

  AdwFlap *flap;
  GtkListBox *list;

  GtkGesture *click_gesture;
  GtkGesture *long_press_gesture;
  GtkPopover *context_menu;

  AdwTabView *view;
  gboolean narrow;
};


G_DEFINE_TYPE (KgxTabSwitcher, kgx_tab_switcher, GTK_TYPE_WIDGET)


enum {
  PROP_0,
  PROP_CHILD,
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
context_menu_notify_visible_cb (KgxTabSwitcher *self)
{
  if (!self->context_menu || gtk_widget_get_visible (GTK_WIDGET (self->context_menu))) {
    return;
  }

  g_idle_add (G_SOURCE_FUNC (reset_setup_menu_cb), self);
}


static void
do_popup (KgxTabSwitcher    *self,
          KgxTabSwitcherRow *row,
          double             x,
          double             y)
{
  GMenuModel *model = adw_tab_view_get_menu_model (self->view);
  AdwTabPage *page = kgx_tab_switcher_row_get_page (row);
  GdkRectangle rect;

  if (!G_IS_MENU_MODEL (model)) {
    return;
  }

  g_signal_emit_by_name (self->view, "setup-menu", page);

  if (!self->context_menu) {
    self->context_menu = GTK_POPOVER (gtk_popover_menu_new_from_model (model));
    gtk_popover_set_position (self->context_menu, GTK_POS_BOTTOM);
    gtk_popover_set_has_arrow (self->context_menu, FALSE);
    gtk_widget_set_parent (GTK_WIDGET (self->context_menu) , GTK_WIDGET (self));

    if (gtk_widget_get_direction (GTK_WIDGET (self)) == GTK_TEXT_DIR_RTL) {
      gtk_widget_set_halign (GTK_WIDGET (self->context_menu), GTK_ALIGN_END);
    } else {
      gtk_widget_set_halign (GTK_WIDGET (self->context_menu), GTK_ALIGN_START);
    }

    g_signal_connect_object (self->context_menu, "notify::visible",
                             G_CALLBACK (context_menu_notify_visible_cb), self,
                             G_CONNECT_AFTER | G_CONNECT_SWAPPED);
  }

  if (x >= 0 && y >= 0) {
    graphene_rect_t bounds;

    g_assert (gtk_widget_compute_bounds (GTK_WIDGET (self->list),
                                         GTK_WIDGET (self), &bounds));

    rect.x = bounds.origin.x + x;
    rect.y = bounds.origin.y + y;
  } else {
    graphene_rect_t bounds;

    g_assert (gtk_widget_compute_bounds (GTK_WIDGET (row),
                                         GTK_WIDGET (self), &bounds));

    rect.x = bounds.origin.x;
    rect.y = bounds.origin.y + bounds.size.height;

    if (gtk_widget_get_direction (GTK_WIDGET (self)) == GTK_TEXT_DIR_RTL) {
      rect.x += bounds.size.width;
    }
  }

  rect.width = 0;
  rect.height = 0;

  gtk_popover_set_pointing_to (self->context_menu, &rect);

  gtk_popover_popup (self->context_menu);
}


static void
reveal_flap_cb (KgxTabSwitcher *self)
{
  AdwTabPage *page;
  GtkWidget *child;

  gtk_widget_set_sensitive (GTK_WIDGET (self->view),
                            !adw_flap_get_reveal_flap (self->flap));

  if (adw_flap_get_reveal_flap (self->flap)) {
    gtk_widget_grab_focus (GTK_WIDGET (self->list));
  } else {
    page = adw_tab_view_get_selected_page (self->view);
    child = adw_tab_page_get_child (page);

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
  AdwTabPage *page;

  if (!row) {
    return;
  }

  g_assert (KGX_IS_TAB_SWITCHER_ROW (row));

  if (!self->view) {
    return;
  }

  page = kgx_tab_switcher_row_get_page (row);
  adw_tab_view_set_selected_page (self->view, page);
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
    g_autoptr (AdwTabPage) page = g_list_model_get_item (pages, position + i);
    GtkWidget *row = kgx_tab_switcher_row_new (page, self->view);

    gtk_list_box_insert (self->list, row, position + i);
    kgx_tab_switcher_row_animate_open (KGX_TAB_SWITCHER_ROW (row));
  }
}


static void
selection_changed_cb (KgxTabSwitcher *self)
{
  AdwTabPage *page = NULL;

  if (self->view) {
    page = adw_tab_view_get_selected_page (self->view);
  }

  if (page) {
    int index = adw_tab_view_get_page_position (self->view, page);
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
  GdkEventSequence *sequence;
  GdkEvent *event;
  GtkListBoxRow *row;
  AdwTabPage *page;
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

  sequence = gtk_gesture_single_get_current_sequence (GTK_GESTURE_SINGLE (self->click_gesture));
  event = gtk_gesture_get_last_event (self->click_gesture, sequence);
  button = gtk_gesture_single_get_current_button (GTK_GESTURE_SINGLE (self->click_gesture));
  page = kgx_tab_switcher_row_get_page (KGX_TAB_SWITCHER_ROW (row));

  if (event && gdk_event_triggers_context_menu (event)) {
    gtk_gesture_set_state (self->click_gesture, GTK_EVENT_SEQUENCE_CLAIMED);
    do_popup (self, KGX_TAB_SWITCHER_ROW (row), x, y);

    return;
  }

  if (button == GDK_BUTTON_MIDDLE) {
    gtk_gesture_set_state (self->click_gesture, GTK_EVENT_SEQUENCE_CLAIMED);
    adw_tab_view_close_page (self->view, page);

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

  do_popup (self, KGX_TAB_SWITCHER_ROW (row), -1, -1);

  gtk_gesture_set_state (self->long_press_gesture, GTK_EVENT_SEQUENCE_CLAIMED);
}


static void
popup_menu_cb (KgxTabSwitcher *self)
{
  GtkListBoxRow *row = gtk_list_box_get_selected_row (self->list);

  do_popup (self, KGX_TAB_SWITCHER_ROW (row), -1, -1);
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

  g_clear_pointer ((GtkWidget **) &self->flap, gtk_widget_unparent);
  g_clear_pointer ((GtkWidget **) &self->context_menu, gtk_widget_unparent);
  self->click_gesture = NULL;
  self->long_press_gesture = NULL;

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
    case PROP_CHILD:
      g_value_set_object (value, kgx_tab_switcher_get_child (self));
      break;
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
    case PROP_CHILD:
      kgx_tab_switcher_set_child (self, g_value_get_object (value));
      break;
    case PROP_VIEW:
      kgx_tab_switcher_set_view (self, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}


static void
kgx_tab_switcher_measure (GtkWidget      *widget,
                          GtkOrientation  orientation,
                          int             for_size,
                          int            *min,
                          int            *nat,
                          int            *min_baseline,
                          int            *nat_baseline)
{
  KgxTabSwitcher *self = KGX_TAB_SWITCHER (widget);

  gtk_widget_measure (GTK_WIDGET (self->flap), orientation, for_size,
                      min, nat, min_baseline, nat_baseline);
}


static void
kgx_tab_switcher_size_allocate (GtkWidget *widget,
                                int        width,
                                int        height,
                                int        baseline)
{
  KgxTabSwitcher *self = KGX_TAB_SWITCHER (widget);

  set_narrow (self, width < 400);

  if (self->context_menu) {
    gtk_popover_present (self->context_menu);
  }

  gtk_widget_allocate (GTK_WIDGET (self->flap), width, height, baseline, NULL);
}


static void
kgx_tab_switcher_direction_changed (GtkWidget        *widget,
                                    GtkTextDirection  previous_direction)
{
  KgxTabSwitcher *self = KGX_TAB_SWITCHER (widget);

  if (gtk_widget_get_direction (widget) == previous_direction) {
    return;
  }

  if (self->context_menu) {
    if (gtk_widget_get_direction (GTK_WIDGET (self)) == GTK_TEXT_DIR_RTL) {
      gtk_widget_set_halign (GTK_WIDGET (self->context_menu), GTK_ALIGN_END);
    } else {
      gtk_widget_set_halign (GTK_WIDGET (self->context_menu), GTK_ALIGN_START);
    }
  }
}



static void
kgx_tab_switcher_class_init (KgxTabSwitcherClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = kgx_tab_switcher_dispose;
  object_class->get_property = kgx_tab_switcher_get_property;
  object_class->set_property = kgx_tab_switcher_set_property;

  widget_class->measure = kgx_tab_switcher_measure;
  widget_class->size_allocate = kgx_tab_switcher_size_allocate;
  widget_class->direction_changed = kgx_tab_switcher_direction_changed;

  pspecs[PROP_CHILD] =
    g_param_spec_object ("child",
                         "Child",
                         "The tab switcher child",
                         GTK_TYPE_WIDGET,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * KgxTabSwitcher:view:
   *
   * The #AdwTabView the tab switcher controls;
   */
  pspecs[PROP_VIEW] =
    g_param_spec_object ("view",
                         "View",
                         "The view the tab switcher controls.",
                         ADW_TYPE_TAB_VIEW,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  pspecs[PROP_NARROW] =
    g_param_spec_boolean ("narrow",
                          "Narrow",
                          "Narrow",
                          TRUE,
                          G_PARAM_READABLE);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);

  signals[NEW_TAB] = g_signal_new ("new-tab",
                                   G_TYPE_FROM_CLASS (klass),
                                   G_SIGNAL_RUN_LAST,
                                   0,
                                   NULL, NULL,
                                   kgx_marshals_VOID__VOID,
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

  gtk_widget_class_install_action (widget_class, "menu.popup", NULL, (GtkWidgetActionActivateFunc) popup_menu_cb);

  gtk_widget_class_add_binding_action (widget_class, GDK_KEY_F10, GDK_SHIFT_MASK, "menu.popup", NULL);
  gtk_widget_class_add_binding_action (widget_class, GDK_KEY_Menu, 0, "menu.popup", NULL);
}


static void
kgx_tab_switcher_init (KgxTabSwitcher *self)
{
  self->narrow = TRUE;

  gtk_widget_init_template (GTK_WIDGET (self));

  self->click_gesture = gtk_gesture_click_new ();
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (self->click_gesture), 0);
  g_signal_connect_swapped (self->click_gesture, "pressed", G_CALLBACK (click_pressed_cb), self);
  gtk_widget_add_controller (GTK_WIDGET (self->list), GTK_EVENT_CONTROLLER (self->click_gesture));

  self->long_press_gesture = gtk_gesture_long_press_new ();
  gtk_gesture_single_set_touch_only (GTK_GESTURE_SINGLE (self->long_press_gesture), TRUE);
  g_signal_connect_swapped (self->long_press_gesture, "pressed", G_CALLBACK (long_press_cb), self);
  gtk_widget_add_controller (GTK_WIDGET (self->list), GTK_EVENT_CONTROLLER (self->long_press_gesture));
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


GtkWidget *
kgx_tab_switcher_get_child (KgxTabSwitcher *self)
{
  g_return_val_if_fail (KGX_IS_TAB_SWITCHER (self), NULL);

  return adw_flap_get_content (self->flap);
}


void
kgx_tab_switcher_set_child (KgxTabSwitcher *self,
                            GtkWidget      *child)
{
  g_return_if_fail (KGX_IS_TAB_SWITCHER (self));
  g_return_if_fail (child == NULL || GTK_IS_WIDGET (child));

  if (child == kgx_tab_switcher_get_child (self)) {
    return;
  }

  adw_flap_set_content (self->flap, child);

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_CHILD]);
}


/**
 * kgx_tab_switcher_get_view:
 * @self: a #KgxTabSwitcher
 *
 * Gets the #AdwTabView @self controls.
 *
 * Returns: (transfer none) (nullable): the #AdwTabView @self controls
 */
AdwTabView *
kgx_tab_switcher_get_view (KgxTabSwitcher *self)
{
  g_return_val_if_fail (KGX_IS_TAB_SWITCHER (self), NULL);

  return self->view;
}


/**
 * kgx_tab_switcher_set_view:
 * @self: a #KgxTabSwitcher
 * @view: (nullable): a #AdwTabView
 *
 * Sets the #AdwTabView @self controls.
 */
void
kgx_tab_switcher_set_view (KgxTabSwitcher *self,
                           AdwTabView     *view)
{
  g_return_if_fail (KGX_IS_TAB_SWITCHER (self));
  g_return_if_fail (view == NULL || ADW_IS_TAB_VIEW (view));

  if (self->view == view) {
    return;
  }

  if (self->view) {
    GtkSelectionModel *pages = adw_tab_view_get_pages (self->view);

    g_signal_handlers_disconnect_by_func (pages, G_CALLBACK (selection_changed_cb), self);
    g_signal_handlers_disconnect_by_func (pages, G_CALLBACK (pages_changed_cb), self);
  }

  g_set_object (&self->view, view);

  if (self->view) {
    GtkSelectionModel *pages = adw_tab_view_get_pages (self->view);

    g_signal_connect_object (pages, "items-changed",
                             G_CALLBACK (pages_changed_cb), self,
                             G_CONNECT_SWAPPED);

    g_signal_connect_object (pages, "selection-changed",
                             G_CALLBACK (selection_changed_cb), self,
                             G_CONNECT_SWAPPED);
  }

  selection_changed_cb (self);

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_VIEW]);
}


void
kgx_tab_switcher_open (KgxTabSwitcher *self)
{
  g_return_if_fail (KGX_IS_TAB_SWITCHER (self));

  adw_flap_set_reveal_flap (self->flap, TRUE);
}


void
kgx_tab_switcher_close (KgxTabSwitcher *self)
{
  g_return_if_fail (KGX_IS_TAB_SWITCHER (self));

  adw_flap_set_reveal_flap (self->flap, FALSE);
}


gboolean
kgx_tab_switcher_get_narrow (KgxTabSwitcher *self)
{
  g_return_val_if_fail (KGX_IS_TAB_SWITCHER (self), FALSE);

  return self->narrow;
}
