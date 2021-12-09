/* kgx-session-manager.c
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

/**
 * SECTION:kgx-session-manager
 * @short_description: An object with proxy details
 * @title: KgxSessionManager 
 *
 * #KgxSessionManager maps org.gnome.system.proxy to environmental variables
 * when launching new sessions
 * 
 * Note that whilst changes to system settings are tracked, they _cannot_ be
 * applied existing terminals
 * 
 * Only manual proxies are supported
 */

#include "kgx-config.h"

#include <gio/gio.h>

#include "kgx-session-manager.h"


struct _KgxSessionManager {
  GObject            parent_instance;

  GFile             *cache_dir;
};


G_DEFINE_TYPE (KgxSessionManager, kgx_session_manager, G_TYPE_OBJECT)


static void
kgx_session_manager_dispose (GObject *object)
{
  KgxSessionManager *self = KGX_SESSION_MANAGER (object);

  g_clear_object (&self->cache_dir);

  G_OBJECT_CLASS (kgx_session_manager_parent_class)->dispose (object);
}


static void
kgx_session_manager_class_init (KgxSessionManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = kgx_session_manager_dispose;
}


static void
kgx_session_manager_init (KgxSessionManager *self)
{
  self->cache_dir = g_file_new_build_filename (g_get_user_cache_dir (),
                                               KGX_APPLICATION_ID,
                                               TRUE);
}
