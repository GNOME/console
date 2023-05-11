/* kgx-settings.c
 *
 * Copyright 2022 Zander Brown
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
 * SECTION:kgx-proxy-info
 * @short_description: An object with proxy details
 * @title: KgxProxyInfo
 *
 * #KgxProxyInfo maps org.gnome.system.proxy to environmental variables
 * when launching new sessions
 *
 * Note that whilst changes to system settings are tracked, they _cannot_ be
 * applied existing terminals
 *
 * Only manual proxies are supported
 */

#include "kgx-config.h"

#include <gio/gio.h>
#include <vte/vte.h>

#include "kgx-settings.h"


/**
 * DESKTOP_INTERFACE_SETTINGS_SCHEMA:
 * The schema that defines the system fonts
 */
#define DESKTOP_INTERFACE_SETTINGS_SCHEMA "org.gnome.desktop.interface"

/**
 * MONOSPACE_FONT_KEY_NAME:
 * The name of the key in %DESKTOP_INTERFACE_SETTINGS_SCHEMA for the monospace
 * font
 */
#define MONOSPACE_FONT_KEY_NAME "monospace-font-name"

#define RESTORE_SIZE_KEY "restore-window-size"
#define LAST_SIZE_KEY "last-window-size"

#define AUDIBLE_BELL "audible-bell"

#define USE_CUSTOM_FONT "use-custom-font"
#define CUSTOM_FONT "custom-font"

struct _KgxSettings {
  GObject      parent_instance;

  KgxTheme              theme;
  double                scale;
  int64_t               scrollback_lines;
  gboolean              audible_bell;
  gboolean              use_custom_font;
  PangoFontDescription *custom_font;

  GSettings            *settings;
  GSettings            *desktop_interface;
};


G_DEFINE_TYPE (KgxSettings, kgx_settings, G_TYPE_OBJECT)


enum {
  PROP_0,
  PROP_THEME,
  PROP_FONT,
  PROP_FONT_SCALE,
  PROP_SCALE_CAN_INCREASE,
  PROP_SCALE_CAN_DECREASE,
  PROP_SCROLLBACK_LINES,
  PROP_AUDIBLE_BELL,
  PROP_USE_CUSTOM_FONT,
  PROP_CUSTOM_FONT,
  LAST_PROP
};

static GParamSpec *pspecs[LAST_PROP] = { NULL, };


static void
kgx_settings_dispose (GObject *object)
{
  KgxSettings *self = KGX_SETTINGS (object);

  g_clear_object (&self->settings);
  g_clear_object (&self->desktop_interface);

  G_OBJECT_CLASS (kgx_settings_parent_class)->dispose (object);
}


static void
update_scale (KgxSettings *self, double value)
{
  double clamped = CLAMP (value, KGX_FONT_SCALE_MIN, KGX_FONT_SCALE_MAX);

  if (self->scale == clamped) {
    return;
  }

  self->scale = clamped;

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_FONT_SCALE]);
  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_SCALE_CAN_INCREASE]);
  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_SCALE_CAN_DECREASE]);
}


static void
kgx_settings_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  KgxSettings *self = KGX_SETTINGS (object);

  switch (property_id) {
    case PROP_THEME:
      self->theme = g_value_get_enum (value);
      break;
    case PROP_FONT_SCALE:
      update_scale (self, g_value_get_double (value));
      break;
    case PROP_SCROLLBACK_LINES:
      kgx_settings_set_scrollback (self, g_value_get_int64 (value));
      break;
    case PROP_AUDIBLE_BELL:
      kgx_settings_set_audible_bell (self, g_value_get_boolean (value));
      break;
    case PROP_USE_CUSTOM_FONT:
      kgx_settings_set_use_custom_font (self, g_value_get_boolean (value));
      break;
    case PROP_CUSTOM_FONT:
      kgx_settings_set_custom_font (self, g_value_get_boxed (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
kgx_settings_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  KgxSettings *self = KGX_SETTINGS (object);

  switch (property_id) {
    case PROP_THEME:
      g_value_set_enum (value, self->theme);
      break;
    case PROP_FONT:
      g_value_take_boxed (value, kgx_settings_get_font (self));
      break;
    case PROP_FONT_SCALE:
      g_value_set_double (value, self->scale);
      break;
    case PROP_SCALE_CAN_INCREASE:
      g_value_set_boolean (value, self->scale < KGX_FONT_SCALE_MAX);
      break;
    case PROP_SCALE_CAN_DECREASE:
      g_value_set_boolean (value, self->scale > KGX_FONT_SCALE_MIN);
      break;
    case PROP_SCROLLBACK_LINES:
      g_value_set_int64 (value, self->scrollback_lines);
      break;
    case PROP_AUDIBLE_BELL:
      g_value_set_boolean (value, self->audible_bell);
      break;
    case PROP_USE_CUSTOM_FONT:
      g_value_set_boolean (value, self->use_custom_font);
      break;
    case PROP_CUSTOM_FONT:
      g_value_set_boxed (value, self->custom_font);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
kgx_settings_class_init (KgxSettingsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = kgx_settings_dispose;
  object_class->set_property = kgx_settings_set_property;
  object_class->get_property = kgx_settings_get_property;

  /**
   * KgxSettings:theme:
   *
   * The palette to use, one of the values of #KgxTheme
   *
   * Officially only "night" exists, "hacker" is just a little fun
   *
   * Bound to ‘theme’ GSetting so changes persist
   */
  pspecs[PROP_THEME] =
    g_param_spec_enum ("theme", "Theme", "Terminal theme",
                       KGX_TYPE_THEME, KGX_THEME_NIGHT,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_FONT] =
    g_param_spec_boxed ("font", "Font", "Monospace font",
                         PANGO_TYPE_FONT_DESCRIPTION,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  pspecs[PROP_FONT_SCALE] =
    g_param_spec_double ("font-scale", "Font scale", "Font scaling",
                         KGX_FONT_SCALE_MIN,
                         KGX_FONT_SCALE_MAX,
                         KGX_FONT_SCALE_DEFAULT,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  pspecs[PROP_SCALE_CAN_INCREASE] =
    g_param_spec_boolean ("scale-can-increase", NULL, NULL,
                          TRUE,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  pspecs[PROP_SCALE_CAN_DECREASE] =
    g_param_spec_boolean ("scale-can-decrease", NULL, NULL,
                          TRUE,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * KgxSettings:scrollback-lines:
   *
   * How many lines of scrollback #KgxTerminal should keep
   *
   * Bound to ‘scrollback-lines’ GSetting so changes persist
   */
  pspecs[PROP_SCROLLBACK_LINES] =
    g_param_spec_int64 ("scrollback-lines", "Scrollback Lines", "Size of the scrollback",
                        G_MININT64, G_MAXINT64, 512,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  pspecs[PROP_AUDIBLE_BELL] =
    g_param_spec_boolean ("audible-bell", NULL, NULL,
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  pspecs[PROP_USE_CUSTOM_FONT] =
    g_param_spec_boolean ("use-custom-font", "Use Custom Font", "Whether to use a custom font",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  pspecs[PROP_CUSTOM_FONT] =
    g_param_spec_boxed ("custom-font", "Custom Font", "The custom font",
                        PANGO_TYPE_FONT_DESCRIPTION,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);
}


static void
font_changed (GSettings    *settings,
              const char   *key,
              KgxSettings  *self)
{
  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_FONT]);
}


static void
restore_window_size_changed (GSettings   *settings,
                             const char  *key,
                             KgxSettings *self)
{
  if (!g_settings_get_boolean (self->settings, RESTORE_SIZE_KEY)) {
    g_settings_set (self->settings, LAST_SIZE_KEY, "(ii)", -1, -1);
  }
}

static gboolean
custom_font_get_mapping (GValue   *value,
                         GVariant *variant,
                         gpointer  data)
{
  const char *font_desc;
  PangoFontDescription *font;

  font_desc = g_variant_get_string (variant, NULL);
  if (font_desc == NULL || g_strcmp0 (font_desc, "") == 0)
    font = pango_font_description_new ();
  else
    font = pango_font_description_from_string (font_desc);

  g_value_set_boxed (value, font);

  pango_font_description_free (font);

  return TRUE;
}


static GVariant *
custom_font_set_mapping (const GValue       *value,
                         const GVariantType *variant_ty,
                         gpointer            data)
{
  g_autofree char *font_desc = NULL;
  PangoFontDescription *font;

  font = g_value_get_boxed (value);
  font_desc = pango_font_description_to_string (font);

  return g_variant_new_string (font_desc);
}


static void
kgx_settings_init (KgxSettings *self)
{
  self->settings = g_settings_new (KGX_APPLICATION_ID);
  g_settings_bind (self->settings, "theme",
                   self, "theme",
                   G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (self->settings, "font-scale",
                   self, "font-scale",
                   G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (self->settings, "scrollback-lines",
                   self, "scrollback-lines",
                   G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (self->settings, "audible-bell",
                   self, "audible-bell",
                   G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (self->settings, "use-custom-font",
                   self, "use-custom-font",
                   G_SETTINGS_BIND_DEFAULT);
  g_settings_bind_with_mapping (self->settings, "custom-font",
                                self, "custom-font",
                                G_SETTINGS_BIND_DEFAULT,
                                custom_font_get_mapping,
                                custom_font_set_mapping,
                                NULL, NULL);

  g_signal_connect (self->settings,
                    "changed::restore-window-size",
                    G_CALLBACK (restore_window_size_changed),
                    self);

  self->desktop_interface = g_settings_new (DESKTOP_INTERFACE_SETTINGS_SCHEMA);
  g_signal_connect (self->desktop_interface,
                    "changed::" MONOSPACE_FONT_KEY_NAME,
                    G_CALLBACK (font_changed),
                    self);
}


/**
 * kgx_settings_get_font:
 * @self: the #KgxSettings
 *
 * Creates a #PangoFontDescription for the system monospace font.
 *
 * Returns: (transfer full): a new #PangoFontDescription
 */
PangoFontDescription *
kgx_settings_get_font (KgxSettings *self)
{
  /* Adapted from gnome-terminal */
  g_autofree char *font = NULL;

  g_return_val_if_fail (KGX_IS_SETTINGS (self), NULL);

  font = g_settings_get_string (self->desktop_interface,
                                MONOSPACE_FONT_KEY_NAME);

  return pango_font_description_from_string (font);
}


void
kgx_settings_increase_scale (KgxSettings *self)
{
  g_return_if_fail (KGX_IS_SETTINGS (self));

  update_scale (self, self->scale + 0.1);
}


void
kgx_settings_decrease_scale (KgxSettings *self)
{
  g_return_if_fail (KGX_IS_SETTINGS (self));

  update_scale (self, self->scale - 0.1);
}


void
kgx_settings_reset_scale (KgxSettings *self)
{
  g_return_if_fail (KGX_IS_SETTINGS (self));

  update_scale (self, KGX_FONT_SCALE_DEFAULT);
}


/**
 * Return: (transfer full):
 */
GStrv
kgx_settings_get_shell (KgxSettings *self)
{
  g_autofree char *user_shell = NULL;
  g_auto (GStrv) shell = NULL;
  g_auto (GStrv) custom_shell = NULL;

  g_return_val_if_fail (KGX_IS_SETTINGS (self), NULL);

  custom_shell = g_settings_get_strv (self->settings, "shell");

  if (g_strv_length (custom_shell) > 0) {
    return g_steal_pointer (&custom_shell);
  }

  user_shell = vte_get_user_shell ();

  if (G_LIKELY (user_shell)) {
    shell = g_new0 (char *, 2);
    shell[0] = g_steal_pointer (&user_shell);
    shell[1] = NULL;

    return g_steal_pointer (&shell);
  }

  /* We could probably do something other than /bin/sh */
  shell = g_new0 (char *, 2);
  shell[0] = g_strdup ("/bin/sh");
  shell[1] = NULL;
  g_warning ("No Shell! Defaulting to “%s”", shell[0]);

  return g_steal_pointer (&shell);
}


void
kgx_settings_set_custom_shell (KgxSettings *self, const char *const *shell)
{
  g_return_if_fail (KGX_IS_SETTINGS (self));

  g_settings_set_strv (self->settings, "shell", shell);
}


void
kgx_settings_set_scrollback (KgxSettings *self,
                             int64_t      value)
{
  g_return_if_fail (KGX_IS_SETTINGS (self));

  if (self->scrollback_lines == value) {
    return;
  }

  self->scrollback_lines = value;

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_SCROLLBACK_LINES]);
}


void
kgx_settings_get_size (KgxSettings *self,
                       int         *width,
                       int         *height)
{
  g_return_if_fail (KGX_IS_SETTINGS (self));
  g_return_if_fail (width != NULL && height != NULL);

  if (!g_settings_get_boolean (self->settings, RESTORE_SIZE_KEY)) {
    *width = -1;
    *height = -1;
    return;
  }

  g_settings_get (self->settings, LAST_SIZE_KEY, "(ii)", width, height);
}


void
kgx_settings_set_custom_size (KgxSettings *self,
                              int          width,
                              int          height)
{
  g_return_if_fail (KGX_IS_SETTINGS (self));

  if (!g_settings_get_boolean (self->settings, RESTORE_SIZE_KEY)) {
    return;
  }

  g_debug ("Store window size: %i×%i", width, height);

  g_settings_set (self->settings, LAST_SIZE_KEY, "(ii)", width, height);
}


gboolean
kgx_settings_get_audible_bell (KgxSettings *self)
{
  g_return_val_if_fail (KGX_IS_SETTINGS (self), FALSE);

  return self->audible_bell;
}


void
kgx_settings_set_audible_bell (KgxSettings *self,
                               gboolean     audible_bell)
{
  g_return_if_fail (KGX_IS_SETTINGS (self));

  if (self->audible_bell == audible_bell)
    return;

  self->audible_bell = audible_bell;

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_AUDIBLE_BELL]);
}


gboolean
kgx_settings_get_use_custom_font (KgxSettings *self)
{
  g_return_val_if_fail (KGX_IS_SETTINGS (self), FALSE);

  return self->use_custom_font;
}


void
kgx_settings_set_use_custom_font (KgxSettings *self,
                                  gboolean     use_custom_font)
{
  g_return_if_fail (KGX_IS_SETTINGS (self));

  if (self->use_custom_font == use_custom_font)
    return;

  self->use_custom_font = use_custom_font;

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_USE_CUSTOM_FONT]);
}


PangoFontDescription *
kgx_settings_get_custom_font (KgxSettings *self)
{
  g_return_val_if_fail (KGX_IS_SETTINGS (self), FALSE);

  return pango_font_description_copy (self->custom_font);
}


void
kgx_settings_set_custom_font (KgxSettings          *self,
                              PangoFontDescription *custom_font)
{
  g_autofree char *font_desc = NULL;

  g_return_if_fail (KGX_IS_SETTINGS (self));
  g_return_if_fail (custom_font != NULL);

  if (self->custom_font == custom_font ||
      (self->custom_font && custom_font &&
       pango_font_description_equal (self->custom_font, custom_font)))
    return;

  if (self->custom_font)
    pango_font_description_free (self->custom_font);

  self->custom_font = pango_font_description_copy (custom_font);

  font_desc = pango_font_description_to_string (custom_font);
  g_settings_set_string (self->settings, CUSTOM_FONT, font_desc);

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_CUSTOM_FONT]);
}
