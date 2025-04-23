/* kgx-theme-switcher.c
 *
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

#include "kgx-config.h"

#include "kgx-settings.h"
#include "kgx-shared-closures.h"

#include "kgx-theme-switcher.h"


struct _KgxThemeSwitcher {
  AdwBin     parent_instance;

  KgxTheme theme;

  GtkWidget *system_selector;
  GtkWidget *light_selector;
  GtkWidget *dark_selector;
};


G_DEFINE_TYPE (KgxThemeSwitcher, kgx_theme_switcher, ADW_TYPE_BIN)


enum {
  PROP_0,
  PROP_THEME,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };


static void
theme_check_active_changed (KgxThemeSwitcher *self)
{
  KgxTheme theme;

  if (gtk_check_button_get_active (GTK_CHECK_BUTTON (self->system_selector))) {
      theme = KGX_THEME_AUTO;
  } else if (gtk_check_button_get_active (GTK_CHECK_BUTTON (self->light_selector))) {
      theme = KGX_THEME_DAY;
  } else {
      theme = KGX_THEME_NIGHT;
  }

  if (theme == self->theme) {
    return;
  }

  self->theme = theme;
  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_THEME]);
}


static inline void
set_theme (KgxThemeSwitcher *self,
           KgxTheme          theme)
{
  if (self->theme == theme)
    return;

  switch (theme) {
    case KGX_THEME_AUTO:
      gtk_check_button_set_active (GTK_CHECK_BUTTON (self->system_selector), TRUE);
      break;
    case KGX_THEME_DAY:
      gtk_check_button_set_active (GTK_CHECK_BUTTON (self->light_selector), TRUE);
      break;
    case KGX_THEME_NIGHT:
    case KGX_THEME_HACKER:
    default:
      gtk_check_button_set_active (GTK_CHECK_BUTTON (self->dark_selector), TRUE);
      break;
  }

  self->theme = theme;
  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_THEME]);
}


static void
kgx_theme_switcher_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  KgxThemeSwitcher *self = KGX_THEME_SWITCHER (object);

  switch (prop_id) {
    case PROP_THEME:
      g_value_set_enum (value, self->theme);
      break;
    KGX_INVALID_PROP (object, prop_id, pspec);
  }
}


static void
kgx_theme_switcher_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  KgxThemeSwitcher *self = KGX_THEME_SWITCHER (object);

  switch (prop_id) {
    case PROP_THEME:
      set_theme (self, g_value_get_enum (value));
      break;
    KGX_INVALID_PROP (object, prop_id, pspec);
  }
}


static void
kgx_theme_switcher_class_init (KgxThemeSwitcherClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = kgx_theme_switcher_get_property;
  object_class->set_property = kgx_theme_switcher_set_property;

  pspecs[PROP_THEME] =
    g_param_spec_enum ("theme", NULL, NULL,
                       KGX_TYPE_THEME,
                       KGX_THEME_NIGHT,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               KGX_APPLICATION_PATH "kgx-theme-switcher.ui");

  gtk_widget_class_bind_template_child (widget_class, KgxThemeSwitcher, system_selector);
  gtk_widget_class_bind_template_child (widget_class, KgxThemeSwitcher, light_selector);
  gtk_widget_class_bind_template_child (widget_class, KgxThemeSwitcher, dark_selector);

  gtk_widget_class_bind_template_callback (widget_class, theme_check_active_changed);
  gtk_widget_class_bind_template_callback (widget_class, kgx_style_manager_for_display);

  gtk_widget_class_set_css_name (widget_class, "themeswitcher");
  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
}


static void
kgx_theme_switcher_init (KgxThemeSwitcher *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
