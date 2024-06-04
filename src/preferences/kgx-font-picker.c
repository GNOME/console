/* kgx-font-picker.c
 *
 * Copyright 2023 Zander Brown
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

#include <pango/pango.h>

#include "kgx-marshals.h"
#include "kgx-font-picker.h"


struct _KgxFontPicker {
  AdwNavigationPage      parent_instance;

  PangoFontDescription  *initial;
  PangoFontDescription  *current;
  PangoFontDescription  *prior;

  GtkSingleSelection    *selection;
};


G_DEFINE_TYPE (KgxFontPicker, kgx_font_picker, ADW_TYPE_NAVIGATION_PAGE)


enum {
  PROP_0,
  PROP_FONT_STORE,
  PROP_INITIAL_FONT,
  PROP_CURRENT_FONT,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };


enum {
  SELECTED,
  N_SIGNALS
};
static guint signals[N_SIGNALS];


static void
kgx_font_picker_dispose (GObject *object)
{
  KgxFontPicker *self = KGX_FONT_PICKER (object);

  g_clear_pointer (&self->initial, pango_font_description_free);
  g_clear_pointer (&self->current, pango_font_description_free);
  g_clear_pointer (&self->prior, pango_font_description_free);

  G_OBJECT_CLASS (kgx_font_picker_parent_class)->dispose (object);
}


static void
kgx_font_picker_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  KgxFontPicker *self = KGX_FONT_PICKER (object);

  switch (property_id) {
    case PROP_INITIAL_FONT:
      g_clear_pointer (&self->initial, pango_font_description_free);
      g_clear_pointer (&self->prior, pango_font_description_free);
      g_clear_pointer (&self->current, pango_font_description_free);
      self->initial = g_value_dup_boxed (value);
      self->prior = g_value_dup_boxed (value);
      self->current = g_value_dup_boxed (value);
      g_object_notify_by_pspec (object, pspecs[PROP_CURRENT_FONT]);
      break;
    case PROP_CURRENT_FONT:
      g_clear_pointer (&self->prior, pango_font_description_free);
      self->prior = self->current;
      self->current = g_value_dup_boxed (value);
      gtk_widget_action_set_enabled (GTK_WIDGET (self),
                                     "picker.select",
                                     self->current != NULL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
kgx_font_picker_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  KgxFontPicker *self = KGX_FONT_PICKER (object);

  switch (property_id) {
    case PROP_FONT_STORE:
      g_value_set_object (value, pango_cairo_font_map_get_default ());
      break;
    case PROP_INITIAL_FONT:
      g_value_set_boxed (value, self->initial);
      break;
    case PROP_CURRENT_FONT:
      g_value_set_boxed (value, self->current);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static PangoFontDescription *
build_description (GObject *object, PangoFontFamily *family, double size)
{
  g_autoptr (PangoFontDescription) desc = NULL;

  if (!family) {
    return NULL;
  }

  desc = pango_font_description_new ();
  pango_font_description_set_family (desc,
                                     pango_font_family_get_name (family));
  pango_font_description_set_size (desc, size * PANGO_SCALE);

  return g_steal_pointer (&desc);
}


static PangoAttrList *
face_as_attributes (GObject *object, PangoFontFamily *family)
{
  g_autoptr (PangoAttrList) list = pango_attr_list_new ();

  if (family) {
    pango_attr_list_insert (list,
                            pango_attr_family_new (pango_font_family_get_name (family)));
  }

  return g_steal_pointer (&list);
}


static PangoAttrList *
font_as_attributes (GObject *object, PangoFontDescription *font)
{
  g_autoptr (PangoAttrList) list = pango_attr_list_new ();

  if (font) {
    pango_attr_list_insert (list, pango_attr_font_desc_new (font));
  }

  return g_steal_pointer (&list);
}


static double
font_as_size (GObject *object, PangoFontDescription *font)
{
  if (!font) {
    return 10.0;
  }

  return pango_font_description_get_size (font) / PANGO_SCALE;
}


static gboolean
pending_as_sensitive (GObject *object, guint pending)
{
  return pending == 0;
}


static void
pending_changed (GObject *obj, GParamSpec *pspec, KgxFontPicker *self)
{
  guint i = 0;
  const char *target_name;
  g_autoptr (PangoFontFamily) family = NULL;

  /* wait for the filter to settle */
  if (gtk_filter_list_model_get_pending (GTK_FILTER_LIST_MODEL (obj)) != 0) {
    return;
  }

  /* we need a prior (or initial) selection, and for nothing to be selected */
  if (!(self->prior && !self->current)) {
    return;
  }

  /* we simply match on strings, and can't do anything without one */
  target_name = pango_font_description_get_family (self->prior);
  if (G_UNLIKELY (!target_name)) {
    return;
  }

  /* walk the model, this works because get_item() is defined to be NULL when
   * reading past the end, but also returns a full ref, hence the use of
   * g_set_object to ensure we don't leak any.
   */
  while (g_set_object (&family, g_list_model_get_item (G_LIST_MODEL (self->selection), i)) && family) {
    if (g_str_equal (target_name, pango_font_family_get_name (family))) {
      /* whew, we found it, select and move on */
      gtk_selection_model_select_item (GTK_SELECTION_MODEL (self->selection),
                                       i,
                                       TRUE);
      break;
    }
    i++;
  }
}


static void
activate (KgxFontPicker *self)
{
  gtk_widget_activate_action (GTK_WIDGET (self), "picker.select", NULL);
}


static char *
preview_text (void)
{
  return g_strdup (pango_language_get_sample_string (NULL));
}


static void
select_activated (GtkWidget  *widget,
                  const char *action_name,
                  GVariant   *parameter)
{
  KgxFontPicker *self = KGX_FONT_PICKER (widget);

  g_signal_emit (self, signals[SELECTED], 0, self->current);
}


static void
kgx_font_picker_class_init (KgxFontPickerClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = kgx_font_picker_dispose;
  object_class->set_property = kgx_font_picker_set_property;
  object_class->get_property = kgx_font_picker_get_property;

  pspecs[PROP_FONT_STORE] =
    g_param_spec_object ("font-store", NULL, NULL,
                         G_TYPE_LIST_MODEL,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_INITIAL_FONT] =
    g_param_spec_boxed ("initial-font", NULL, NULL,
                        PANGO_TYPE_FONT_DESCRIPTION,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_CURRENT_FONT] =
    g_param_spec_boxed ("current-font", NULL, NULL,
                        PANGO_TYPE_FONT_DESCRIPTION,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);

  signals[SELECTED] = g_signal_new ("selected",
                                    G_TYPE_FROM_CLASS (klass),
                                    G_SIGNAL_RUN_LAST,
                                    0, NULL, NULL,
                                    kgx_marshals_VOID__BOXED,
                                    G_TYPE_NONE,
                                    1,
                                    PANGO_TYPE_FONT_DESCRIPTION);
  g_signal_set_va_marshaller (signals[SELECTED],
                              G_TYPE_FROM_CLASS (klass),
                              kgx_marshals_VOID__BOXEDv);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               KGX_APPLICATION_PATH "preferences/kgx-font-picker.ui");

  gtk_widget_class_bind_template_child (widget_class, KgxFontPicker, selection);

  gtk_widget_class_bind_template_callback (widget_class, build_description);
  gtk_widget_class_bind_template_callback (widget_class, face_as_attributes);
  gtk_widget_class_bind_template_callback (widget_class, font_as_attributes);
  gtk_widget_class_bind_template_callback (widget_class, font_as_size);
  gtk_widget_class_bind_template_callback (widget_class, pending_as_sensitive);
  gtk_widget_class_bind_template_callback (widget_class, pending_changed);
  gtk_widget_class_bind_template_callback (widget_class, activate);
  gtk_widget_class_bind_template_callback (widget_class, preview_text);

  gtk_widget_class_install_action (widget_class, "picker.select", NULL, select_activated);
}


static void
kgx_font_picker_init (KgxFontPicker *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
