/* kgx-test-utils.c
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

#include <gio/gio.h>
#include <gobject/gvaluecollector.h>

#include "kgx-utils.h"

#include "kgx-test-utils.h"


GQuark kgx_test_notify_data_quark (void);


G_DEFINE_QUARK (kgx-test-notify-data, kgx_test_notify_data)


struct _NotifyData {
  GPtrArray *expected_properties;
  GPtrArray *got_properties;
  GObject *object;
  gulong handler;
  gboolean no_notify;
};


KGX_DEFINE_DATA (NotifyData, notify_data)


static inline void
notify_data_cleanup (NotifyData *self)
{
  g_clear_pointer (&self->expected_properties, g_ptr_array_unref);
  g_clear_pointer (&self->got_properties, g_ptr_array_unref);
  g_clear_signal_handler (&self->handler, self->object);
  g_clear_weak_pointer (&self->object);
}


static void
handle_notify (GObject *object, GParamSpec *pspec, gpointer user_data)
{
  NotifyData *data =
    g_object_get_qdata (object, kgx_test_notify_data_quark ());
  guint where;

  g_ptr_array_add (data->got_properties, g_strdup (pspec->name));

  if (g_ptr_array_find_with_equal_func (data->expected_properties,
                                        pspec->name,
                                        g_str_equal,
                                        &where)) {
    g_ptr_array_remove_index (data->expected_properties, where);
  }
}


static inline NotifyData *
get_notify_data (gpointer object)
{
  NotifyData *data =
    g_object_get_qdata (object, kgx_test_notify_data_quark ());
  NotifyData *new_data;

  if (data) {
    return data;
  }

  new_data = notify_data_alloc ();
  new_data->expected_properties =
    g_ptr_array_new_null_terminated (5, g_free, TRUE);
  new_data->got_properties =
    g_ptr_array_new_null_terminated (5, g_free, TRUE);
  g_set_weak_pointer (&new_data->object, object);
  new_data->handler =
    g_signal_connect (object, "notify", G_CALLBACK (handle_notify), NULL);

  g_object_set_qdata_full (object,
                           kgx_test_notify_data_quark (),
                           new_data,
                           notify_data_free);

  return new_data;
}


/**
 * kgx_expect_property_notify:
 * @object: (type GObject.Object): the object to watch
 * @property: the property to expect
 *
 * Adds @property to the list of expected property notifications on @object,
 * this can be called multiple times with the same @property to require
 * the property be notified more than once.
 *
 * This cannot be used at the same time as [func@Kgx.expect_no_notify].
 */
void
kgx_expect_property_notify (gpointer object, const char *property)
{
  NotifyData *data = get_notify_data (object);

  g_assert_false (data->no_notify);

  g_ptr_array_add (data->expected_properties, g_strdup (property));
}


/**
 * kgx_expect_no_notify:
 * @object: (type GObject.Object): the object to watch
 *
 * Watches @object with the expectation that *no* notifications are received
 * before the corresponding [func@Kgx.assert_expected_notifies] call.
 *
 * This cannot be used at the same time as [func@Kgx.expect_property_notify].
 */
void
kgx_expect_no_notify (gpointer object)
{
  NotifyData *data = get_notify_data (object);

  g_assert_cmpint (data->expected_properties->len, ==, 0);

  data->no_notify = TRUE;
}


/**
 * kgx_assert_expected_notifies:
 * @object: (type GObject.Object): the watched object
 *
 * If notifications were expected, assert that each of them were received.
 * Note that order isn't enforced, and we ignore an ‘extra’ notifications, we
 * simple assert that the ‘expected’ notifications were fulfilled.
 *
 * Conversely, if we expected *no* notifications, we assert that there were
 * indeed none *whatsoever* on @object.
 */
void
kgx_assert_expected_notifies (gpointer object)
{
  g_autoptr (NotifyData) data =
    g_object_steal_qdata (object, kgx_test_notify_data_quark ());

  g_assert_nonnull (data);

  if (data->no_notify) {
    g_autofree char *notified =
      g_strjoinv (", ", (GStrv) data->got_properties->pdata);

    g_assert_cmpstr (notified, ==, "");
  } else {
    g_autofree char *unnotified =
      g_strjoinv (", ", (GStrv) data->expected_properties->pdata);

    g_assert_cmpstr (unnotified, ==, "");
  }
}


void
kgx_test_property_notify (gpointer object, const char *property, ...)
{
  g_autofree char *signal = g_strdup_printf ("notify::%s", property);
  g_auto (GValue) value_a = G_VALUE_INIT;
  g_auto (GValue) value_b = G_VALUE_INIT;
  g_autofree char *error = NULL;
  GParamSpec *spec;
  va_list args;

  va_start (args, property);

  spec = g_object_class_find_property (G_OBJECT_GET_CLASS (object),
                                       property);
  g_assert_nonnull (spec);

  G_VALUE_COLLECT_INIT (&value_a, spec->value_type, args, 0, &error);
  g_assert_cmpstr (error, ==, NULL);

  G_VALUE_COLLECT_INIT (&value_b, spec->value_type, args, 0, &error);
  g_assert_cmpstr (error, ==, NULL);

  kgx_expect_property_notify (object, property);
  g_object_set_property (object, property, &value_a);
  kgx_assert_expected_notifies (object);

  kgx_expect_no_notify (object);
  g_object_set_property (object, property, &value_a);
  kgx_assert_expected_notifies (object);

  kgx_expect_property_notify (object, property);
  g_object_set_property (object, property, &value_b);
  kgx_assert_expected_notifies (object);

  va_end (args);
}
