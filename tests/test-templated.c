/* test-templated.c
 *
 * Copyright 2025 Zander Brown
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

#include "kgx-locale.h"
#include "kgx-utils.h"

#include "kgx-templated.h"

#define KGX_TYPE_TEMPLATED_BASE (kgx_templated_base_get_type ())

G_DECLARE_DERIVABLE_TYPE (KgxTemplatedBase, kgx_templated_base, KGX, TEMPLATED_BASE, GObject)


struct _KgxTemplatedBaseClass {
  GObjectClass parent;
};


typedef struct _KgxTemplatedBasePrivate KgxTemplatedBasePrivate;
struct _KgxTemplatedBasePrivate {
  int64_t a;
  int64_t b;
};


G_DEFINE_TYPE_WITH_CODE (KgxTemplatedBase, kgx_templated_base, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (KgxTemplatedBase)
                         G_IMPLEMENT_INTERFACE (KGX_TYPE_TEMPLATED, NULL))


enum {
  PROP_0_BASE,
  PROP_A,
  PROP_B,
  LAST_PROP_BASE
};
static GParamSpec *pspecs_base[LAST_PROP_BASE] = { NULL, };


static void
kgx_templated_base_dispose (GObject *object)
{
  kgx_templated_dispose_template (KGX_TEMPLATED (object),
                                  KGX_TYPE_TEMPLATED_BASE);

  G_OBJECT_CLASS (kgx_templated_base_parent_class)->dispose (object);
}


static void
kgx_templated_base_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  KgxTemplatedBasePrivate *priv =
    kgx_templated_base_get_instance_private (KGX_TEMPLATED_BASE (object));

  switch (property_id) {
    case PROP_A:
      g_value_set_int64 (value, priv->a);
      break;
    case PROP_B:
      g_value_set_int64 (value, priv->b);
      break;
    KGX_INVALID_PROP (object, property_id, pspec);
  }
}


static void
kgx_templated_base_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  KgxTemplatedBasePrivate *priv =
    kgx_templated_base_get_instance_private (KGX_TEMPLATED_BASE (object));

  switch (property_id) {
    case PROP_A:
      kgx_set_int64_prop (object, pspec, &priv->a, value);
      break;
    case PROP_B:
      kgx_set_int64_prop (object, pspec, &priv->b, value);
      break;
    KGX_INVALID_PROP (object, property_id, pspec);
  }
}


static void
kgx_templated_base_class_init (KgxTemplatedBaseClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = kgx_templated_base_dispose;
  object_class->get_property = kgx_templated_base_get_property;
  object_class->set_property = kgx_templated_base_set_property;

  pspecs_base[PROP_A] =
    g_param_spec_int64 ("a", NULL, NULL,
                        G_MININT64, G_MAXINT64, 0,
                        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  pspecs_base[PROP_B] =
    g_param_spec_int64 ("b", NULL, NULL,
                        G_MININT64, G_MAXINT64, 0,
                        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP_BASE, pspecs_base);

  kgx_templated_class_set_template_from_resource (object_class,
                                                  KGX_APPLICATION_PATH "test/base.ui");
}


static void
kgx_templated_base_init (KgxTemplatedBase *self)
{
  kgx_templated_init_template (KGX_TEMPLATED (self));
}


#define KGX_TYPE_UNTEMPLATED_INTERMEDIATE (kgx_untemplated_intermediate_get_type ())

G_DECLARE_DERIVABLE_TYPE (KgxUntemplatedIntermediate, kgx_untemplated_intermediate, KGX, UNTEMPLATED_INTERMEDIATE, GObject)


struct _KgxUntemplatedIntermediateClass {
  KgxTemplatedBaseClass parent;
};


G_DEFINE_TYPE (KgxUntemplatedIntermediate, kgx_untemplated_intermediate, KGX_TYPE_TEMPLATED_BASE)


static void
kgx_untemplated_intermediate_class_init (KgxUntemplatedIntermediateClass *klass)
{

}


static void
kgx_untemplated_intermediate_init (KgxUntemplatedIntermediate *self)
{
}


#define KGX_TYPE_ALSO_TEMPLATED (kgx_also_templated_get_type ())

G_DECLARE_FINAL_TYPE (KgxAlsoTemplated, kgx_also_templated, KGX, ALSO_TEMPLATED, KgxUntemplatedIntermediate)


struct _KgxAlsoTemplated {
  KgxUntemplatedIntermediate parent;

  int64_t c;

  GListStore *list_store;
};


typedef struct _KgxAlsoTemplatedPrivate KgxAlsoTemplatedPrivate;
struct _KgxAlsoTemplatedPrivate {
  GInputStream *input_stream;
};


G_DEFINE_FINAL_TYPE_WITH_CODE (KgxAlsoTemplated, kgx_also_templated, KGX_TYPE_UNTEMPLATED_INTERMEDIATE,
                               G_IMPLEMENT_INTERFACE (KGX_TYPE_TEMPLATED, NULL)
                               G_ADD_PRIVATE (KgxAlsoTemplated))


enum {
  PROP_0_ALSO,
  PROP_C,
  LAST_PROP_ALSO
};
static GParamSpec *pspecs_also[LAST_PROP_ALSO] = { NULL, };


static void
kgx_also_templated_dispose (GObject *object)
{
  kgx_templated_dispose_template (KGX_TEMPLATED (object),
                                  KGX_TYPE_ALSO_TEMPLATED);

  G_OBJECT_CLASS (kgx_also_templated_parent_class)->dispose (object);
}


static void
kgx_also_templated_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  KgxAlsoTemplated *self = KGX_ALSO_TEMPLATED (object);

  switch (property_id) {
    case PROP_C:
      g_value_set_int64 (value, self->c);
      break;
    KGX_INVALID_PROP (object, property_id, pspec);
  }
}


static void
kgx_also_templated_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  KgxAlsoTemplated *self = KGX_ALSO_TEMPLATED (object);

  switch (property_id) {
    case PROP_C:
      kgx_set_int64_prop (object, pspec, &self->c, value);
      break;
    KGX_INVALID_PROP (object, property_id, pspec);
  }
}


static gboolean changed = FALSE;


static void
items_changed (GListModel *model,
               guint       position,
               guint       removed,
               guint       added,
               gpointer    user_data)
{
  KgxAlsoTemplated *self = KGX_ALSO_TEMPLATED (user_data);

  g_assert_nonnull (self);

  changed = TRUE;
}


static void
kgx_also_templated_class_init (KgxAlsoTemplatedClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = kgx_also_templated_dispose;
  object_class->get_property = kgx_also_templated_get_property;
  object_class->set_property = kgx_also_templated_set_property;

  pspecs_also[PROP_C] =
    g_param_spec_int64 ("c", NULL, NULL,
                        G_MININT64, G_MAXINT64, 0,
                        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP_ALSO, pspecs_also);

  kgx_templated_class_set_template_from_resource (object_class,
                                                  KGX_APPLICATION_PATH "test/also.ui");

  kgx_templated_class_bind_template_child (object_class, KgxAlsoTemplated, list_store);
  kgx_templated_class_bind_template_child_private (object_class, KgxAlsoTemplated, input_stream);

  kgx_templated_class_bind_template_callback (object_class, items_changed);
}


static void
kgx_also_templated_init (KgxAlsoTemplated *self)
{
  KgxAlsoTemplatedPrivate *priv =
    kgx_also_templated_get_instance_private (self);
  g_autoptr (GObject) obj = g_object_new (G_TYPE_OBJECT, NULL);

  kgx_templated_init_template (KGX_TEMPLATED (self));

  g_assert_nonnull (self->list_store);
  g_assert_true (G_IS_LIST_STORE (self->list_store));

  g_assert_nonnull (priv->input_stream);
  g_assert_true (G_IS_INPUT_STREAM (priv->input_stream));

  changed = FALSE;

  g_assert_false (changed);
  g_list_store_append (self->list_store, obj);
  g_assert_true (changed);

  g_list_store_append (self->list_store, obj);
  g_list_store_append (self->list_store, obj);
  g_list_store_append (self->list_store, obj);
}


static void
test_templated_base_new_destroy (void)
{
  KgxTemplatedBase *obj = g_object_new (KGX_TYPE_TEMPLATED_BASE, NULL);

  g_assert_finalize_object (obj);
}


static void
test_templated_base_properties (void)
{
  g_autoptr (KgxTemplatedBase) obj =
    g_object_new (KGX_TYPE_TEMPLATED_BASE, NULL);
  int64_t a, b;

  g_object_get (obj, "a", &a, "b", &b, NULL);
  g_assert_cmpint (a, ==, 1);
  g_assert_cmpint (b, ==, 2);
}


static void
test_templated_also_new_destroy (void)
{
  KgxAlsoTemplated *obj = g_object_new (KGX_TYPE_ALSO_TEMPLATED, NULL);

  g_assert_finalize_object (obj);
}


static void
test_templated_also_properties (void)
{
  g_autoptr (KgxTemplatedBase) obj =
    g_object_new (KGX_TYPE_ALSO_TEMPLATED, NULL);
  int64_t a, b, c;

  g_object_get (obj, "a", &a, "b", &b, "c", &c, NULL);
  g_assert_cmpint (a, ==, 1);
  g_assert_cmpint (b, ==, 3);
  g_assert_cmpint (c, ==, 4);
}


static void
test_templated_also_multi_dispose (void)
{
  g_autoptr (KgxAlsoTemplated) obj = g_object_new (KGX_TYPE_ALSO_TEMPLATED, NULL);

  g_object_run_dispose (G_OBJECT (obj));
  g_object_run_dispose (G_OBJECT (obj));
}


static const char *template = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
"<interface>"
"  <template class=\"GBufferedInputStream\">"
"    <property name=\"buffer-size\">200</property>"
"  </template>"
"</interface>";


static void
test_templated_apply (void)
{
  g_autoptr (GtkBuilderScope) scope = gtk_builder_cscope_new ();
  g_autoptr (GBytes) bytes = g_bytes_new_static (template, strlen (template));
  g_autoptr (GInputStream) base = g_memory_input_stream_new ();
  GInputStream *obj =
    g_buffered_input_stream_new_sized (base, 100);

  g_assert_cmpint (g_buffered_input_stream_get_buffer_size (G_BUFFERED_INPUT_STREAM (obj)),
                   ==,
                   100);

  kgx_templated_apply (G_OBJECT (obj), scope, bytes);

  g_assert_cmpint (g_buffered_input_stream_get_buffer_size (G_BUFFERED_INPUT_STREAM (obj)),
                   ==,
                   200);

  g_assert_finalize_object (obj);
}


int
main (int argc, char *argv[])
{
  kgx_locale_init (KGX_LOCALE_TEST);

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/kgx/templated/base/new-destroy", test_templated_base_new_destroy);
  g_test_add_func ("/kgx/templated/base/properties", test_templated_base_properties);
  g_test_add_func ("/kgx/templated/also/new-destroy", test_templated_also_new_destroy);
  g_test_add_func ("/kgx/templated/also/properties", test_templated_also_properties);
  g_test_add_func ("/kgx/templated/also/multi-dispose", test_templated_also_multi_dispose);
  g_test_add_func ("/kgx/templated/apply", test_templated_apply);

  return g_test_run ();
}
