/* kgx-search-box.c
 *
 * Copyright 2019 Zander Brown
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
 * SECTION:kgx-search-box
 * @title: KgxSearchBox
 * @short_description: The dynamically sized search box
 * 
 * #GtkSearchBar has a weird way of sizing its children which results in a
 * tiny #GtkSearchEntry, we could set #GtkWidget:width-request but then the
 * entry wouldn't go small enough for the
 * [librem5](https://puri.sm/products/librem-5/) and would still be small on
 * in larger windows, sigh
 * 
 * So here we are with a custom widget that does almost nothing but allows
 * the entry to grow up to 500px wide whilst still going as small as possible
 * as the window gets smaller, it also proxies various signals from
 * #GtkSearchEntry up for consumers to listen to
 * 
 * This *must* be inside a #GtkSearchBar, it may still work outside one but
 * that's very much undefined behaviour
 * 
 * Since: 0.1.0
 */

#include <glib/gi18n.h>

#include "kgx-config.h"
#include "kgx-search-box.h"

G_DEFINE_TYPE (KgxSearchBox, kgx_search_box, GTK_TYPE_BOX)

enum {
    NEXT,
    PREVIOUS,
    CHANGED,
    LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = { 0 };

static void
kgx_search_box_get_preferred_width (GtkWidget *widget,
                                    gint      *minimum_width,
                                    gint      *natural_width)
{
  KgxSearchBox *self = KGX_SEARCH_BOX (widget);
  int min = 0;

  GTK_WIDGET_CLASS (kgx_search_box_parent_class)->get_preferred_width (widget,
                                                                       &min,
                                                                       NULL);

  if (minimum_width)
    *minimum_width = min;

  if (natural_width) {
    int request = 400;
    if (self->parent_width < 400) {
      // Nothing like a good, arbitrary, magic value
      request = self->parent_width - 50;
    } else if (G_LIKELY (self->parent_width > 500)) {
      request = 500;
    } else {
      // Effectivly provides a nice transition between states
      request = (self->parent_width - 50) * 0.9;
    }

    // Seems unlikly to happen, but not impossible
    if (G_UNLIKELY (min > request)) {
      *natural_width = min;
    } else {
      *natural_width = request;
    }
  }
}

static void
search_changed (GtkEntry     *entry,
                KgxSearchBox *self)
{
  g_signal_emit (self, signals[CHANGED], 0, kgx_search_box_get_search (self));
}

static void
search_next (gpointer      entry_or_button,
             KgxSearchBox *self)
{
  g_signal_emit (self, signals[NEXT], 0);
}

static void
search_prev (gpointer      entry_or_button,
             KgxSearchBox *self)
{
  g_signal_emit (self, signals[PREVIOUS], 0);
}

static void
kgx_search_box_class_init (KgxSearchBoxClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->get_preferred_width = kgx_search_box_get_preferred_width;

  /**
   * KgxSearchBox::next:
   * 
   * Straight proxy to #GtkSearchEntry::next-match so check that for details
   * (also includes #GtkButton::clicked for the relevent button)
   * 
   * Since: 0.1.0
   */
  signals[NEXT] = g_signal_new ("next",
                                G_TYPE_FROM_CLASS (klass),
                                G_SIGNAL_RUN_FIRST,
                                0, NULL, NULL, NULL,
                                G_TYPE_NONE, 0);

  /**
   * KgxSearchBox::previous:
   * 
   * Straight proxy to #GtkSearchEntry::previous-match so check that for
   * details (also includes #GtkButton::clicked for the relevent button)
   * 
   * Since: 0.1.0
   */
  signals[PREVIOUS] = g_signal_new ("previous",
                                    G_TYPE_FROM_CLASS (klass),
                                    G_SIGNAL_RUN_FIRST,
                                    0, NULL, NULL, NULL,
                                    G_TYPE_NONE, 0);

  /**
   * KgxSearchBox::changed:
   * @self: the #KgxSearchBox
   * @search: The current contents of the #GtkSearchEntry
   * 
   * Proxy to #GtkSearchEntry::search-changed but with the current search
   * as a parameter
   * 
   * Since: 0.1.0
   */
  signals[CHANGED] = g_signal_new ("changed",
                                    G_TYPE_FROM_CLASS (klass),
                                    G_SIGNAL_RUN_FIRST,
                                    0, NULL, NULL, NULL,
                                    G_TYPE_NONE, 1, G_TYPE_STRING);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               RES_PATH "kgx-search-box.ui");

  gtk_widget_class_bind_template_child (widget_class, KgxSearchBox, entry);

  gtk_widget_class_bind_template_callback (widget_class, search_changed);
  gtk_widget_class_bind_template_callback (widget_class, search_next);
  gtk_widget_class_bind_template_callback (widget_class, search_prev);
}

static void
parent_size_changed (GtkWidget    *parent,
                     GdkRectangle *allocation,
                     KgxSearchBox *self)
{
  // Cache the value for get_preferred_width to use later
  self->parent_width = allocation->width;

  // Ask to be reallocated so get_preferred_width can work out how big
  // we should be now the parent has been resized
  gtk_widget_queue_allocate (GTK_WIDGET (self));
}

static void
set_parent (KgxSearchBox *self)
{
  GtkWidget *new_parent = gtk_widget_get_parent (GTK_WIDGET (self));

  // We may not have a valid parent yet (or it's been destroyed)
  if (self->parent && G_IS_OBJECT (self->parent))
    g_signal_handlers_disconnect_by_func (self->parent, parent_size_changed, self);

  // GtkSearchBar has various internal widgets, we dont really care about them
  // and unless some theme has done something really weird they shouldn't
  // effect the available width
  if (new_parent)
    new_parent = gtk_widget_get_ancestor (new_parent, GTK_TYPE_SEARCH_BAR);

  // Listen for the parent to be given a size
  if (new_parent)
    g_signal_connect (new_parent,
                      "size-allocate",
                      G_CALLBACK (parent_size_changed),
                      self);
  
  // Cache the parent
  self->parent = new_parent;
}

static void
kgx_search_box_init (KgxSearchBox *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->parent = NULL;

  // Not even construct properties are set yet thus parent will
  // only be set after we connect to no need for initial sync
  g_signal_connect (self, "notify::parent", G_CALLBACK (set_parent), NULL);
}

/**
 * kgx_search_box_get_search:
 * @self: The #KgxSearchBox
 * 
 * Gets the current search, aka #GtkEntry:text on the #GtkSearchEntry
 * 
 * Since: 0.1.0
 */
const char *
kgx_search_box_get_search (KgxSearchBox *self)
{
  g_return_val_if_fail (KGX_IS_SEARCH_BOX (self), NULL);

  return gtk_entry_get_text (GTK_ENTRY (self->entry));
}

/**
 * kgx_search_box_focus:
 * @self: the #KgxSearchBox
 * 
 * gtk_widget_grab_focus() the #GtkSearchEntry
 * 
 * Since: 0.2.0
 */
void
kgx_search_box_focus (KgxSearchBox *self)
{
  g_return_if_fail (KGX_IS_SEARCH_BOX (self));

  gtk_widget_grab_focus (self->entry);
}
