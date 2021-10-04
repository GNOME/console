/* kgx-nautilus.c
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

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include "kgx-nautilus.h"
#include "kgx-nautilus-menu-item.h"


static void kgx_nautilus_menu_provider_iface_init (NautilusMenuProviderIface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED (KgxNautilus, kgx_nautilus, G_TYPE_OBJECT, 0,
                                G_IMPLEMENT_INTERFACE_DYNAMIC (NAUTILUS_TYPE_MENU_PROVIDER,
                                                               kgx_nautilus_menu_provider_iface_init))


static void
kgx_nautilus_class_finalize (KgxNautilusClass *klass)
{
}


static void
kgx_nautilus_class_init (KgxNautilusClass *klass)
{
}


static GList *
get_items (KgxNautilus      *self,
           GtkWidget        *window,
           NautilusFileInfo *info,
           gboolean          is_back)
{
  g_autoptr (GFile) file = NULL;
  g_autofree char *path = NULL;
  GList *items = NULL;
  NautilusMenuItem *item;

  if (G_UNLIKELY (self->kgx == NULL)) {
    return items;
  }

  file = nautilus_file_info_get_location (info);

  item = kgx_nautilus_menu_item_new (self, window, file, is_back);
  items = g_list_append (items, item);

  return items;
}


static GList *
kgx_nautilus_get_background_items (NautilusMenuProvider *provider,
                                   GtkWidget            *window,
                                   NautilusFileInfo     *info)
{
  return get_items (KGX_NAUTILUS (provider), window, info, TRUE);
}


static GList *
kgx_nautilus_get_file_items (NautilusMenuProvider *provider,
                             GtkWidget            *window,
                             GList                *files)
{
  if (files == NULL || files->next != NULL) {
    return NULL;
  }

  if (!nautilus_file_info_is_directory (files->data)) {
    return NULL;
  }

  return get_items (KGX_NAUTILUS (provider), window, files->data, FALSE);
}


static void
kgx_nautilus_menu_provider_iface_init (NautilusMenuProviderIface *iface)
{
  iface->get_background_items = kgx_nautilus_get_background_items;
  iface->get_file_items = kgx_nautilus_get_file_items;
}


static void 
kgx_nautilus_init (KgxNautilus *self)
{
  g_autoptr (GDesktopAppInfo) info = NULL;
  
  info = g_desktop_app_info_new (KGX_APPLICATION_ID ".desktop");

  if (G_LIKELY (info)) {
    self->kgx = G_APP_INFO (g_steal_pointer (&info));
  } else {
    g_warning ("KGX (" KGX_APPLICATION_ID ") is missing");
  }
}


void
kgx_nautilus_open (KgxNautilus *self,
                   GtkWidget   *window,
                   GFile       *file)
{
  g_autoptr (GdkAppLaunchContext) context = NULL;
  g_autoptr (GError) error = NULL;
  g_autoptr (GList) list = NULL;

  list = g_list_append (list, file);

  context = gdk_display_get_app_launch_context (gtk_widget_get_display (window));
  g_app_info_launch (self->kgx, list, G_APP_LAUNCH_CONTEXT (context), &error);

  if (G_UNLIKELY (error)) {
    g_critical ("Everything is bad: %s", error->message);
  }
}


G_MODULE_EXPORT void
nautilus_module_initialize (GTypeModule *module)
{
  kgx_nautilus_register_type (module);
  kgx_nautilus_menu_item_register (module);

  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
}


G_MODULE_EXPORT void
nautilus_module_shutdown (void)
{
}


G_MODULE_EXPORT void
nautilus_module_list_types (const GType **types,
                            int          *num_types)
{
  static GType type_list[1];

  type_list[0] = KGX_TYPE_NAUTILUS;
  *types = type_list;

  *num_types = G_N_ELEMENTS (type_list);
}
