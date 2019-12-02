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
#include "kgx-pages-tab.h"


G_DEFINE_TYPE (KgxPagesTab, kgx_pages_tab, GTK_TYPE_BOX)


enum {
  PROP_0,
  PROP_TITLE,
  LAST_PROP
};

static GParamSpec *pspecs[LAST_PROP] = { NULL, };


static void
kgx_pages_tab_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  KgxPagesTab *self = KGX_PAGES_TAB (object);

  switch (property_id) {
    case PROP_TITLE:
      g_clear_pointer (&self->title, g_free);
      self->title = g_value_dup_string (value);
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

  switch (property_id) {
    case PROP_TITLE:
      g_value_set_string (value, self->title);
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

  g_clear_pointer (&self->title, g_free);

  G_OBJECT_CLASS (kgx_pages_tab_parent_class)->finalize (object);
}


static void
kgx_pages_tab_class_init (KgxPagesTabClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->set_property = kgx_pages_tab_set_property;
  object_class->get_property = kgx_pages_tab_get_property;
  object_class->finalize = kgx_pages_tab_finalize;

  pspecs[PROP_TITLE] =
    g_param_spec_string ("title", "Title", "Tab title",
                         NULL,
                         G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               RES_PATH "kgx-pages-tab.ui");
}

static void
kgx_pages_tab_init (KgxPagesTab *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}