#pragma once

#include <glib-object.h>

// Taken from GLib 2.62
inline static void
clear_signal_handler (gulong   *handler_id_ptr,
                      gpointer  instance)
{
  g_return_if_fail (handler_id_ptr != NULL);

  if (*handler_id_ptr != 0)
    {
      g_signal_handler_disconnect (instance, *handler_id_ptr);
      *handler_id_ptr = 0;
    }
}
