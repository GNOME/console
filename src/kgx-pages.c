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
 * The container of open #KgxTab (uses #HdyTabView internally)
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
  GtkWidget            *view;
  GtkWidget            *tabbar;

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
  PROP_TAB_BAR,
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
      g_value_set_uint (value, hdy_tab_view_get_n_pages (HDY_TAB_VIEW (priv->view)));
      break;
    case PROP_TAB_BAR:
      g_value_set_object (value, priv->tabbar);
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
    case PROP_TAB_BAR:
      g_clear_object (&priv->tabbar);
      priv->tabbar = g_value_dup_object (value);
      if (priv->tabbar) {
        hdy_tab_bar_set_view (HDY_TAB_BAR (priv->tabbar),
                              HDY_TAB_VIEW (priv->view));
      }
      break;
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
size_changed (KgxTab   *tab,
              guint     rows,
              guint     cols,
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
page_changed (GObject *object, GParamSpec *pspec, KgxPages *self)
{
  KgxPagesPrivate *priv;
  HdyTabPage *page = NULL;
  KgxTab *tab;

  priv = kgx_pages_get_instance_private (self);
  page = hdy_tab_view_get_selected_page (HDY_TAB_VIEW (priv->view));
  if (!page)
    return;
  tab = KGX_TAB (hdy_tab_page_get_child (page));


  clear_signal_handler (&priv->size_watcher, priv->active_page);
  priv->size_watcher = g_signal_connect (tab,
                                         "size-changed",
                                         G_CALLBACK (size_changed),
                                         self);

  g_clear_object (&priv->title_bind);
  priv->title_bind = g_object_bind_property (tab,
                                             "tab-title",
                                             self,
                                             "title",
                                             G_BINDING_SYNC_CREATE);

  g_clear_object (&priv->path_bind);
  priv->path_bind = g_object_bind_property (tab,
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
                                                 tab,
                                                 "is-active",
                                                 G_BINDING_SYNC_CREATE);

  g_clear_object (&priv->page_status_bind);
  priv->page_status_bind = g_object_bind_property (tab,
                                                   "tab-status",
                                                   self,
                                                   "status",
                                                   G_BINDING_SYNC_CREATE);

  priv->active_page = KGX_TAB (tab);
}


static void
died (KgxTab         *page,
      GtkMessageType  type,
      const char     *message,
      gboolean        success,
      KgxPages       *self)
{
  KgxPagesPrivate *priv;
  HdyTabPage *tab_page;
  gboolean close_on_quit;
  int tab_count;

  priv = kgx_pages_get_instance_private (self);
  tab_page = hdy_tab_view_get_page (HDY_TAB_VIEW (priv->view), GTK_WIDGET (page));

  g_object_get (page, "close-on-quit", &close_on_quit, NULL);

  if (!close_on_quit) {
    return;
  }

  g_object_get (self, "tab-count", &tab_count, NULL);

  if (tab_count < 1) {
    return;
  }

  hdy_tab_view_close_page (HDY_TAB_VIEW (priv->view), tab_page);
}


static void
page_attached (HdyTabView *view,
               HdyTabPage *page,
               int         position,
               KgxPages   *self)
{
  KgxTab *tab;
  KgxPagesPrivate *priv;

  g_return_if_fail (HDY_IS_TAB_PAGE (page));

  tab = KGX_TAB (hdy_tab_page_get_child (page));

  priv = kgx_pages_get_instance_private (self);

  g_signal_connect (tab, "died", G_CALLBACK (died), self);

  gtk_stack_set_visible_child (GTK_STACK (priv->stack), priv->view);
}


static void
page_detached (HdyTabView *view,
               HdyTabPage *page,
               int         position,
               KgxPages   *self)
{
  KgxTab *tab;
  KgxPagesPrivate *priv;
  GtkWidget *toplevel;

  g_return_if_fail (HDY_IS_TAB_PAGE (page));

  tab = KGX_TAB (hdy_tab_page_get_child (page));

  priv = kgx_pages_get_instance_private (self);

  g_signal_handlers_disconnect_by_data (tab, self);

  if (hdy_tab_view_get_n_pages (HDY_TAB_VIEW (priv->view)) == 0) {
    toplevel = gtk_widget_get_toplevel (GTK_WIDGET (self));
    if (GTK_IS_WINDOW (toplevel))
      {
        gtk_window_close (GTK_WINDOW (toplevel));
      }

    priv->active_page = NULL;
    priv->size_watcher = 0;
  }
}


static HdyTabView *
create_window (HdyTabView *view,
               KgxPages   *self)
{
  GtkWindow *window;
  KgxWindow *new_window;
  GtkApplication *app;
  KgxPages *new_pages;
  KgxPagesPrivate *priv;

  window = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (self)));
  app = gtk_window_get_application (window);

  new_window = g_object_new (KGX_TYPE_WINDOW,
                             "application", app,
                             "close-on-zero", TRUE,
                             "initially-empty", TRUE,
                             NULL);

  new_pages = kgx_window_get_pages (new_window);
  priv = kgx_pages_get_instance_private (new_pages);

  gtk_window_set_position (GTK_WINDOW (new_window), GTK_WIN_POS_MOUSE);
  gtk_window_present (GTK_WINDOW (new_window));

  return HDY_TAB_VIEW (priv->view);
}


static void
kgx_pages_class_init (KgxPagesClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = kgx_pages_get_property;
  object_class->set_property = kgx_pages_set_property;

  /**
   * KgxPages:tab-bar:
   * 
   * The #HdyTabBar
   * 
   * Stability: Private
   * 
   * Since: 0.3.0
   */
  pspecs[PROP_TAB_BAR] =
    g_param_spec_object ("tab-bar", "Tab Bar", "The tab bar",
                         HDY_TYPE_TAB_BAR,
                         G_PARAM_READWRITE);

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
  gtk_widget_class_bind_template_child_private (widget_class, KgxPages, view);
  gtk_widget_class_bind_template_child_private (widget_class, KgxPages, status);

  gtk_widget_class_bind_template_callback (widget_class, page_changed);
  gtk_widget_class_bind_template_callback (widget_class, page_attached);
  gtk_widget_class_bind_template_callback (widget_class, page_detached);
  gtk_widget_class_bind_template_callback (widget_class, create_window);

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
                    KgxTab   *tab)
{
  KgxPagesPrivate *priv;
  HdyTabPage *page;

  g_return_if_fail (KGX_IS_PAGES (self));

  priv = kgx_pages_get_instance_private (self);

  page = hdy_tab_view_add_page (HDY_TAB_VIEW (priv->view), GTK_WIDGET (tab), NULL);
  g_object_bind_property (tab, "tab-title", page, "title", G_BINDING_SYNC_CREATE);
  g_object_bind_property (tab, "tab-tooltip", page, "tooltip", G_BINDING_SYNC_CREATE);
}


void
kgx_pages_remove_page (KgxPages *self,
                       KgxTab   *page)
{
  KgxPagesPrivate *priv;
  HdyTabPage *tab_page;
  
  g_return_if_fail (KGX_IS_PAGES (self));

  priv = kgx_pages_get_instance_private (self);

  if (!page)
    {
      tab_page = hdy_tab_view_get_selected_page (HDY_TAB_VIEW (priv->view));
      hdy_tab_view_close_page (HDY_TAB_VIEW (priv->view), tab_page);
      return;
    }

  g_return_if_fail (KGX_IS_TAB (page));

  tab_page = hdy_tab_view_get_page (HDY_TAB_VIEW (priv->view), GTK_WIDGET (page));
  hdy_tab_view_close_page (HDY_TAB_VIEW (priv->view), tab_page);
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
  HdyTabPage *index;
  
  g_return_if_fail (KGX_IS_PAGES (self));
  g_return_if_fail (KGX_IS_TAB (page));

  priv = kgx_pages_get_instance_private (self);

  index = hdy_tab_view_get_page (HDY_TAB_VIEW (priv->view),
                                 GTK_WIDGET (page));
  
  g_return_if_fail (index != NULL);

  hdy_tab_view_set_selected_page (HDY_TAB_VIEW (priv->view), index);

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
  GListModel *pages;
  HdyTabPage *page;
  int n = 0;

  g_return_val_if_fail (KGX_IS_PAGES (self), KGX_NONE);

  priv = kgx_pages_get_instance_private (self);

  children = g_ptr_array_new_full (10, (GDestroyNotify) kgx_process_unref);

  pages = hdy_tab_view_get_pages (HDY_TAB_VIEW (priv->view));

  while ((page = g_list_model_get_item (pages, n))) {
    g_autoptr (GPtrArray) page_children = NULL;
    
    page_children = kgx_tab_get_children (KGX_TAB (hdy_tab_page_get_child (page)));

    for (int i = 0; i < page_children->len; i++) {
      g_ptr_array_add (children, g_ptr_array_steal_index (page_children, i));
    }

    n++;
    g_clear_object (&page);

    // 2.62: g_ptr_array_extend_and_steal (children, page_children);
  };

  return children;
}
