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

#include "kgx-livery-manager.h"
#include "kgx-livery.h"
#include "kgx-marshals.h"
#include "kgx-system-info.h"
#include "kgx-templated.h"
#include "kgx-utils.h"

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
  int                   scrollback_lines;
  gboolean              audible_bell;
  gboolean              visual_bell;
  gboolean              use_system_font;
  PangoFontDescription *custom_font;
  int64_t               scrollback_limit;
  gboolean              ignore_scrollback_limit;
  gboolean              software_flow_control;
  KgxLivery            *livery;
  gboolean              transparency;

  KgxLiveryManager     *livery_manager;

  GSettings            *settings;
  KgxSystemInfo        *system_info;
};


G_DEFINE_FINAL_TYPE_WITH_CODE (KgxSettings, kgx_settings, G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (KGX_TYPE_TEMPLATED, NULL))


enum {
  PROP_0,
  PROP_THEME,
  PROP_FONT,
  PROP_FONT_SCALE,
  PROP_SCALE_CAN_INCREASE,
  PROP_SCALE_CAN_DECREASE,
  PROP_SCROLLBACK_LINES,
  PROP_AUDIBLE_BELL,
  PROP_VISUAL_BELL,
  PROP_USE_SYSTEM_FONT,
  PROP_CUSTOM_FONT,
  PROP_SCROLLBACK_LIMIT,
  PROP_IGNORE_SCROLLBACK_LIMIT,
  PROP_SOFTWARE_FLOW_CONTROL,
  PROP_LIVERY,
  PROP_TRANSPARENCY,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };


static void
kgx_settings_dispose (GObject *object)
{
  KgxSettings *self = KGX_SETTINGS (object);

  kgx_templated_dispose_template (KGX_TEMPLATED (object), KGX_TYPE_SETTINGS);

  g_clear_pointer (&self->font, pango_font_description_free);
  g_clear_pointer (&self->custom_font, pango_font_description_free);

  g_clear_pointer (&self->custom_font, pango_font_description_free);
  g_clear_pointer (&self->livery, kgx_livery_unref);

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
      g_object_notify_by_pspec (object, pspec);
      break;
    case PROP_FONT:
      g_clear_pointer (&self->font, pango_font_description_free);
      self->font = g_value_dup_boxed (value);
      g_object_notify_by_pspec (object, pspec);
      break;
    case PROP_FONT_SCALE:
      update_scale (self, g_value_get_double (value));
      break;
    case PROP_SCROLLBACK_LINES:
      {
        int new_value = g_value_get_int (value);

        if (new_value != self->scrollback_lines) {
          self->scrollback_lines = new_value;
          g_object_notify_by_pspec (object, pspec);
        }
      }
      break;
    case PROP_AUDIBLE_BELL:
      kgx_set_boolean_prop (object,
                            pspec,
                            &self->audible_bell,
                            value);
      break;
    case PROP_VISUAL_BELL:
      kgx_set_boolean_prop (object,
                            pspec,
                            &self->visual_bell,
                            value);
      break;
    case PROP_USE_SYSTEM_FONT:
      kgx_set_boolean_prop (object,
                            pspec,
                            &self->use_system_font,
                            value);
      break;
    case PROP_CUSTOM_FONT:
      kgx_settings_set_custom_font (self, g_value_get_boxed (value));
      break;
    case PROP_SCROLLBACK_LIMIT:
      kgx_settings_set_scrollback_limit (self, g_value_get_int64 (value));
      break;
    case PROP_IGNORE_SCROLLBACK_LIMIT:
      kgx_set_boolean_prop (object,
                            pspec,
                            &self->ignore_scrollback_limit,
                            value);
      break;
    case PROP_SOFTWARE_FLOW_CONTROL:
      kgx_set_boolean_prop (object,
                            pspec,
                            &self->software_flow_control,
                            value);
      break;
    case PROP_TRANSPARENCY:
      kgx_set_boolean_prop (object,
                            pspec,
                            &self->transparency,
                            value);
      break;
    case PROP_LIVERY:
      kgx_settings_set_livery (self, g_value_get_boxed (value));
      break;
    KGX_INVALID_PROP (object, property_id, pspec);
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
      g_value_set_int (value, self->scrollback_lines);
      break;
    case PROP_AUDIBLE_BELL:
      g_value_set_boolean (value, kgx_settings_get_audible_bell (self));
      break;
    case PROP_VISUAL_BELL:
      g_value_set_boolean (value, kgx_settings_get_visual_bell (self));
      break;
    case PROP_USE_SYSTEM_FONT:
      g_value_set_boolean (value, self->use_system_font);
      break;
    case PROP_CUSTOM_FONT:
      g_value_take_boxed (value, kgx_settings_dup_custom_font (self));
      break;
    case PROP_SCROLLBACK_LIMIT:
      g_value_set_int64 (value, self->scrollback_limit);
      break;
    case PROP_IGNORE_SCROLLBACK_LIMIT:
      g_value_set_boolean (value, self->ignore_scrollback_limit);
      break;
    case PROP_SOFTWARE_FLOW_CONTROL:
      g_value_set_boolean (value, kgx_settings_get_software_flow_control (self));
      break;
    case PROP_LIVERY:
      g_value_set_boxed (value, kgx_settings_get_livery (self));
      break;
    case PROP_TRANSPARENCY:
      g_value_set_boolean (value, self->transparency);
      break;
    KGX_INVALID_PROP (object, property_id, pspec);
  }
}


static int
resolve_lines (GObject  *object,
               gboolean  ignore_limit,
               int64_t   limit)
{
  return CLAMP (ignore_limit ? -1 : limit, -1, G_MAXINT);
}


static PangoFontDescription *
resolve_font (GObject              *object,
              gboolean              use_system,
              PangoFontDescription *system,
              PangoFontDescription *custom)
{
  if (!use_system && custom) {
    return pango_font_description_copy (custom);
  }

  return pango_font_description_copy (system);
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
   * Bound to ‘theme’ GSetting so changes persist
   */
  pspecs[PROP_THEME] =
    g_param_spec_enum ("theme", NULL, NULL,
                       KGX_TYPE_THEME, KGX_THEME_NIGHT,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

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
    g_param_spec_int ("scrollback-lines", NULL, NULL,
                      -1, G_MAXINT, 10000,
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

  pspecs[PROP_SCROLLBACK_LIMIT] =
    g_param_spec_int64 ("scrollback-limit", NULL, NULL,
                        G_MININT64, G_MAXINT64, 10000,
                        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_IGNORE_SCROLLBACK_LIMIT] =
    g_param_spec_boolean ("ignore-scrollback-limit", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_SOFTWARE_FLOW_CONTROL] =
    g_param_spec_boolean ("software-flow-control", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_LIVERY] =
    g_param_spec_boxed ("livery", NULL, NULL,
                        KGX_TYPE_LIVERY,
                        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_TRANSPARENCY] =
    g_param_spec_boolean ("transparency", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);

  g_type_ensure (KGX_TYPE_SYSTEM_INFO);
  g_type_ensure (KGX_TYPE_LIVERY_MANAGER);

  kgx_templated_class_set_template_from_resource (object_class,
                                                  KGX_APPLICATION_PATH "kgx-settings.ui");

  kgx_templated_class_bind_template_child (object_class, KgxSettings, livery_manager);
  kgx_templated_class_bind_template_child (object_class, KgxSettings, settings);

  /* We don't interact with this from C but GtkExpression doesn't keep it
   * alive on it's own */
  kgx_templated_class_bind_template_child (object_class, KgxSettings, system_info);

  kgx_templated_class_bind_template_callback (object_class, resolve_lines);
  kgx_templated_class_bind_template_callback (object_class, resolve_font);
}


static gboolean
variant_to_value (GValue *value, GVariant *variant, gpointer data)
{
  g_value_set_variant (value, variant);

  return TRUE;
}


static GVariant *
value_to_variant (const GValue       *value,
                  const GVariantType *variant_ty,
                  gpointer            data)
{
  return g_value_dup_variant (value);
}


static gboolean
resolve_livery (GValue   *value,
                GVariant *variant,
                gpointer  data)
{
  KgxSettings *self = data;
  const char *uuid = g_variant_get_string (variant, NULL);
  KgxLivery *livery = kgx_livery_manager_resolve (self->livery_manager, uuid);

  if (!livery) {
    g_log_structured (G_LOG_DOMAIN,
                      G_LOG_LEVEL_WARNING,
                      "CODE_FILE", __FILE__,
                      "CODE_LINE", G_STRINGIFY (__LINE__),
                      "CODE_FUNC", G_STRFUNC,
                      "KGX_LIVERY_UUID", uuid,
                      "MESSAGE", "settings: ‘%s’ is unknown, using default livery",
                      uuid);

    livery = kgx_livery_manager_dup_fallback (self->livery_manager);
  }

  g_value_take_boxed (value, livery);

  return TRUE;
}


static GVariant *
select_livery (const GValue       *value,
               const GVariantType *variant_ty,
               gpointer            data)
{
  KgxLivery *livery = g_value_get_boxed (value);

  if (!livery) {
    return NULL;
  }

  return g_variant_new_string (kgx_livery_get_uuid (livery));
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

  if (!kgx_str_non_empty (font_desc)) {
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
  PangoFontDescription *font = g_value_get_boxed (value);

  if (!font) {
    return g_variant_new_string ("");
  }

  return g_variant_new_take_string (pango_font_description_to_string (font));
}


static void
kgx_settings_init (KgxSettings *self)
{
  kgx_templated_init_template (KGX_TEMPLATED (self));

  g_settings_bind (self->settings, "theme",
                   self, "theme",
                   G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (self->settings, "font-scale",
                   self, "font-scale",
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
  g_settings_bind (self->settings, "scrollback-lines",
                   /* Yes, this is intentional */
                   self, "scrollback-limit",
                   G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (self->settings, "ignore-scrollback-limit",
                   self, "ignore-scrollback-limit",
                   G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (self->settings, "software-flow-control",
                   self, "software-flow-control",
                   G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (self->settings, "transparency",
                   self, "transparency",
                   G_SETTINGS_BIND_DEFAULT);
  g_settings_bind_with_mapping (self->settings, "custom-liveries",
                                self->livery_manager, "custom-liveries",
                                G_SETTINGS_BIND_DEFAULT, variant_to_value,
                                value_to_variant,
                                NULL, NULL);
  g_settings_bind_with_mapping (self->settings, "livery",
                                self, "livery",
                                G_SETTINGS_BIND_DEFAULT,
                                resolve_livery,
                                select_livery,
                                self, NULL);

  g_signal_connect (self->settings,
                    "changed::" RESTORE_SIZE_KEY,
                    G_CALLBACK (restore_window_size_changed),
                    self);
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
kgx_settings_set_scrollback_limit (KgxSettings *self,
                                   int64_t      value)
{
  g_return_if_fail (KGX_IS_SETTINGS (self));

  if (self->scrollback_limit == value) {
    return;
  }

  self->scrollback_limit = value;

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_SCROLLBACK_LIMIT]);
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


gboolean
kgx_settings_get_visual_bell (KgxSettings *self)
{
  g_return_val_if_fail (KGX_IS_SETTINGS (self), FALSE);

  return self->visual_bell;
}


/**
 * kgx_settings_dup_custom_font:
 *
 * Return: (transfer full):
 */
PangoFontDescription *
kgx_settings_dup_custom_font (KgxSettings *self)
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


gboolean
kgx_settings_get_software_flow_control (KgxSettings *self)
{
  g_return_val_if_fail (KGX_IS_SETTINGS (self), FALSE);

  return self->software_flow_control;
}


KgxLivery *
kgx_settings_get_livery (KgxSettings *self)
{
  g_return_val_if_fail (KGX_IS_SETTINGS (self), NULL);

  return self->livery;
}


void
kgx_settings_set_livery (KgxSettings *restrict self,
                         KgxLivery   *restrict livery)
{
  g_return_if_fail (KGX_IS_SETTINGS (self));

  if (kgx_set_livery (&self->livery, livery)) {
    g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_LIVERY]);
  }
}


/**
 * kgx_settings_get_resolved_theme:
 * @self: the #KgxSettings
 * @dark_environment: if the environment is dark
 *
 * Unlike #KgxSettings:theme this is never %KGX_THEME_AUTO, instead providing
 * the value %KGX_THEME_AUTO should be treated as
 */
KgxTheme
kgx_settings_resolve_theme (KgxSettings *self, gboolean dark_environment)
{
  g_return_val_if_fail (KGX_IS_SETTINGS (self), KGX_THEME_HACKER);

  if (G_LIKELY (self->theme != KGX_THEME_AUTO)) {
    if (G_UNLIKELY (self->theme == KGX_THEME_HACKER)) {
      return KGX_THEME_NIGHT;
    }

    return self->theme;
  }

  if (dark_environment) {
    return KGX_THEME_NIGHT;
  }

  return KGX_THEME_DAY;
}
