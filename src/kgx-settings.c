/* kgx-settings.c
 *
 * Copyright 2022-2023 Zander Brown
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
#include <vte/vte.h>
#include <adwaita.h>

#include "kgx-system-info.h"
#include "kgx-marshals.h"

#include "kgx-settings.h"

#define RESTORE_SIZE_KEY "restore-window-size"
#define LAST_SIZE_KEY "last-window-size"
#define LAST_MAXIMISED_KEY "last-window-maximised"

#define AUDIBLE_BELL "audible-bell"

#define CUSTOM_FONT "custom-font"

struct _KgxSettings {
  GObject               parent_instance;

  KgxTheme              theme;
  PangoFontDescription *font;
  double                scale;
  int64_t               scrollback_lines;
  gboolean              audible_bell;
  gboolean              visual_bell;
  gboolean              use_system_font;
  PangoFontDescription *custom_font;

  GSettings            *settings;
  KgxSystemInfo        *system_info;
};


G_DEFINE_TYPE (KgxSettings, kgx_settings, G_TYPE_OBJECT)


enum {
  PROP_0,
  PROP_THEME,
  PROP_RESOLVED_THEME,
  PROP_FONT,
  PROP_FONT_SCALE,
  PROP_SCALE_CAN_INCREASE,
  PROP_SCALE_CAN_DECREASE,
  PROP_SCROLLBACK_LINES,
  PROP_AUDIBLE_BELL,
  PROP_VISUAL_BELL,
  PROP_USE_SYSTEM_FONT,
  PROP_CUSTOM_FONT,
  LAST_PROP
};

static GParamSpec *pspecs[LAST_PROP] = { NULL, };


static void
kgx_settings_dispose (GObject *object)
{
  KgxSettings *self = KGX_SETTINGS (object);

  g_clear_pointer (&self->font, pango_font_description_free);
  g_clear_pointer (&self->custom_font, pango_font_description_free);

  g_clear_object (&self->settings);
  g_clear_object (&self->system_info);

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
      g_object_notify_by_pspec (object, pspecs[PROP_RESOLVED_THEME]);
      break;
    case PROP_FONT:
      g_clear_pointer (&self->font, pango_font_description_free);
      self->font = g_value_dup_boxed (value);
      g_object_notify_by_pspec (object, pspecs[PROP_FONT]);
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
    case PROP_VISUAL_BELL:
      kgx_settings_set_visual_bell (self, g_value_get_boolean (value));
      break;
    case PROP_USE_SYSTEM_FONT:
      kgx_settings_set_use_system_font (self, g_value_get_boolean (value));
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
    case PROP_RESOLVED_THEME:
      g_value_set_enum (value, kgx_settings_get_resolved_theme (self));
      break;
    case PROP_FONT:
      g_value_set_boxed (value, self->font);
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
    case PROP_VISUAL_BELL:
      g_value_set_boolean (value, kgx_settings_get_visual_bell (self));
      break;
    case PROP_USE_SYSTEM_FONT:
      g_value_set_boolean (value, self->use_system_font);
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
    g_param_spec_enum ("theme", NULL, NULL,
                       KGX_TYPE_THEME, KGX_THEME_NIGHT,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_RESOLVED_THEME] =
    g_param_spec_enum ("resolved-theme", NULL, NULL,
                       KGX_TYPE_THEME, KGX_THEME_NIGHT,
                       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_FONT] =
    g_param_spec_boxed ("font", NULL, NULL,
                        PANGO_TYPE_FONT_DESCRIPTION,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  pspecs[PROP_FONT_SCALE] =
    g_param_spec_double ("font-scale", NULL, NULL,
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
    g_param_spec_int64 ("scrollback-lines", NULL, NULL,
                        G_MININT64, G_MAXINT64, 512,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  pspecs[PROP_AUDIBLE_BELL] =
    g_param_spec_boolean ("audible-bell", NULL, NULL,
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  pspecs[PROP_VISUAL_BELL] =
    g_param_spec_boolean ("visual-bell", NULL, NULL,
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  pspecs[PROP_USE_SYSTEM_FONT] =
    g_param_spec_boolean ("use-system-font", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  pspecs[PROP_CUSTOM_FONT] =
    g_param_spec_boxed ("custom-font", NULL, NULL,
                        PANGO_TYPE_FONT_DESCRIPTION,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);
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
decode_font (GValue   *value,
             GVariant *variant,
             gpointer  data)
{
  const char *font_desc = g_variant_get_string (variant, NULL);
  g_autoptr (PangoFontDescription) font = NULL;

  if (!font_desc || g_strcmp0 (font_desc, "") == 0) {
    g_value_set_boxed (value, NULL);

    return TRUE;
  }

  font = pango_font_description_from_string (font_desc);

  if ((pango_font_description_get_set_fields (font) & (PANGO_FONT_MASK_FAMILY | PANGO_FONT_MASK_SIZE)) == 0) {
    g_value_set_boxed (value, NULL);

    g_warning ("settings: ignoring ‘%s’ as lacking family and/or size", font_desc);

    return TRUE;
  }

  g_value_set_boxed (value, font);

  return TRUE;
}


static GVariant *
encode_font (const GValue       *value,
             const GVariantType *variant_ty,
             gpointer            data)
{
  PangoFontDescription *font;

  font = g_value_get_boxed (value);

  return g_variant_new_take_string (pango_font_description_to_string (font));
}

static void
dark_changed (KgxSettings *self)
{
  if (self->theme == KGX_THEME_AUTO) {
    g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_RESOLVED_THEME]);
  }
}


static PangoFontDescription *
resolve_font (GObject              *object,
              gboolean              use_system,
              PangoFontDescription *system,
              PangoFontDescription *custom)
{
  return pango_font_description_copy (use_system ? system : custom);
}


static inline void
setup_font_expression (KgxSettings *self)
{
  GtkExpression *params[] = {
    gtk_property_expression_new (KGX_TYPE_SETTINGS, NULL, "use-system-font"),
    gtk_property_expression_new (KGX_TYPE_SYSTEM_INFO,
                                 gtk_constant_expression_new (KGX_TYPE_SYSTEM_INFO, self->system_info),
                                 "monospace-font"),
    gtk_property_expression_new (KGX_TYPE_SETTINGS, NULL, "custom-font"),
  };
  GtkExpression *exp =
    gtk_cclosure_expression_new (PANGO_TYPE_FONT_DESCRIPTION,
                                 kgx_marshals_BOXED__BOOLEAN_BOXED_BOXED,
                                 G_N_ELEMENTS (params), params,
                                 G_CALLBACK (resolve_font), NULL, NULL);

  gtk_expression_bind (exp, self, "font", self);
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
  g_settings_bind (self->settings, "visual-bell",
                   self, "visual-bell",
                   G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (self->settings, "use-system-font",
                   self, "use-system-font",
                   G_SETTINGS_BIND_DEFAULT);
  g_settings_bind_with_mapping (self->settings, "custom-font",
                                self, "custom-font",
                                G_SETTINGS_BIND_DEFAULT,
                                decode_font,
                                encode_font,
                                NULL, NULL);

  g_signal_connect (self->settings,
                    "changed::" RESTORE_SIZE_KEY,
                    G_CALLBACK (restore_window_size_changed),
                    self);

  self->system_info = g_object_new (KGX_TYPE_SYSTEM_INFO, NULL);
  setup_font_expression (self);

  g_signal_connect_object (adw_style_manager_get_default (),
                           "notify::dark", G_CALLBACK (dark_changed),
                           self, G_CONNECT_SWAPPED);
}

/**
 * kgx_settings_get_resolved_theme:
 * @self: the #KgxSettings
 *
 * Unlike #KgxSettings:theme this is never %KGX_THEME_AUTO, instead providing
 * the value %KGX_THEME_AUTO should be treated as
 */
KgxTheme
kgx_settings_get_resolved_theme (KgxSettings *self)
{
  AdwStyleManager *manager = adw_style_manager_get_default ();

  g_return_val_if_fail (KGX_IS_SETTINGS (self), KGX_THEME_HACKER);

  if (G_LIKELY (self->theme != KGX_THEME_AUTO)) {
    return self->theme;
  }

  if (adw_style_manager_get_dark (manager)) {
    return KGX_THEME_NIGHT;
  }

  return KGX_THEME_DAY;
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
 * kgx_settings_get_shell:
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


gboolean
kgx_settings_get_restore_size (KgxSettings *self)
{
  g_return_val_if_fail (KGX_IS_SETTINGS (self), FALSE);

  return g_settings_get_boolean (self->settings, RESTORE_SIZE_KEY);
}


void
kgx_settings_get_size (KgxSettings *self,
                       int         *width,
                       int         *height,
                       gboolean    *maximised)
{
  g_return_if_fail (KGX_IS_SETTINGS (self));
  g_return_if_fail (width != NULL && height != NULL && maximised != NULL);

  if (!g_settings_get_boolean (self->settings, RESTORE_SIZE_KEY)) {
    *width = -1;
    *height = -1;
    *maximised = FALSE;
    return;
  }

  g_settings_get (self->settings, LAST_SIZE_KEY, "(ii)", width, height);
  g_settings_get (self->settings, LAST_MAXIMISED_KEY, "b", maximised);
}


void
kgx_settings_set_custom_size (KgxSettings *self,
                              int          width,
                              int          height,
                              gboolean     maximised)
{
  g_return_if_fail (KGX_IS_SETTINGS (self));

  if (!g_settings_get_boolean (self->settings, RESTORE_SIZE_KEY)) {
    return;
  }

  g_debug ("settings: store size (%i×%i)", width, height);

  g_settings_set (self->settings, LAST_SIZE_KEY, "(ii)", width, height);
  g_settings_set (self->settings, LAST_MAXIMISED_KEY, "b", maximised);
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
kgx_settings_get_visual_bell (KgxSettings *self)
{
  g_return_val_if_fail (KGX_IS_SETTINGS (self), FALSE);

  return self->visual_bell;
}


void
kgx_settings_set_visual_bell (KgxSettings *self,
                              gboolean     visual_bell)
{
  g_return_if_fail (KGX_IS_SETTINGS (self));

  if (self->visual_bell == visual_bell)
    return;

  self->visual_bell = visual_bell;

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_VISUAL_BELL]);
}


gboolean
kgx_settings_get_use_system_font (KgxSettings *self)
{
  g_return_val_if_fail (KGX_IS_SETTINGS (self), FALSE);

  return self->use_system_font;
}


void
kgx_settings_set_use_system_font (KgxSettings *self,
                                  gboolean     use_system_font)
{
  g_return_if_fail (KGX_IS_SETTINGS (self));

  if (self->use_system_font == use_system_font)
    return;

  self->use_system_font = use_system_font;

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_USE_SYSTEM_FONT]);
}


/**
 * kgx_settings_get_custom_font:
 *
 * Return: (transfer full):
 */
PangoFontDescription *
kgx_settings_get_custom_font (KgxSettings *self)
{
  g_return_val_if_fail (KGX_IS_SETTINGS (self), NULL);

  return pango_font_description_copy (self->custom_font);
}


void
kgx_settings_set_custom_font (KgxSettings          *self,
                              PangoFontDescription *custom_font)
{
  g_return_if_fail (KGX_IS_SETTINGS (self));

  if (self->custom_font == custom_font ||
      (self->custom_font && custom_font &&
       pango_font_description_equal (self->custom_font, custom_font))) {
    return;
  }

  g_clear_pointer (&self->custom_font, pango_font_description_free);
  self->custom_font = pango_font_description_copy (custom_font);

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_CUSTOM_FONT]);
}
