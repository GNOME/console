/* kgx-settings.h
 *
 * Copyright 2022-2024 Zander Brown
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

#pragma once

#include <glib-object.h>
#include <pango/pango.h>

#include "kgx-enums.h"
#include "kgx-livery.h"

G_BEGIN_DECLS


/**
 * KgxTheme:
 * @KGX_THEME_AUTO: Matches %KGX_THEME_DAY or %KGX_THEME_NIGHT depending on
 *                  the user's global setting
 * @KGX_THEME_NIGHT: The default, dark, theme
 * @KGX_THEME_DAY: Alternate, light, theme
 * @KGX_THEME_HACKER: Legacy alias for night
 *
 * Until [meson#1687](https://github.com/mesonbuild/meson/issues/1687) is
 * resolved this enum must be manually kept in sync with
 * the ‘Theme’ enum in our gschema
 */
typedef enum /*< enum,prefix=KGX >*/ {
  KGX_THEME_AUTO = 0,   /*< nick=auto >*/
  KGX_THEME_NIGHT = 1,  /*< nick=night >*/
  KGX_THEME_DAY = 2,    /*< nick=day >*/
  KGX_THEME_HACKER = 3, /*< nick=hacker >*/
} KgxTheme;


/**
 * KGX_FONT_SCALE_MIN:
 * The smallest font scale/zoom
 */
#define KGX_FONT_SCALE_MIN 0.5


/**
 * KGX_FONT_SCALE_MAX:
 * The largest font scale/zoom
 */
#define KGX_FONT_SCALE_MAX 4.0


/**
 * KGX_FONT_SCALE_DEFAULT:
 * The standard font scale/zoom
 */
#define KGX_FONT_SCALE_DEFAULT 1.0


#define KGX_TYPE_SETTINGS kgx_settings_get_type ()

G_DECLARE_FINAL_TYPE (KgxSettings, kgx_settings, KGX, SETTINGS, GObject)

void                  kgx_settings_increase_scale    (KgxSettings       *self);
void                  kgx_settings_decrease_scale    (KgxSettings       *self);
void                  kgx_settings_reset_scale       (KgxSettings       *self);
GStrv                 kgx_settings_get_shell         (KgxSettings       *self);
void                  kgx_settings_set_custom_shell  (KgxSettings       *self,
                                                      const char *const *shell);
void                  kgx_settings_set_scrollback_limit (KgxSettings       *self,
                                                         int64_t            value);
gboolean              kgx_settings_get_restore_size  (KgxSettings       *self);
void                  kgx_settings_get_size             (KgxSettings       *self,
                                                         int               *width,
                                                         int               *height,
                                                         gboolean          *maximised);
void                  kgx_settings_set_custom_size      (KgxSettings       *self,
                                                         int                width,
                                                         int                height,
                                                         gboolean           maximised);
gboolean              kgx_settings_get_audible_bell  (KgxSettings       *self);
gboolean              kgx_settings_get_visual_bell   (KgxSettings       *self);
PangoFontDescription *kgx_settings_dup_custom_font           (KgxSettings           *self);
void                  kgx_settings_set_custom_font      (KgxSettings           *self,
                                                         PangoFontDescription  *custom_font);
gboolean              kgx_settings_get_software_flow_control (KgxSettings      *self);
KgxLivery            *kgx_settings_get_livery                (KgxSettings           *self);
void                  kgx_settings_set_livery                (KgxSettings           *self,
                                                              KgxLivery             *livery);
KgxTheme              kgx_settings_resolve_theme             (KgxSettings           *self,
                                                              gboolean               dark_environment);

G_END_DECLS
