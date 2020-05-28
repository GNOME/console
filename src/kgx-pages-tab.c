/* kgx-pages-tab.c
 *
 * Copyright 2019 Zander Brown
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
 * SECTION:kgx-pages-tab
 * @title: KgxPagesTab
 * @short_description: Tab representing a #KgxPage in a #KgxPages
 * 
 * Since: 0.3.0
 */

#include <glib/gi18n.h>

#include "kgx-config.h"
#include "kgx-page.h"
#include "kgx-pages-tab.h"
#include "kgx-enums.h"
#include "kgx-window.h"


typedef struct _KgxPagesTabPrivate KgxPagesTabPrivate;
struct _KgxPagesTabPrivate {
  char        *title;
  KgxStatus    status;
  GActionMap  *actions;
};


G_DEFINE_TYPE_WITH_PRIVATE (KgxPagesTab, kgx_pages_tab, GTK_TYPE_EVENT_BOX)


enum {
  PROP_0,
  PROP_TITLE,
  PROP_DESCRIPTION,
  PROP_STATUS,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };


enum {
  CLOSE,
  DETACH,
  N_SIGNALS
};
static guint signals[N_SIGNALS];


static void
kgx_pages_tab_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  KgxPagesTab *self = KGX_PAGES_TAB (object);
  KgxPagesTabPrivate *priv = kgx_pages_tab_get_instance_private (self);
  GtkStyleContext *context;

  switch (property_id) {
    case PROP_TITLE:
      g_clear_pointer (&priv->title, g_free);
      priv->title = g_value_dup_string (value);
      break;
    case PROP_DESCRIPTION:
      gtk_widget_set_tooltip_markup (GTK_WIDGET (object),
                                     g_value_get_string (value));
      break;
    case PROP_STATUS:
      priv->status = g_value_get_flags (value);
      context = gtk_widget_get_style_context (GTK_WIDGET (self));

      if (priv->status & KGX_REMOTE) {
        gtk_style_context_add_class (context, KGX_WINDOW_STYLE_REMOTE);
      } else {
        gtk_style_context_remove_class (context, KGX_WINDOW_STYLE_REMOTE);
      }

      if (priv->status & KGX_PRIVILEGED) {
        gtk_style_context_add_class (context, KGX_WINDOW_STYLE_ROOT);
      } else {
        gtk_style_context_remove_class (context, KGX_WINDOW_STYLE_ROOT);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
kgx_pages_tab_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  KgxPagesTab *self = KGX_PAGES_TAB (object);
  KgxPagesTabPrivate *priv = kgx_pages_tab_get_instance_private (self);

  switch (property_id) {
    case PROP_TITLE:
      g_value_set_string (value, priv->title);
      break;
    case PROP_DESCRIPTION:
      g_value_set_string (value,
                          gtk_widget_get_tooltip_markup (GTK_WIDGET (object)));
      break;
    case PROP_STATUS:
      g_value_set_flags (value, priv->status);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
kgx_pages_tab_finalize (GObject *object)
{
  KgxPagesTab *self = KGX_PAGES_TAB (object);
  KgxPagesTabPrivate *priv = kgx_pages_tab_get_instance_private (self);

  g_clear_pointer (&priv->title, g_free);

  G_OBJECT_CLASS (kgx_pages_tab_parent_class)->finalize (object);
}


static void
context_menu (GtkWidget *widget,
              int        x,
              int        y,
              GdkEvent  *event)
{
  GtkApplication *app;
  GMenu          *model;
  GtkWidget      *menu;
  GdkRectangle    rect = {x, y, 1, 1};

  app = GTK_APPLICATION (g_application_get_default ());
  model = gtk_application_get_menu_by_id (app, "tab-menu");

  menu = gtk_popover_new_from_model (widget, G_MENU_MODEL (model));
  gtk_popover_set_pointing_to (GTK_POPOVER (menu), &rect);
  gtk_popover_popup (GTK_POPOVER (menu));
}


static gboolean
kgx_pages_tab_popup_menu (GtkWidget *self)
{
  context_menu (self, 1, 1, NULL);
  return TRUE;
}


static gboolean
kgx_pages_tab_button_press_event (GtkWidget *self, GdkEventButton *event)
{
  if (gdk_event_triggers_context_menu ((GdkEvent *) event) &&
      event->type == GDK_BUTTON_PRESS) {
    context_menu (self, event->x, event->y, (GdkEvent *) event);
    return TRUE;
  }

  return GTK_WIDGET_CLASS (kgx_pages_tab_parent_class)->button_press_event (self, event);
}


static void
kgx_pages_tab_class_init (KgxPagesTabClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->set_property = kgx_pages_tab_set_property;
  object_class->get_property = kgx_pages_tab_get_property;
  object_class->finalize = kgx_pages_tab_finalize;

  widget_class->popup_menu = kgx_pages_tab_popup_menu;
  widget_class->button_press_event = kgx_pages_tab_button_press_event;

  pspecs[PROP_TITLE] =
    g_param_spec_string ("title", "Title", "Tab title",
                         NULL,
                         G_PARAM_READWRITE);

  pspecs[PROP_DESCRIPTION] =
    g_param_spec_string ("description", "Description", "Tab description",
                         NULL,
                         G_PARAM_READWRITE);

  pspecs[PROP_STATUS] =
    g_param_spec_flags ("status", "Status", "Special state of the tab",
                         KGX_TYPE_STATUS,
                         KGX_NONE,
                         G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);

  signals[CLOSE] = g_signal_new ("close",
                                 G_TYPE_FROM_CLASS (klass),
                                 G_SIGNAL_RUN_LAST,
                                 0, NULL, NULL, NULL,
                                 G_TYPE_NONE,
                                 0);

  signals[DETACH] = g_signal_new ("detach",
                                  G_TYPE_FROM_CLASS (klass),
                                  G_SIGNAL_RUN_LAST,
                                  0, NULL, NULL, NULL,
                                  G_TYPE_NONE,
                                  0);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               RES_PATH "kgx-pages-tab.ui");
}


static void
close_activated (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       data)
{
  g_signal_emit (data, signals[CLOSE], 0);
}


static void
detach_activated (GSimpleAction *action,
                  GVariant      *parameter,
                  gpointer       data)
{
  g_signal_emit (data, signals[DETACH], 0);
}


static GActionEntry tab_entries[] =
{
  { "close", close_activated, NULL, NULL, NULL },
  { "detach", detach_activated, NULL, NULL, NULL },
};


static void
kgx_pages_tab_init (KgxPagesTab *self)
{
  KgxPagesTabPrivate *priv = kgx_pages_tab_get_instance_private (self);

  gtk_widget_set_has_window (GTK_WIDGET (self), TRUE);

  priv->actions = G_ACTION_MAP (g_simple_action_group_new ());
  g_action_map_add_action_entries (priv->actions,
                                   tab_entries,
                                   G_N_ELEMENTS (tab_entries),
                                   self);
  gtk_widget_insert_action_group (GTK_WIDGET (self),
                                  "tab",
                                  G_ACTION_GROUP (priv->actions));

  gtk_widget_add_events (GTK_WIDGET (self), GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

  gtk_widget_init_template (GTK_WIDGET (self));
}
