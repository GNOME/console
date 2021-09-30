/* kgx-tab-button.c
 *
 * Copyright 2019 Alexander Mikhaylenko <exalm7659@gmail.com>
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
 * SECTION:hdy-tab-button
 * @short_description: A button that displays the number of #HdyTabView pages
 * @title: KgxTabButton
 *
 * The #KgxTabButton widget is a #GtkButton subclass that displays the number
 * of pages in a given #HdyTabView.
 *
 * It can be used to open a tab switcher view in a mobile UI.
 *
 * # CSS nodes
 *
 * #KgxTabButton has a main CSS node with name button and style class
 * .tab-button.
 *
 * It contains the subnode overlay, which contains nodes image and label. The
 * label subnode can contain the .small style class for 10 or more tabs.
 */

#include "kgx-config.h"
#include "kgx-tab-button.h"


/* Copied from GtkInspector code */
#define XFT_DPI_MULTIPLIER (96.0 * PANGO_SCALE)


struct _KgxTabButton {
  GtkButton parent_instance;

  GtkLabel *label;
  GtkImage *icon;

  HdyTabView *view;
};


G_DEFINE_TYPE (KgxTabButton, kgx_tab_button, GTK_TYPE_BUTTON)


enum {
  PROP_0,
  PROP_VIEW,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };


/* FIXME: I hope there is a better way to prevent the label from changing scale */
static void
update_label_scale (KgxTabButton *self,
                    GtkSettings  *settings)
{
  int xft_dpi;
  PangoAttrList *attrs;
  PangoAttribute *scale_attribute;

  g_object_get (settings, "gtk-xft-dpi", &xft_dpi, NULL);

  attrs = pango_attr_list_new ();

  scale_attribute = pango_attr_scale_new (XFT_DPI_MULTIPLIER / (double) xft_dpi);

  pango_attr_list_change (attrs, scale_attribute);

  gtk_label_set_attributes (self->label, attrs);

  pango_attr_list_unref (attrs);
}


static void
xft_dpi_changed (KgxTabButton *self,
                 GParamSpec   *pspec,
                 GtkSettings  *settings)
{
  update_label_scale (self, settings);
}


static void
update_icon (KgxTabButton *self)
{
  gboolean display_label = FALSE;
  gboolean small_label = FALSE;
  const char *icon_name = "tab-counter-symbolic";
  g_autofree char *label_text = NULL;
  GtkStyleContext *context;

  if (self->view) {
    guint n_pages = hdy_tab_view_get_n_pages (self->view);

    small_label = n_pages >= 10;

    if (n_pages < 100) {
      display_label = TRUE;
      label_text = g_strdup_printf ("%u", n_pages);
    } else {
      icon_name = "tab-overflow-symbolic";
    }
  }

  context = gtk_widget_get_style_context (GTK_WIDGET (self->label));

  if (small_label) {
    gtk_style_context_add_class (context, "small");
  } else {
    gtk_style_context_remove_class (context, "small");
  }

  gtk_widget_set_visible (GTK_WIDGET (self->label), display_label);
  gtk_label_set_text (self->label, label_text);
  gtk_image_set_from_icon_name (self->icon, icon_name, GTK_ICON_SIZE_BUTTON);
}


static void
kgx_tab_button_dispose (GObject *object)
{
  KgxTabButton *self = KGX_TAB_BUTTON (object);

  kgx_tab_button_set_view (self, NULL);

  G_OBJECT_CLASS (kgx_tab_button_parent_class)->dispose (object);
}


static void
kgx_tab_button_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  KgxTabButton *self = KGX_TAB_BUTTON (object);

  switch (prop_id) {
    case PROP_VIEW:
      g_value_set_object (value, kgx_tab_button_get_view (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}


static void
kgx_tab_button_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  KgxTabButton *self = KGX_TAB_BUTTON (object);

  switch (prop_id) {
    case PROP_VIEW:
      kgx_tab_button_set_view (self, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}


static void
kgx_tab_button_class_init (KgxTabButtonClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = kgx_tab_button_dispose;
  object_class->get_property = kgx_tab_button_get_property;
  object_class->set_property = kgx_tab_button_set_property;

  /**
   * KgxTabButton:view:
   *
   * The #HdyTabView the tab button displays.
   */
  pspecs[PROP_VIEW] =
    g_param_spec_object ("view",
                         "View",
                         "The view the tab button displays.",
                         HDY_TYPE_TAB_VIEW,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               KGX_APPLICATION_PATH "kgx-tab-button.ui");

  gtk_widget_class_bind_template_child (widget_class, KgxTabButton, label);
  gtk_widget_class_bind_template_child (widget_class, KgxTabButton, icon);
}


static void
kgx_tab_button_init (KgxTabButton *self)
{
  GtkSettings *settings;

  gtk_widget_init_template (GTK_WIDGET (self));

  update_icon (self);

  settings = gtk_widget_get_settings (GTK_WIDGET (self));

  update_label_scale (self, settings);
  g_signal_connect_object (settings, "notify::gtk-xft-dpi",
                           G_CALLBACK (xft_dpi_changed), self,
                           G_CONNECT_SWAPPED);
}


/**
 * kgx_tab_button_new:
 *
 * Creates a new #KgxTabButton widget.
 *
 * Returns: a new #KgxTabButton
 */
GtkWidget *
kgx_tab_button_new (void)
{
  return g_object_new (KGX_TYPE_TAB_BUTTON, NULL);
}


/**
 * kgx_tab_button_get_view:
 * @self: a #KgxTabButton
 *
 * Gets the #HdyTabView @self displays.
 *
 * Returns: (transfer none) (nullable): the #HdyTabView @self displays
 */
HdyTabView *
kgx_tab_button_get_view (KgxTabButton *self)
{
  g_return_val_if_fail (KGX_IS_TAB_BUTTON (self), NULL);

  return self->view;
}


/**
 * kgx_tab_button_set_view:
 * @self: a #KgxTabButton
 * @view: (nullable): a #HdyTabView
 *
 * Sets the #HdyTabView @self displays.
 */
void
kgx_tab_button_set_view (KgxTabButton *self,
                         HdyTabView   *view)
{
  g_return_if_fail (KGX_IS_TAB_BUTTON (self));
  g_return_if_fail (view == NULL || HDY_IS_TAB_VIEW (view));

  if (self->view == view) {
    return;
  }

  if (self->view) {
    g_signal_handlers_disconnect_by_func (self->view,
                                          update_icon,
                                          self);
  }

  g_set_object (&self->view, view);

  if (self->view) {
    g_signal_connect_object (self->view, "notify::n-pages",
                             G_CALLBACK (update_icon), self,
                             G_CONNECT_SWAPPED);
  }

  update_icon (self);

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_VIEW]);
}
