/* kgx-pages.c
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
 * SECTION:kgx-pages
 * @title: KgxPages
 * @short_description: Container of #KgxTab s
 * 
 * The container of open #KgxTab , through #GtkNotebook it also provides tabs
 * in desktop mode
 * 
 * Since: 0.3.0
 */

#include <glib/gi18n.h>

#include "kgx-config.h"
#include "kgx-pages.h"
#include "kgx-tab.h"
#include "kgx-window.h"
#include "kgx-terminal.h"
#include "util.h"


typedef struct _KgxPagesPrivate KgxPagesPrivate;
struct _KgxPagesPrivate {
  GtkWidget            *stack;
  GtkWidget            *notebook;
  GtkWidget            *empty;

  GtkWidget            *status;

  int                   last_cols;
  int                   last_rows;
  guint                 timeout;

  gulong                size_watcher;

  KgxTab               *active_page;

  char                 *title;
  GBinding             *title_bind;
  GFile                *path;
  GBinding             *path_bind;

  KgxStatus             page_status;
  GBinding             *page_status_bind;

  gboolean              is_active;
  GBinding             *is_active_bind;

  PangoFontDescription *font;
  double                zoom;
  KgxTheme              theme;
  gboolean              opaque;
};


G_DEFINE_TYPE_WITH_PRIVATE (KgxPages, kgx_pages, GTK_TYPE_OVERLAY)


enum {
  PROP_0,
  PROP_TAB_COUNT,
  PROP_TITLE,
  PROP_PATH,
  PROP_THEME,
  PROP_OPAQUE,
  PROP_FONT,
  PROP_ZOOM,
  PROP_IS_ACTIVE,
  PROP_STATUS,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };


enum {
  ZOOM,
  N_SIGNALS
};
static guint signals[N_SIGNALS];


static void
kgx_pages_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  KgxPages *self = KGX_PAGES (object);
  KgxPagesPrivate *priv = kgx_pages_get_instance_private (self);

  switch (property_id) {
    case PROP_TAB_COUNT:
      g_value_set_uint (value, gtk_notebook_get_n_pages (GTK_NOTEBOOK (priv->notebook)));
      break;
    case PROP_TITLE:
      g_value_set_string (value, priv->title);
      break;
    case PROP_PATH:
      g_value_set_object (value, priv->path);
      break;
    case PROP_THEME:
      g_value_set_enum (value, priv->theme);
      break;
    case PROP_OPAQUE:
      g_value_set_boolean (value, priv->opaque);
      break;
    case PROP_FONT:
      g_value_set_boxed (value, priv->font);
      break;
    case PROP_ZOOM:
      g_value_set_double (value, priv->zoom);
      break;
    case PROP_IS_ACTIVE:
      g_value_set_boolean (value, priv->is_active);
      break;
    case PROP_STATUS:
      g_value_set_flags (value, priv->page_status);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
kgx_pages_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  KgxPages *self = KGX_PAGES (object);
  KgxPagesPrivate *priv = kgx_pages_get_instance_private (self);

  switch (property_id) {
    case PROP_TITLE:
      g_clear_pointer (&priv->title, g_free);
      priv->title = g_value_dup_string (value);
      break;
    case PROP_PATH:
      g_clear_object (&priv->path);
      priv->path = g_value_dup_object (value);
      break;
    case PROP_THEME:
      priv->theme = g_value_get_enum (value);
      break;
    case PROP_OPAQUE:
      priv->opaque = g_value_get_boolean (value);
      break;
    case PROP_FONT:
      if (priv->font) {
        g_boxed_free (PANGO_TYPE_FONT_DESCRIPTION, priv->font);
      }
      priv->font = g_value_dup_boxed (value);
      break;
    case PROP_ZOOM:
      priv->zoom = g_value_get_double (value);
      break;
    case PROP_IS_ACTIVE:
      priv->is_active = g_value_get_boolean (value);
      break;
    case PROP_STATUS:
      priv->page_status = g_value_get_flags (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static gboolean
size_timeout (KgxPages *self)
{
  KgxPagesPrivate *priv = kgx_pages_get_instance_private (self);

  priv->timeout = 0;

  gtk_widget_hide (priv->status);

  return G_SOURCE_REMOVE;
}


static void
size_changed (KgxTab  *page,
              guint     cols,
              guint     rows,
              KgxPages *self)
{
  KgxPagesPrivate *priv = kgx_pages_get_instance_private (self);
  g_autofree char *label = NULL;
 
  if (cols == priv->last_cols && rows == priv->last_rows)
    return;

  priv->last_cols = cols;
  priv->last_rows = rows;

  if (gtk_window_is_maximized (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (self))))) {
    // Don't show when maximised as it isn't very interesting
    return;
  }

  if (priv->timeout != 0) {
    g_source_remove (priv->timeout);
  }
  priv->timeout = g_timeout_add (800, G_SOURCE_FUNC (size_timeout), self);

  label = g_strdup_printf ("%i × %i", cols, rows);

  gtk_label_set_label (GTK_LABEL (priv->status), label);
  gtk_widget_show (priv->status);
}


static void
page_changed (GtkNotebook *notebook,
              GtkWidget   *page,
              int          page_num,
              KgxPages    *self)
{
  KgxPagesPrivate *priv = kgx_pages_get_instance_private (self);

  g_return_if_fail (KGX_IS_TAB (page));

  clear_signal_handler (&priv->size_watcher, priv->active_page);
  priv->size_watcher = g_signal_connect (page,
                                         "size-changed",
                                         G_CALLBACK (size_changed),
                                         self);

  g_clear_object (&priv->title_bind);
  priv->title_bind = g_object_bind_property (page,
                                             "tab-title",
                                             self,
                                             "title",
                                             G_BINDING_SYNC_CREATE);

  g_clear_object (&priv->path_bind);
  priv->path_bind = g_object_bind_property (page,
                                            "tab-path",
                                            self,
                                            "path",
                                            G_BINDING_SYNC_CREATE);

  if (priv->active_page) {
    g_object_set (priv->active_page, "is-active", FALSE, NULL);
  }

  g_clear_object (&priv->is_active_bind);
  priv->is_active_bind = g_object_bind_property (self,
                                                 "is-active",
                                                 page,
                                                 "is-active",
                                                 G_BINDING_SYNC_CREATE);

  g_clear_object (&priv->page_status_bind);
  priv->page_status_bind = g_object_bind_property (page,
                                                   "tab-status",
                                                   self,
                                                   "status",
                                                   G_BINDING_SYNC_CREATE);

  priv->active_page = KGX_TAB (page);
}


static void
update_tabs (KgxPages *self)
{
  KgxPagesPrivate *priv = kgx_pages_get_instance_private (self);
  int width = gtk_widget_get_allocated_width (GTK_WIDGET (self));

  if (gtk_notebook_get_n_pages (GTK_NOTEBOOK (priv->notebook)) > 1 &&
      width >= 400) {
    gtk_notebook_set_show_tabs (GTK_NOTEBOOK (priv->notebook), TRUE);
  } else {
    gtk_notebook_set_show_tabs (GTK_NOTEBOOK (priv->notebook), FALSE);
  }

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_TAB_COUNT]);
}


static void
died (KgxTab        *page,
      GtkMessageType  type,
      const char     *message,
      gboolean        success,
      KgxPages       *self)
{
  gboolean close_on_quit;
  int tab_count;

  g_object_get (page, "close-on-quit", &close_on_quit, NULL);

  if (!close_on_quit) {
    return;
  }

  g_object_get (self, "tab-count", &tab_count, NULL);

  if (tab_count < 1) {
    return;
  }

  gtk_widget_destroy (GTK_WIDGET (self));
}


static void
page_added (GtkNotebook *notebook,
            GtkWidget   *page,
            guint        id,
            KgxPages    *self)
{
  KgxPagesPrivate *priv;

  g_return_if_fail (KGX_IS_TAB (page));

  priv = kgx_pages_get_instance_private (self);

  g_signal_connect (page, "died", G_CALLBACK (died), self);
  gtk_notebook_set_tab_detachable (GTK_NOTEBOOK (priv->notebook), page, TRUE);
  gtk_notebook_set_tab_reorderable (GTK_NOTEBOOK (priv->notebook), page, TRUE);

  update_tabs (self);

  gtk_stack_set_visible_child (GTK_STACK (priv->stack), priv->notebook);
}


static void
page_removed (GtkNotebook *notebook,
              GtkWidget   *page,
              guint        id,
              KgxPages    *self)
{
  KgxPagesPrivate *priv;

  g_return_if_fail (KGX_IS_TAB (page));

  priv = kgx_pages_get_instance_private (self);

  g_signal_handlers_disconnect_by_data (page, self);

  update_tabs (self);

  if (gtk_notebook_get_n_pages (GTK_NOTEBOOK (priv->notebook)) == 0) {
    gtk_stack_set_visible_child (GTK_STACK (priv->stack), priv->empty);

    priv->active_page = NULL;
    priv->size_watcher = 0;

    g_object_set (self,
#if IS_GENERIC
                  "title", _("Terminal"),
#else
                  "title", _("King’s Cross"),
#endif
                  "path", NULL,
                  NULL);
  }
}


static void
kgx_pages_class_init (KgxPagesClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = kgx_pages_get_property;
  object_class->set_property = kgx_pages_set_property;

  /**
   * KgxPages:tab-count:
   * 
   * The number of open pages
   * 
   * Stability: Private
   * 
   * Since: 0.3.0
   */
  pspecs[PROP_TAB_COUNT] =
    g_param_spec_uint ("tab-count", "Page Count", "Number of pages",
                       0,
                       G_MAXUINT32,
                       0,
                       G_PARAM_READABLE);

  /**
   * KgxPages:title:
   * 
   * The #KgxTab:tab-title of the current #KgxTab
   * 
   * Note the writability of this property in an implementation detail, DO NOT
   * set this property
   * 
   * Stability: Private
   * 
   * Since: 0.3.0
   */
  pspecs[PROP_TITLE] =
    g_param_spec_string ("title", "Title", "The title of the active page",
                         NULL,
                         G_PARAM_READWRITE);

  /**
   * KgxPages:path:
   * 
   * The #KgxTab:tab-path of the current #KgxTab
   * 
   * Note the writability of this property in an implementation detail, DO NOT
   * set this property
   * 
   * Stability: Private
   * 
   * Since: 0.3.0
   */
  pspecs[PROP_PATH] =
    g_param_spec_object ("path", "Path", "The path of the active page",
                         G_TYPE_FILE,
                         G_PARAM_READWRITE);

  /**
   * KgxPages:theme:
   * 
   * The #KgxTheme to apply to the #KgxTerminal s in the #KgxTab s
   * 
   * Stability: Private
   * 
   * Since: 0.3.0
   */
  pspecs[PROP_THEME] =
    g_param_spec_enum ("theme", "Theme", "The path of the active page",
                       KGX_TYPE_THEME,
                       KGX_THEME_NIGHT,
                       G_PARAM_READWRITE);

  /**
   * KgxPages:opaque:
   * 
   * Whether to disable transparency
   * 
   * Bound to #GtkWindow:is-maximized on the #KgxWindow
   * 
   * Stability: Private
   * 
   * Since: 0.3.0
   */
  pspecs[PROP_OPAQUE] =
    g_param_spec_boolean ("opaque", "Opaque", "Terminal opaqueness",
                          FALSE,
                          G_PARAM_READWRITE);

  pspecs[PROP_FONT] =
    g_param_spec_boxed ("font", "Font", "Monospace font",
                         PANGO_TYPE_FONT_DESCRIPTION,
                         G_PARAM_READWRITE);

  pspecs[PROP_ZOOM] =
    g_param_spec_double ("zoom", "Zoom", "Font scaling",
                         0.5, 2.0, 1.0,
                         G_PARAM_READWRITE);

  pspecs[PROP_IS_ACTIVE] =
    g_param_spec_boolean ("is-active", "Is Active", "Is active pages",
                          FALSE,
                          G_PARAM_READWRITE);

  pspecs[PROP_STATUS] =
    g_param_spec_flags ("status", "Status", "Active page status",
                        KGX_TYPE_STATUS,
                        KGX_NONE,
                        G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);

  signals[ZOOM] = g_signal_new ("zoom",
                                G_TYPE_FROM_CLASS (klass),
                                G_SIGNAL_RUN_LAST,
                                0, NULL, NULL, NULL,
                                G_TYPE_NONE,
                                1,
                                KGX_TYPE_ZOOM);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               RES_PATH "kgx-pages.ui");

  gtk_widget_class_bind_template_child_private (widget_class, KgxPages, stack);
  gtk_widget_class_bind_template_child_private (widget_class, KgxPages, notebook);
  gtk_widget_class_bind_template_child_private (widget_class, KgxPages, empty);
  gtk_widget_class_bind_template_child_private (widget_class, KgxPages, status);

  gtk_widget_class_bind_template_callback (widget_class, page_changed);
  gtk_widget_class_bind_template_callback (widget_class, page_added);
  gtk_widget_class_bind_template_callback (widget_class, page_removed);

  gtk_widget_class_set_css_name (widget_class, "pages");
}


static void
kgx_pages_init (KgxPages *self)
{
  KgxPagesPrivate *priv = kgx_pages_get_instance_private (self);

  priv->font = NULL;
  priv->zoom = 1.0;
  priv->theme = KGX_THEME_NIGHT;
  priv->opaque = FALSE;

  gtk_widget_init_template (GTK_WIDGET (self));
}


void
kgx_pages_focus_terminal (KgxPages *self)
{
  KgxPagesPrivate *priv;
  
  g_return_if_fail (KGX_IS_PAGES (self));

  priv = kgx_pages_get_instance_private (self);

  g_return_if_fail (priv->active_page);

  kgx_tab_focus_terminal (priv->active_page);
}


void
kgx_pages_search_forward (KgxPages *self)
{
  KgxPagesPrivate *priv;
  
  g_return_if_fail (KGX_IS_PAGES (self));

  priv = kgx_pages_get_instance_private (self);

  g_return_if_fail (priv->active_page);

  kgx_tab_search_forward (priv->active_page);
}


void
kgx_pages_search_back (KgxPages *self)
{
  KgxPagesPrivate *priv;
  
  g_return_if_fail (KGX_IS_PAGES (self));

  priv = kgx_pages_get_instance_private (self);

  g_return_if_fail (priv->active_page);

  kgx_tab_search_back (priv->active_page);
}


void
kgx_pages_search (KgxPages   *self,
                  const char *search)
{
  KgxPagesPrivate *priv;
  
  g_return_if_fail (KGX_IS_PAGES (self));

  priv = kgx_pages_get_instance_private (self);

  g_return_if_fail (priv->active_page);

  kgx_tab_search (priv->active_page, search);
}


void
kgx_pages_add_page (KgxPages *self,
                    KgxTab  *page)
{
  KgxPagesPrivate *priv;
  KgxPagesTab *tab;
  
  g_return_if_fail (KGX_IS_PAGES (self));

  priv = kgx_pages_get_instance_private (self);

  tab = g_object_new (KGX_TYPE_PAGES_TAB,
                      "visible", TRUE,
                      NULL);

  gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook),
                            GTK_WIDGET (page),
                            GTK_WIDGET (tab));
}


void
kgx_pages_remove_page (KgxPages *self,
                       KgxTab  *page)
{
  KgxPagesPrivate *priv;
  
  g_return_if_fail (KGX_IS_PAGES (self));
  g_return_if_fail (KGX_IS_TAB (page));

  priv = kgx_pages_get_instance_private (self);
  
  gtk_container_remove (GTK_CONTAINER (priv->notebook), GTK_WIDGET (page));
}


/**
 * kgx_pages_focus_page:
 * @self: the #KgxPages
 * @page: the #KgxTab to focus
 * 
 * Switch to a page
 * 
 * Since: 0.3.0
 */
void
kgx_pages_focus_page (KgxPages *self,
                      KgxTab  *page)
{
  KgxPagesPrivate *priv;
  int index;
  
  g_return_if_fail (KGX_IS_PAGES (self));
  g_return_if_fail (KGX_IS_TAB (page));

  priv = kgx_pages_get_instance_private (self);

  index = gtk_notebook_page_num (GTK_NOTEBOOK (priv->notebook),
                                 GTK_WIDGET (page));
  
  g_return_if_fail (index != -1);

  gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), index);

  kgx_tab_focus_terminal (page);
}


/**
 * kgx_pages_current_status:
 * @self: the #KgxPages
 * 
 * Get the #KgxStatus of the current #KgxTab
 * 
 * Since: 0.3.0
 */
KgxStatus
kgx_pages_current_status (KgxPages *self)
{
  KgxPagesPrivate *priv;
  
  g_return_val_if_fail (KGX_IS_PAGES (self), KGX_NONE);

  priv = kgx_pages_get_instance_private (self);

  return priv->page_status;
}


/**
 * kgx_pages_get_children:
 * @self: the #KgxPages
 * 
 * Call kgx_tab_get_children on all #KgxTab s in @self building a
 * combined list
 * 
 * Returns: (element-type Kgx.Process) (transfer full): the list of #KgxProcess
 */
GPtrArray *
kgx_pages_get_children (KgxPages *self)
{
  KgxPagesPrivate *priv;
  GPtrArray *children;
  GList *pages;

  g_return_val_if_fail (KGX_IS_PAGES (self), KGX_NONE);

  priv = kgx_pages_get_instance_private (self);

  children = g_ptr_array_new_full (10, (GDestroyNotify) kgx_process_unref);

  pages = gtk_container_get_children (GTK_CONTAINER (priv->notebook));

  do {
    g_autoptr (GPtrArray) page_children = NULL;
    
    page_children = kgx_tab_get_children (KGX_TAB (pages->data));

    for (int i = 0; i < page_children->len; i++) {
      g_ptr_array_add (children, g_ptr_array_steal_index (page_children, i));
    }

    // 2.62: g_ptr_array_extend_and_steal (children, page_children);
  } while ((pages = g_list_next (pages)));

  return children;
}
