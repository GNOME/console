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
 * @short_description: Container of #KgxPage s
 * 
 * The container of open #KgxPage , through #GtkNotebook it also provides tabs
 * in desktop mode
 * 
 * Since: 0.3.0
 */

#include <glib/gi18n.h>

#include "kgx-config.h"
#include "kgx-pages.h"
#include "kgx-page.h"
#include "kgx-window.h"
#include "kgx-terminal.h"
#include "util.h"


typedef struct _KgxPagesPrivate KgxPagesPrivate;
struct _KgxPagesPrivate {
  GtkWidget            *notebook;
  GtkWidget            *status;

  int                   last_cols;
  int                   last_rows;
  guint                 timeout;

  gulong                size_watcher;

  KgxPage              *active_page;

  char                 *title;
  GBinding             *title_bind;
  GFile                *path;
  GBinding             *path_bind;

  PangoFontDescription *font;
  double                zoom;
  KgxTheme              theme;
  gboolean              opaque;
};


G_DEFINE_TYPE_WITH_PRIVATE (KgxPages, kgx_pages, GTK_TYPE_OVERLAY)


enum {
  PROP_0,
  PROP_PAGE_COUNT,
  PROP_TITLE,
  PROP_PATH,
  PROP_THEME,
  PROP_OPAQUE,
  PROP_FONT,
  PROP_ZOOM,
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
    case PROP_PAGE_COUNT:
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
size_changed (KgxPage  *page,
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

  g_return_if_fail (KGX_IS_PAGE (page));

  clear_signal_handler (&priv->size_watcher, priv->active_page);
  priv->size_watcher = g_signal_connect (page,
                                         "size-changed",
                                         G_CALLBACK (size_changed),
                                         self);

  g_clear_object (&priv->title_bind);
  priv->title_bind = g_object_bind_property (page,
                                             "page-title",
                                             self,
                                             "title",
                                             G_BINDING_SYNC_CREATE);

  g_clear_object (&priv->path_bind);
  priv->path_bind = g_object_bind_property (page,
                                            "page-path",
                                            self,
                                            "path",
                                            G_BINDING_SYNC_CREATE);

  priv->active_page = KGX_PAGE (page);

  g_message ("Change page");
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

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_PAGE_COUNT]);
}


static void
page_added (GtkNotebook *notebook,
            GtkWidget   *page,
            guint        id,
            KgxPages    *self)
{
  g_return_if_fail (KGX_IS_PAGE (page));

  update_tabs (self);
}


static void
page_removed (GtkNotebook *notebook,
              GtkWidget   *page,
              guint        id,
              KgxPages    *self)
{
  g_return_if_fail (KGX_IS_PAGE (page));

  update_tabs (self);
}


static void
kgx_pages_class_init (KgxPagesClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = kgx_pages_get_property;
  object_class->set_property = kgx_pages_set_property;

  /**
   * KgxPages:page-count:
   * 
   * The number of open pages
   * 
   * Stability: Private
   * 
   * Since: 0.3.0
   */
  pspecs[PROP_PAGE_COUNT] =
    g_param_spec_uint ("page-count", "Page Count", "Number of pages",
                       0,
                       G_MAXUINT32,
                       0,
                       G_PARAM_READABLE);

  /**
   * KgxPages:title:
   * 
   * The #KgxPage:page-title of the current #KgxPage
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
   * The #KgxPage:page-path of the current #KgxPage
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
   * The #KgxTheme to apply to the #KgxTerminal s in the #KgxPage s
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

  gtk_widget_class_bind_template_child_private (widget_class, KgxPages, notebook);
  gtk_widget_class_bind_template_child_private (widget_class, KgxPages, status);

  gtk_widget_class_bind_template_callback (widget_class, page_changed);
  gtk_widget_class_bind_template_callback (widget_class, page_added);
  gtk_widget_class_bind_template_callback (widget_class, page_removed);
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

  kgx_page_focus_terminal (priv->active_page);
}


void
kgx_pages_search_forward (KgxPages *self)
{
  KgxPagesPrivate *priv;
  
  g_return_if_fail (KGX_IS_PAGES (self));

  priv = kgx_pages_get_instance_private (self);

  g_return_if_fail (priv->active_page);

  kgx_page_search_forward (priv->active_page);
}


void
kgx_pages_search_back (KgxPages *self)
{
  KgxPagesPrivate *priv;
  
  g_return_if_fail (KGX_IS_PAGES (self));

  priv = kgx_pages_get_instance_private (self);

  g_return_if_fail (priv->active_page);

  kgx_page_search_back (priv->active_page);
}


void
kgx_pages_search (KgxPages   *self,
                  const char *search)
{
  KgxPagesPrivate *priv;
  
  g_return_if_fail (KGX_IS_PAGES (self));

  priv = kgx_pages_get_instance_private (self);

  g_return_if_fail (priv->active_page);

  kgx_page_search (priv->active_page, search);
}


void
kgx_pages_add_page (KgxPages *self,
                    KgxPage  *page)
{
  KgxPagesPrivate *priv;
  KgxPagesTab *tab;
  
  g_return_if_fail (KGX_IS_PAGES (self));

  priv = kgx_pages_get_instance_private (self);

  tab = g_object_new (KGX_TYPE_PAGES_TAB,
                      "visible", TRUE,
                      NULL);
  kgx_page_connect_tab (page, tab);

  gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook),
                            GTK_WIDGET (page),
                            GTK_WIDGET (tab));
}
