/* kgx-pages.c
 *
 * Copyright 2019-2023 Zander Brown
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

#include <glib/gi18n.h>

#include "kgx-close-dialog.h"
#include "kgx-file-closures.h"
#include "kgx-icon-closures.h"
#include "kgx-marshals.h"
#include "kgx-settings.h"
#include "kgx-shared-closures.h"
#include "kgx-tab.h"
#include "kgx-templated.h"
#include "kgx-terminal.h"
#include "kgx-train.h"
#include "kgx-utils.h"

#include "kgx-pages.h"

/**
 * KgxPages:
 *
 * The container of open #KgxTab (uses #AdwTabView internally)
 */
typedef struct _KgxPagesPrivate KgxPagesPrivate;
struct _KgxPagesPrivate {
  KgxSettings          *settings;

  GtkWidget            *view;
  char                 *title;
  GFile                *path;
  KgxTab               *active_page;
  gboolean              is_active;
  KgxStatus             page_status;
  gboolean              search_mode_enabled;
  gboolean              ringing;

  GtkWidget            *status_message;
  GtkWidget            *status_revealer;

  int                   last_cols;
  int                   last_rows;
  guint                 timeout;

  GSignalGroup         *active_page_signals;
  GBindingGroup        *active_page_binds;
  GSignalGroup         *settings_signals;
  GSignalGroup         *style_manager_signals;

  GBinding             *is_active_bind;

  AdwTabPage           *action_page;
};


typedef struct _KgxPagesClassPrivate KgxPagesClassPrivate;
struct _KgxPagesClassPrivate {
  GBytes          *page_expression_data;
  GtkBuilderScope *page_expression_scope;
};


G_DEFINE_TYPE_WITH_CODE (KgxPages, kgx_pages, ADW_TYPE_BIN,
                         G_ADD_PRIVATE (KgxPages)
                         g_type_add_class_private (g_define_type_id, sizeof (KgxPagesClassPrivate)))


enum {
  PROP_0,
  PROP_SETTINGS,
  PROP_TAB_VIEW,
  PROP_TITLE,
  PROP_PATH,
  PROP_ACTIVE_PAGE,
  PROP_IS_ACTIVE,
  PROP_STATUS,
  PROP_SEARCH_MODE_ENABLED,
  PROP_RINGING,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };


enum {
  ZOOM,
  CREATE_TEAROFF_HOST,
  MAYBE_CLOSE_WINDOW,
  N_SIGNALS
};
static guint signals[N_SIGNALS];


static void
kgx_pages_dispose (GObject *object)
{
  KgxPages *self = KGX_PAGES (object);
  KgxPagesPrivate *priv = kgx_pages_get_instance_private (self);

  g_clear_object (&priv->settings);

  g_clear_handle_id (&priv->timeout, g_source_remove);

  g_clear_pointer (&priv->title, g_free);
  g_clear_object (&priv->path);

  g_clear_weak_pointer (&priv->is_active_bind);
  g_clear_weak_pointer (&priv->active_page);

  G_OBJECT_CLASS (kgx_pages_parent_class)->dispose (object);
}


static void
kgx_pages_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  KgxPages *self = KGX_PAGES (object);
  KgxPagesPrivate *priv = kgx_pages_get_instance_private (self);

  switch (property_id) {
    case PROP_SETTINGS:
      g_value_set_object (value, priv->settings);
      break;
    case PROP_TAB_VIEW:
      g_value_set_object (value, priv->view);
      break;
    case PROP_TITLE:
      g_value_set_string (value, priv->title);
      break;
    case PROP_PATH:
      g_value_set_object (value, priv->path);
      break;
    case PROP_ACTIVE_PAGE:
      g_value_set_object (value, priv->active_page);
      break;
    case PROP_IS_ACTIVE:
      g_value_set_boolean (value, priv->is_active);
      break;
    case PROP_STATUS:
      g_value_set_flags (value, priv->page_status);
      break;
    case PROP_SEARCH_MODE_ENABLED:
      g_value_set_boolean (value, priv->search_mode_enabled);
      break;
    case PROP_RINGING:
      g_value_set_boolean (value, priv->ringing);
      break;
    KGX_INVALID_PROP (object, property_id, pspec);
  }
}


static inline void
set_active_page (KgxPages *self, KgxTab *incoming_page)
{
  KgxPagesPrivate *priv = kgx_pages_get_instance_private (self);
  g_autoptr (KgxTab) original_page = NULL;

  /* Before we do anything, grab a reference to the current page (if any) */
  g_set_object (&original_page, priv->active_page);

  /* Update our weak ref point at the new page */
  if (!g_set_weak_pointer (&priv->active_page, incoming_page)) {
    /* The page didn't actually change, do nothing */
    return;
  }

  /* First, disconnect from the old page, if any */
  if (G_LIKELY (priv->is_active_bind)) {
    g_binding_unbind (priv->is_active_bind);
  }
  g_clear_weak_pointer (&priv->is_active_bind);

  /* If we had an old page, set it inactive */
  if (original_page) {
    g_object_set (original_page, "is-active", FALSE, NULL);
  }

  /* If we've got a new page, connect it up */
  if (G_LIKELY (priv->active_page)) {
    GBinding *bind =
      g_object_bind_property (self, "is-active",
                              priv->active_page, "is-active",
                              G_BINDING_SYNC_CREATE);
    /* GBinding confusingly owns itself, it's lifetime being tied to the
      * pair of objects it binds, so we don't get — or need — a full ref */
    g_set_weak_pointer (&priv->is_active_bind, bind);
  }

  /* Phew, now lets update everything else! */
  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_ACTIVE_PAGE]);
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
    case PROP_SETTINGS:
      g_set_object (&priv->settings, g_value_get_object (value));
      break;
    case PROP_TITLE:
      g_clear_pointer (&priv->title, g_free);
      priv->title = g_value_dup_string (value);
      break;
    case PROP_PATH:
      g_set_object (&priv->path, g_value_get_object (value));
      break;
    case PROP_ACTIVE_PAGE:
      set_active_page (self, g_value_get_object (value));
      break;
    case PROP_IS_ACTIVE:
      kgx_set_boolean_prop (object, pspec, &priv->is_active, value);
      break;
    case PROP_STATUS:
      priv->page_status = g_value_get_flags (value);
      break;
    case PROP_SEARCH_MODE_ENABLED:
      kgx_set_boolean_prop (object, pspec, &priv->search_mode_enabled, value);
      break;
    case PROP_RINGING:
      kgx_set_boolean_prop (object, pspec, &priv->ringing, value);
      break;
    KGX_INVALID_PROP (object, property_id, pspec);
  }
}


static void
size_timeout (gpointer data)
{
  KgxPages *self = data;
  KgxPagesPrivate *priv = kgx_pages_get_instance_private (self);

  priv->timeout = 0;

  gtk_revealer_set_reveal_child (GTK_REVEALER (priv->status_revealer), FALSE);
}


static void
size_changed (KgxTab   *tab,
              guint     rows,
              guint     cols,
              KgxPages *self)
{
  KgxPagesPrivate *priv = kgx_pages_get_instance_private (self);
  g_autofree char *label = NULL;

  if (gtk_widget_in_destruction (GTK_WIDGET (self)))
    return;

  if (cols == priv->last_cols && rows == priv->last_rows) {
    return;
  }

  priv->last_cols = cols;
  priv->last_rows = rows;

  if (gtk_window_is_maximized (GTK_WINDOW (gtk_widget_get_root (GTK_WIDGET (self))))) {
    // Don't show when maximised as it isn't very interesting
    return;
  }

  g_clear_handle_id (&priv->timeout, g_source_remove);
  priv->timeout = g_timeout_add_once (800, size_timeout, self);
  g_source_set_name_by_id (priv->timeout, "[kgx] resize label timeout");

  label = g_strdup_printf ("%i × %i", cols, rows);

  gtk_label_set_label (GTK_LABEL (priv->status_message), label);
  gtk_revealer_set_reveal_child (GTK_REVEALER (priv->status_revealer), TRUE);
}


static void
died (KgxTab         *page,
      GtkMessageType  type,
      const char     *message,
      gboolean        success,
      KgxPages       *self)
{
  KgxPagesPrivate *priv = kgx_pages_get_instance_private (self);
  AdwTabPage *tab_page =
    adw_tab_view_get_page (ADW_TAB_VIEW (priv->view), GTK_WIDGET (page));
  int tab_count = adw_tab_view_get_n_pages (ADW_TAB_VIEW (priv->view));
  gboolean close_on_quit;

  g_object_get (page, "close-on-quit", &close_on_quit, NULL);

  if (!(close_on_quit && success) || tab_count < 1) {
    return;
  }

  adw_tab_view_close_page (ADW_TAB_VIEW (priv->view), tab_page);
}


static void
zoom (KgxTab   *tab,
      KgxZoom   dir,
      KgxPages *self)
{
  g_signal_emit (self, signals[ZOOM], 0, dir);
}


static void
page_attached (AdwTabView *view,
               AdwTabPage *page,
               int         position,
               KgxPages   *self)
{
  KgxTab *tab;

  g_return_if_fail (ADW_IS_TAB_PAGE (page));

  tab = KGX_TAB (adw_tab_page_get_child (page));

  g_object_connect (tab,
                    "signal::died", G_CALLBACK (died), self,
                    "signal::zoom", G_CALLBACK (zoom), self,
                    NULL);
}


static void
page_detached (AdwTabView *view,
               AdwTabPage *page,
               int         position,
               KgxPages   *self)
{
  KgxTab *tab;

  g_return_if_fail (ADW_IS_TAB_PAGE (page));

  tab = KGX_TAB (adw_tab_page_get_child (page));

  g_object_disconnect (tab,
                       "any-signal::died", G_CALLBACK (died), self,
                       "any-signal::zoom", G_CALLBACK (zoom), self,
                       NULL);

  if (adw_tab_view_get_n_pages (ADW_TAB_VIEW (view)) == 0) {
    g_signal_emit (self, signals[MAYBE_CLOSE_WINDOW], 0);
  }
}


static AdwTabView *
create_window (AdwTabView *view,
               KgxPages   *self)
{
  KgxPages *new_pages;
  KgxPagesPrivate *priv;

  g_signal_emit (self, signals[CREATE_TEAROFF_HOST], 0, &new_pages);

  priv = kgx_pages_get_instance_private (new_pages);

  return ADW_TAB_VIEW (priv->view);
}


struct _CloseData {
  AdwTabView *view;
  AdwTabPage *page;
};


KGX_DEFINE_DATA (CloseData, close_data)


static inline void
close_data_cleanup (CloseData *self)
{
  g_clear_object (&self->view);
  g_clear_weak_pointer (&self->page);
}


static void
got_close (GObject      *source,
           GAsyncResult *res,
           gpointer      user_data)
{
  g_autoptr (KgxCloseDialog) dialogue = KGX_CLOSE_DIALOG (source);
  g_autoptr (CloseData) data = user_data;
  g_autoptr (GError) error = NULL;
  KgxCloseDialogResult result;

  result = kgx_close_dialog_run_finish (dialogue, res, &error);

  if (G_UNLIKELY (!data->page)) {
    return;
  }

  if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
    /* The tab was cancelled (died), rendering the dialogue moot */
    result = KGX_CLOSE_ANYWAY;
  } else if (error) {
    g_critical ("Unexpected: %s", error->message);
    return;
  }

  adw_tab_view_close_page_finish (ADW_TAB_VIEW (data->view),
                                  data->page,
                                  result == KGX_CLOSE_ANYWAY);
}


static gboolean
close_page (AdwTabView *view,
            AdwTabPage *page,
            KgxPages   *self)
{
  g_autoptr (CloseData) data = close_data_alloc ();
  g_autoptr (GPtrArray) children = NULL;
  g_autoptr (GCancellable) cancellable = NULL;
  KgxCloseDialog *dlg = NULL;
  KgxTab *tab;

  tab = KGX_TAB (adw_tab_page_get_child (page));
  children = kgx_tab_get_children (tab);

  if (children->len < 1) {
    return GDK_EVENT_PROPAGATE; /* Aka no, I don’t want to block closing */
  }

  dlg = g_object_new (KGX_TYPE_CLOSE_DIALOG,
                      "context", KGX_CONTEXT_TAB,
                      "commands", children,
                      NULL);

  /* slightly paranoid */
  g_set_object (&data->view, view);
  g_set_weak_pointer (&data->page, page);

  g_object_get (tab, "cancellable", &cancellable, NULL);

  kgx_close_dialog_run (dlg,
                        GTK_WIDGET (self),
                        cancellable,
                        got_close,
                        g_steal_pointer (&data));

  return GDK_EVENT_STOP; // Block the close
}


static void
setup_menu (AdwTabView *view,
            AdwTabPage *page,
            KgxPages   *self)
{
  KgxPagesPrivate *priv = kgx_pages_get_instance_private (self);

  priv->action_page = page;
}


static char *
fallback_title (G_GNUC_UNUSED GObject *self,
                KgxTab                *tab)
{
  /* Translators: %i is, from the users perspective, a random number.
   * this string will only be seen when the running program has
   * failed to set a title, and exists purely to avoid blank tabs
   */
  return g_strdup_printf (_("Tab %i"), kgx_tab_get_id (tab));
}


static gboolean
object_accumulator (GSignalInvocationHint *ihint,
                    GValue                *return_value,
                    const GValue          *signal_value,
                    gpointer               data)
{
  GObject *object = g_value_get_object (signal_value);

  g_value_set_object (return_value, object);

  return !object;
}


static void
kgx_pages_class_init (KgxPagesClass *klass)
{
  g_autoptr (GError) error = NULL;
  g_autoptr (GBytes) template =
    g_resources_lookup_data (KGX_APPLICATION_PATH "kgx-page-expression.ui",
                             G_RESOURCE_LOOKUP_FLAGS_NONE,
                             &error);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  KgxPagesClassPrivate *class_priv =
    G_TYPE_CLASS_GET_PRIVATE (klass, KGX_TYPE_PAGES, KgxPagesClassPrivate);

  object_class->dispose = kgx_pages_dispose;
  object_class->get_property = kgx_pages_get_property;
  object_class->set_property = kgx_pages_set_property;

  pspecs[PROP_SETTINGS] =
    g_param_spec_object ("settings", NULL, NULL,
                         KGX_TYPE_SETTINGS,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * KgxPages:tab-view:
   *
   * The #AdwTabView
   *
   * Stability: Private
   */
  pspecs[PROP_TAB_VIEW] =
    g_param_spec_object ("tab-view", NULL, NULL,
                         ADW_TYPE_TAB_VIEW,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * KgxPages:title:
   *
   * The #KgxTab:tab-title of the current #KgxTab
   *
   * Note the writability of this property in an implementation detail, DO NOT
   * set this property
   *
   * Stability: Private
   */
  pspecs[PROP_TITLE] =
    g_param_spec_string ("title", NULL, NULL,
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * KgxPages:path:
   *
   * The #KgxTab:tab-path of the current #KgxTab
   *
   * Note the writability of this property in an implementation detail, DO NOT
   * set this property
   *
   * Stability: Private
   */
  pspecs[PROP_PATH] =
    g_param_spec_object ("path", NULL, NULL,
                         G_TYPE_FILE,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * KgxPages:active-page:
   *
   * The #KgxTab in #KgxPages:tab-view's current #AdwTabPage
   *
   * Note the writability of this property in an implementation detail, DO NOT
   * set this property
   */
  pspecs[PROP_ACTIVE_PAGE] =
    g_param_spec_object ("active-page", NULL, NULL,
                         KGX_TYPE_TAB,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_IS_ACTIVE] =
    g_param_spec_boolean ("is-active", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_STATUS] =
    g_param_spec_flags ("status", NULL, NULL,
                        KGX_TYPE_STATUS,
                        KGX_NONE,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_SEARCH_MODE_ENABLED] =
    g_param_spec_boolean ("search-mode-enabled", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_RINGING] =
    g_param_spec_boolean ("ringing", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);


  g_object_class_install_properties (object_class, LAST_PROP, pspecs);

  signals[ZOOM] = g_signal_new ("zoom",
                                G_TYPE_FROM_CLASS (klass),
                                G_SIGNAL_RUN_LAST,
                                0, NULL, NULL,
                                kgx_marshals_VOID__ENUM,
                                G_TYPE_NONE,
                                1,
                                KGX_TYPE_ZOOM);
  g_signal_set_va_marshaller (signals[ZOOM],
                              G_TYPE_FROM_CLASS (klass),
                              kgx_marshals_VOID__ENUMv);

  signals[CREATE_TEAROFF_HOST] = g_signal_new ("create-tearoff-host",
                                               G_TYPE_FROM_CLASS (klass),
                                               G_SIGNAL_RUN_LAST,
                                               0,
                                               object_accumulator, NULL,
                                               kgx_marshals_OBJECT__VOID,
                                               KGX_TYPE_PAGES,
                                               0);
  g_signal_set_va_marshaller (signals[CREATE_TEAROFF_HOST],
                              G_TYPE_FROM_CLASS (klass),
                              kgx_marshals_OBJECT__VOIDv);

  signals[MAYBE_CLOSE_WINDOW] = g_signal_new ("maybe-close-window",
                                              G_TYPE_FROM_CLASS (klass),
                                              G_SIGNAL_RUN_LAST,
                                              0,
                                              NULL, NULL,
                                              kgx_marshals_VOID__VOID,
                                              G_TYPE_NONE,
                                              0);
  g_signal_set_va_marshaller (signals[MAYBE_CLOSE_WINDOW],
                              G_TYPE_FROM_CLASS (klass),
                              kgx_marshals_VOID__VOIDv);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               KGX_APPLICATION_PATH "kgx-pages.ui");

  gtk_widget_class_bind_template_child_private (widget_class, KgxPages, view);
  gtk_widget_class_bind_template_child_private (widget_class, KgxPages, status_message);
  gtk_widget_class_bind_template_child_private (widget_class, KgxPages, status_revealer);
  gtk_widget_class_bind_template_child_private (widget_class, KgxPages, active_page_signals);
  gtk_widget_class_bind_template_child_private (widget_class, KgxPages, active_page_binds);
  gtk_widget_class_bind_template_child_private (widget_class, KgxPages, settings_signals);
  gtk_widget_class_bind_template_child_private (widget_class, KgxPages, style_manager_signals);

  gtk_widget_class_bind_template_callback (widget_class, page_attached);
  gtk_widget_class_bind_template_callback (widget_class, page_detached);
  gtk_widget_class_bind_template_callback (widget_class, create_window);
  gtk_widget_class_bind_template_callback (widget_class, close_page);
  gtk_widget_class_bind_template_callback (widget_class, setup_menu);

  gtk_widget_class_bind_template_callback (widget_class, kgx_style_manager_for_display);
  gtk_widget_class_bind_template_callback (widget_class, kgx_text_or_fallback);
  gtk_widget_class_bind_template_callback (widget_class, kgx_object_or_fallback);

  gtk_widget_class_set_css_name (widget_class, "pages");

  class_priv->page_expression_data = g_steal_pointer (&template);
  if (error) {
    g_critical ("pages: Failed to load template: %s", error->message);
  }

  class_priv->page_expression_scope = gtk_builder_cscope_new ();
  gtk_builder_cscope_add_callback_symbols (GTK_BUILDER_CSCOPE (class_priv->page_expression_scope),
                                           "kgx_text_or_fallback", G_CALLBACK (kgx_text_or_fallback),
                                           "kgx_file_as_display", G_CALLBACK (kgx_file_as_display),
                                           "kgx_status_as_icon", G_CALLBACK (kgx_status_as_icon),
                                           "kgx_ringing_as_icon", G_CALLBACK (kgx_ringing_as_icon),
                                           "fallback_title", G_CALLBACK (fallback_title),
                                           NULL);
}


static void
style_changed (AdwStyleManager *style_manager,
               GParamSpec      *pspec,
               KgxPages        *self)
{
  KgxPagesPrivate *priv = kgx_pages_get_instance_private (self);

  adw_tab_view_invalidate_thumbnails (ADW_TAB_VIEW (priv->view));
}


static void
kgx_pages_init (KgxPages *self)
{
  KgxPagesPrivate *priv = kgx_pages_get_instance_private (self);

  gtk_widget_init_template (GTK_WIDGET (self));

  g_signal_group_connect (priv->active_page_signals,
                          "size-changed", G_CALLBACK (size_changed),
                          self);

  g_signal_group_connect_swapped (priv->settings_signals, "notify::theme",
                                  G_CALLBACK (adw_tab_view_invalidate_thumbnails),
                                  priv->view);

  g_signal_group_connect (priv->style_manager_signals,
                          "notify::dark",
                          G_CALLBACK (style_changed),
                          self);
  g_signal_group_connect (priv->style_manager_signals,
                          "notify::high-contrast",
                          G_CALLBACK (style_changed),
                          priv->view);

  g_binding_group_bind (priv->active_page_binds, "search-mode-enabled",
                        self, "search-mode-enabled",
                        G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

  adw_tab_view_remove_shortcuts (ADW_TAB_VIEW (priv->view),
                                 ADW_TAB_VIEW_SHORTCUT_CONTROL_HOME |
                                 ADW_TAB_VIEW_SHORTCUT_CONTROL_END |
                                 ADW_TAB_VIEW_SHORTCUT_CONTROL_SHIFT_HOME |
                                 ADW_TAB_VIEW_SHORTCUT_CONTROL_SHIFT_END);
}


void
kgx_pages_add_page (KgxPages *self,
                    KgxTab   *tab)
{
  KgxPagesClassPrivate *class_priv;
  KgxPagesPrivate *priv;
  AdwTabPage *page;

  g_return_if_fail (KGX_IS_PAGES (self));
  g_return_if_fail (KGX_IS_TAB (tab));

  class_priv =
    G_TYPE_CLASS_GET_PRIVATE (G_OBJECT_GET_CLASS (self), KGX_TYPE_PAGES, KgxPagesClassPrivate);

  priv = kgx_pages_get_instance_private (self);

  kgx_tab_set_initial_title (tab, priv->title, priv->path);

  page = adw_tab_view_add_page (ADW_TAB_VIEW (priv->view), GTK_WIDGET (tab), NULL);

  kgx_templated_apply (G_OBJECT (page),
                       class_priv->page_expression_scope,
                       class_priv->page_expression_data);
}


/**
 * kgx_pages_focus_page:
 * @self: the #KgxPages
 * @page: the #KgxTab to focus
 *
 * Switch to a page
 */
void
kgx_pages_focus_page (KgxPages *self,
                      KgxTab  *page)
{
  KgxPagesPrivate *priv;
  AdwTabPage *index;

  g_return_if_fail (KGX_IS_PAGES (self));
  g_return_if_fail (KGX_IS_TAB (page));

  priv = kgx_pages_get_instance_private (self);

  index = adw_tab_view_get_page (ADW_TAB_VIEW (priv->view),
                                 GTK_WIDGET (page));

  g_return_if_fail (index != NULL);

  adw_tab_view_set_selected_page (ADW_TAB_VIEW (priv->view), index);

  gtk_widget_grab_focus (GTK_WIDGET (page));
}


/**
 * kgx_pages_current_status:
 * @self: the #KgxPages
 *
 * Get the #KgxStatus of the current #KgxTab
 */
KgxStatus
kgx_pages_current_status (KgxPages *self)
{
  KgxPagesPrivate *priv;

  g_return_val_if_fail (KGX_IS_PAGES (self), KGX_NONE);

  priv = kgx_pages_get_instance_private (self);

  return priv->page_status;
}


gboolean
kgx_pages_is_ringing (KgxPages *self)
{
  KgxPagesPrivate *priv;

  g_return_val_if_fail (KGX_IS_PAGES (self), FALSE);

  priv = kgx_pages_get_instance_private (self);

  return priv->ringing;
}


/**
 * kgx_pages_count:
 * @self: the #KgxPages
 *
 * Returns: number of #KgxTab s in @self
 */
int
kgx_pages_count (KgxPages *self)
{
  KgxPagesPrivate *priv;

  g_return_val_if_fail (KGX_IS_PAGES (self), KGX_NONE);

  priv = kgx_pages_get_instance_private (self);

  return adw_tab_view_get_n_pages (ADW_TAB_VIEW (priv->view));
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
  guint n;

  g_return_val_if_fail (KGX_IS_PAGES (self), NULL);

  priv = kgx_pages_get_instance_private (self);

  children = g_ptr_array_new_full (10, (GDestroyNotify) kgx_process_unref);

  n = adw_tab_view_get_n_pages (ADW_TAB_VIEW (priv->view));

  for (guint i = 0; i < n; i++) {
    AdwTabPage *page = adw_tab_view_get_nth_page (ADW_TAB_VIEW (priv->view), i);
    g_autoptr (GPtrArray) page_children = NULL;

    page_children = kgx_tab_get_children (KGX_TAB (adw_tab_page_get_child (page)));

    g_ptr_array_extend_and_steal (children, g_steal_pointer (&page_children));
  }

  return children;
}


void
kgx_pages_close_page (KgxPages *self)
{
  KgxPagesPrivate *priv;
  AdwTabPage *page;

  g_return_if_fail (KGX_IS_PAGES (self));

  priv = kgx_pages_get_instance_private (self);
  page = priv->action_page;

  if (!page)
    page = adw_tab_view_get_selected_page (ADW_TAB_VIEW (priv->view));

  if (!page)
    return;

  adw_tab_view_close_page (ADW_TAB_VIEW (priv->view), page);
}


void
kgx_pages_detach_page (KgxPages *self)
{
  KgxPagesPrivate *priv;
  AdwTabPage *page;
  AdwTabView *new_view;

  g_return_if_fail (KGX_IS_PAGES (self));

  priv = kgx_pages_get_instance_private (self);
  page = priv->action_page;

  if (!page)
    page = adw_tab_view_get_selected_page (ADW_TAB_VIEW (priv->view));

  if (!page)
    return;

  new_view = create_window (ADW_TAB_VIEW (priv->view), self);
  adw_tab_view_transfer_page (ADW_TAB_VIEW (priv->view), page, new_view, 0);
}


AdwTabPage *
kgx_pages_get_selected_page (KgxPages  *self)
{
  KgxPagesPrivate *priv;

  g_return_val_if_fail (KGX_IS_PAGES (self), NULL);

  priv = kgx_pages_get_instance_private (self);

  return adw_tab_view_get_selected_page (ADW_TAB_VIEW (priv->view));
}
