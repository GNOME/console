/* kgx-system-info.c
 *
 * Copyright 2024 Zander Brown
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

#include <gio/gio.h>
#include <pango/pango.h>

#include "kgx-system-info.h"

#define INTERFACE_SETTINGS_SCHEMA "org.gnome.desktop.interface"
#define MONOSPACE_FONT_KEY_NAME "monospace-font-name"


struct _KgxSystemInfo {
  GObject parent;

  GSettings *desktop_interface;
  PangoFontDescription *monospace_font;
};

G_DEFINE_FINAL_TYPE (KgxSystemInfo, kgx_system_info, G_TYPE_OBJECT)


enum {
  PROP_0,
  PROP_MONOSPACE_FONT,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };


static void
kgx_system_info_dispose (GObject *object)
{
  KgxSystemInfo *self = KGX_SYSTEM_INFO (object);

  g_clear_object (&self->desktop_interface);
  g_clear_pointer (&self->monospace_font, pango_font_description_free);

  G_OBJECT_CLASS (kgx_system_info_parent_class)->dispose (object);
}


static void
kgx_system_info_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  KgxSystemInfo *self = KGX_SYSTEM_INFO (object);

  switch (property_id) {
    case PROP_MONOSPACE_FONT:
      g_value_set_boxed (value, self->monospace_font);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
kgx_system_info_class_init (KgxSystemInfoClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = kgx_system_info_dispose;
  object_class->get_property = kgx_system_info_get_property;

  pspecs[PROP_MONOSPACE_FONT] = g_param_spec_boxed ("monospace-font", NULL, NULL,
                                                    PANGO_TYPE_FONT_DESCRIPTION,
                                                    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);
}


static void
monospace_changed (GSettings     *settings,
                   const char    *key,
                   KgxSystemInfo *self)
{
  g_autofree char *font = g_settings_get_string (self->desktop_interface,
                                                 MONOSPACE_FONT_KEY_NAME);

  g_clear_pointer (&self->monospace_font, pango_font_description_free);
  self->monospace_font = pango_font_description_from_string (font);

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_MONOSPACE_FONT]);
}


static void
kgx_system_info_init (KgxSystemInfo *self)
{
  self->desktop_interface = g_settings_new (INTERFACE_SETTINGS_SCHEMA);
  g_signal_connect_object (self->desktop_interface,
                           "changed::" MONOSPACE_FONT_KEY_NAME, G_CALLBACK (monospace_changed),
                           self, G_CONNECT_DEFAULT);
  monospace_changed (self->desktop_interface, MONOSPACE_FONT_KEY_NAME, self);
}
