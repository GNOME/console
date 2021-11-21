/* kgx-nautilus-menu-item.c
 *
 * Copyright 2021 Zander Brown
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

#include "kgx-nautilus-menu-item.h"

#include <glib/gi18n-lib.h>


enum {
  PROP_0,
  PROP_EXTENSION,
  PROP_WINDOW,
  PROP_FILE,
  LAST_PROP
};

static GParamSpec *pspecs[LAST_PROP] = { NULL, };


G_DEFINE_DYNAMIC_TYPE (KgxNautilusMenuItem, kgx_nautilus_menu_item, NAUTILUS_TYPE_MENU_ITEM)


static void
kgx_nautilus_menu_item_class_finalize (KgxNautilusMenuItemClass *klass)
{

}


static void
kgx_nautilus_menu_item_dispose (GObject *object)
{
  KgxNautilusMenuItem *self = KGX_NAUTILUS_MENU_ITEM (object);

  g_clear_object (&self->extension);
  g_clear_object (&self->window);
  g_clear_object (&self->file);

  G_OBJECT_CLASS (kgx_nautilus_menu_item_parent_class)->dispose (object);
}



static void
kgx_nautilus_menu_item_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  KgxNautilusMenuItem *self = KGX_NAUTILUS_MENU_ITEM (object);

  switch (property_id) {
    case PROP_EXTENSION:
      g_set_object (&self->extension, g_value_get_object (value));
      break;
    case PROP_WINDOW:
      g_set_object (&self->window, g_value_get_object (value));
      break;
    case PROP_FILE:
      g_set_object (&self->file, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
kgx_nautilus_menu_item_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  KgxNautilusMenuItem *self = KGX_NAUTILUS_MENU_ITEM (object);

  switch (property_id) {
    case PROP_EXTENSION:
      g_value_set_object (value, self->extension);
      break;
    case PROP_WINDOW:
      g_value_set_object (value, self->window);
      break;
    case PROP_FILE:
      g_value_set_object (value, self->file);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
kgx_nautilus_menu_item_constructed (GObject *object)
{
  KgxNautilusMenuItem *self = KGX_NAUTILUS_MENU_ITEM (object);

  G_OBJECT_CLASS (kgx_nautilus_menu_item_parent_class)->constructed (object);

  g_object_set (self,
#ifdef IS_DEVEL
                "label", _("Open in Co_nsole (Devel)"),
#else
                "label", _("Open in Co_nsole"),
#endif
                "tip", _("Start a terminal session for this location"),
                NULL);
}


static void
kgx_nautilus_menu_item_activate (NautilusMenuItem *item)
{
  KgxNautilusMenuItem *self = KGX_NAUTILUS_MENU_ITEM (item);

  kgx_nautilus_open (self->extension, self->window, self->file);
}


static void
kgx_nautilus_menu_item_class_init (KgxNautilusMenuItemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  NautilusMenuItemClass *menu_item_class = NAUTILUS_MENU_ITEM_CLASS (klass);

  object_class->dispose = kgx_nautilus_menu_item_dispose;
  object_class->set_property = kgx_nautilus_menu_item_set_property;
  object_class->get_property = kgx_nautilus_menu_item_get_property;
  object_class->constructed = kgx_nautilus_menu_item_constructed;

  menu_item_class->activate = kgx_nautilus_menu_item_activate;

  pspecs[PROP_EXTENSION] =
    g_param_spec_object ("extension", "Extension", "The KGX extension object",
                         KGX_TYPE_NAUTILUS,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  pspecs[PROP_WINDOW] =
    g_param_spec_object ("window", "Window", "The window containing the file",
                         GTK_TYPE_WIDGET,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  pspecs[PROP_FILE] =
    g_param_spec_object ("file", "File", "The file the item is for",
                         G_TYPE_FILE,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);
}


static void 
kgx_nautilus_menu_item_init (KgxNautilusMenuItem *self)
{
}


void
kgx_nautilus_menu_item_register (GTypeModule *module)
{
  kgx_nautilus_menu_item_register_type (module);
}


NautilusMenuItem *
kgx_nautilus_menu_item_new (KgxNautilus *extension,
                            GtkWidget   *window,
                            GFile       *file,
                            gboolean     is_back)
{
  return g_object_new (KGX_TYPE_NAUTILUS_MENU_ITEM,
                       "extension", extension,
                       "window", window,
                       "file", file,
#ifdef IS_DEVEL
                       "name", is_back ? "KgxDevel:Open" : "KgxDevel:OpenItem",
#else
                       "name", is_back ? "Kgx:Open" : "Kgx:OpenItem",
#endif
                       "icon", KGX_APPLICATION_ID,
                       NULL);
}
