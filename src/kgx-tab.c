/* kgx-tab.c
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
#define PCRE2_CODE_UNIT_WIDTH 0
#include <pcre2.h>

#include "kgx-application.h"
#include "kgx-drop-target.h"
#include "kgx-empty.h"
#include "kgx-marshals.h"
#include "kgx-pages.h"
#include "kgx-settings.h"
#include "kgx-spad-source.h"
#include "kgx-spad.h"
#include "kgx-terminal.h"
#include "kgx-utils.h"

#include "kgx-tab.h"


typedef struct _KgxTabPrivate KgxTabPrivate;
struct _KgxTabPrivate {
  guint                 id;

  KgxApplication       *application;
  KgxSettings          *settings;

  KgxTrain             *train;
  GSignalGroup         *train_signals;

  char                 *title;
  char                 *tooltip;
  GFile                *path;
  KgxStatus             status;

  gboolean              is_active;
  gboolean              close_on_quit;
  gboolean              needs_attention;
  gboolean              search_mode_enabled;

  gboolean              ringing;
  guint                 ringing_timeout;

  gboolean              dropping;
  int                   working;

  KgxTerminal          *terminal;
  GSignalGroup         *terminal_signals;

  GCancellable         *cancellable;

  char                 *initial_title;
  GFile                *initial_path;
  guint                 initial_timeout;

  KgxDropTarget        *drop_target;

  GtkWidget            *stack;
  GtkWidget            *content;

  GtkWidget            *exit_revealer;
  GtkWidget            *exit_message;
  GtkWidget            *search_entry;
  GtkWidget            *search_bar;
  char                 *last_search;

  AdwToastOverlay      *toasts;

  char                 *notification_id;
};


static void kgx_tab_buildable_iface_init (GtkBuildableIface *iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (KgxTab, kgx_tab, ADW_TYPE_BIN,
                                  G_ADD_PRIVATE (KgxTab)
                                  G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE,
                                                         kgx_tab_buildable_iface_init))

static GtkBuildableIface *parent_buildable_iface;

enum {
  PROP_0,
  PROP_APPLICATION,
  PROP_SETTINGS,
  PROP_TRAIN,
  PROP_TERMINAL,
  PROP_TAB_TITLE,
  PROP_TAB_PATH,
  PROP_TAB_STATUS,
  PROP_TAB_TOOLTIP,
  PROP_IS_ACTIVE,
  PROP_CLOSE_ON_QUIT,
  PROP_NEEDS_ATTENTION,
  PROP_SEARCH_MODE_ENABLED,
  PROP_RINGING,
  PROP_DROPPING,
  PROP_WORKING,
  PROP_CANCELLABLE,
  PROP_INITIAL_TITLE,
  PROP_INITIAL_PATH,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };


enum {
  SIZE_CHANGED,
  ZOOM,
  DIED,
  BELL,
  N_SIGNALS
};
static guint signals[N_SIGNALS];


static void
kgx_tab_dispose (GObject *object)
{
  KgxTab *self = KGX_TAB (object);
  KgxTabPrivate *priv = kgx_tab_get_instance_private (self);

  g_cancellable_cancel (priv->cancellable);

  if (G_UNLIKELY (priv->working != 0)) {
    g_warning ("tab: disposed whilst still working");
    g_application_unmark_busy (G_APPLICATION (priv->application));
    priv->working = 0;
  }

  if (priv->notification_id) {
    g_application_withdraw_notification (G_APPLICATION (priv->application),
                                         priv->notification_id);
    g_clear_pointer (&priv->notification_id, g_free);
  }

  g_clear_object (&priv->application);
  g_clear_object (&priv->settings);
  g_clear_object (&priv->train);
  g_clear_object (&priv->terminal);
  g_clear_object (&priv->cancellable);

  g_clear_pointer (&priv->title, g_free);
  g_clear_pointer (&priv->tooltip, g_free);
  g_clear_object (&priv->path);

  g_clear_handle_id (&priv->ringing_timeout, g_source_remove);

  g_clear_pointer (&priv->initial_title, g_free);
  g_clear_object (&priv->initial_path);
  g_clear_handle_id (&priv->initial_timeout, g_source_remove);

  g_clear_pointer (&priv->last_search, g_free);

  G_OBJECT_CLASS (kgx_tab_parent_class)->dispose (object);
}


static void
search_enabled (GObject    *object,
                GParamSpec *pspec,
                KgxTab     *self)
{
  KgxTabPrivate *priv = kgx_tab_get_instance_private (self);

  if (!gtk_search_bar_get_search_mode (GTK_SEARCH_BAR (priv->search_bar))) {
    gtk_widget_grab_focus (GTK_WIDGET (self));
  }
}


static void
search_changed (GtkSearchBar *bar,
                KgxTab       *self)
{
  KgxTabPrivate *priv = kgx_tab_get_instance_private (self);
  const char *search = NULL;
  VteRegex *regex;
  g_autoptr (GError) error = NULL;
  gboolean narrowing_down;
  guint32 flags = PCRE2_MULTILINE;

  search = gtk_editable_get_text (GTK_EDITABLE (priv->search_entry));

  if (search) {
    g_autofree char *lowercase = g_utf8_strdown (search, -1);

    if (!g_strcmp0 (lowercase, search))
      flags |= PCRE2_CASELESS;
  }

  regex = vte_regex_new_for_search (g_regex_escape_string (search, -1),
                                    -1, flags, &error);

  if (error) {
    g_warning ("Search error: %s", error->message);
    return;
  }

  /* Since VTE doesn't automatically highlight the search match and doesn't have
   * an API to do that or select text manually, we have to be creative.
   *
   * The goals are to:
   * 1. immediately highlight text for type-to-search
   * 2. make sure we don't jump to another match when pressing backspace
   *
   * Achieving 1. is easy by going to the previous match before setting the
   * regex and the next match after setting it. However, this fails with 2:
   * consider a buffer like "foo bar baz". If "baz" is currently highlighted and
   * we press backspace, our regex is "ba" so "ba" in "bar" starts to match as
   * well. If we go to the previous match, change regex from "bar" to "ba" and
   * go next, it will select the occurence in "bar" while we want it to stay on
   * "baz". Hence, if we're narrowing down the search, go previous/next after
   * setting the regex and not before. */

  narrowing_down = search && priv->last_search &&
                   g_strrstr (priv->last_search, search);

  g_set_str (&priv->last_search, search);

  if (!narrowing_down)
    vte_terminal_search_find_previous (VTE_TERMINAL (priv->terminal));

  vte_terminal_search_set_regex (VTE_TERMINAL (priv->terminal),
                                 regex, 0);

  if (narrowing_down)
    vte_terminal_search_find_previous (VTE_TERMINAL (priv->terminal));

  vte_terminal_search_find_next (VTE_TERMINAL (priv->terminal));
}


static void
search_next (GtkSearchBar *bar,
             KgxTab       *self)
{
  KgxTabPrivate *priv = kgx_tab_get_instance_private (self);

  vte_terminal_search_find_next (VTE_TERMINAL (priv->terminal));
}


static void
search_prev (GtkSearchBar *bar,
             KgxTab       *self)
{
  KgxTabPrivate *priv = kgx_tab_get_instance_private (self);

  vte_terminal_search_find_previous (VTE_TERMINAL (priv->terminal));
}


static inline void
set_status (KgxTab    *self,
            KgxStatus  status)
{
  KgxTabPrivate *priv = kgx_tab_get_instance_private (self);

  if (priv->status == status) {
    return;
  }

  priv->status = status;

  if (status & KGX_REMOTE) {
    gtk_widget_add_css_class (GTK_WIDGET (self), KGX_WINDOW_STYLE_REMOTE);
  } else {
    gtk_widget_remove_css_class (GTK_WIDGET (self), KGX_WINDOW_STYLE_REMOTE);
  }

  if (status & KGX_PRIVILEGED) {
    gtk_widget_add_css_class (GTK_WIDGET (self), KGX_WINDOW_STYLE_ROOT);
  } else {
    gtk_widget_remove_css_class (GTK_WIDGET (self), KGX_WINDOW_STYLE_ROOT);
  }

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_TAB_STATUS]);
}


static void
kgx_tab_get_property (GObject    *object,
                      guint       property_id,
                      GValue     *value,
                      GParamSpec *pspec)
{
  KgxTab *self = KGX_TAB (object);
  KgxTabPrivate *priv = kgx_tab_get_instance_private (self);

  switch (property_id) {
    case PROP_APPLICATION:
      g_value_set_object (value, priv->application);
      break;
    case PROP_SETTINGS:
      g_value_set_object (value, priv->settings);
      break;
    case PROP_TRAIN:
      g_value_set_object (value, priv->train);
      break;
    case PROP_TERMINAL:
      g_value_set_object (value, priv->terminal);
      break;
    case PROP_TAB_TITLE:
      g_value_set_string (value, priv->title);
      break;
    case PROP_TAB_PATH:
      g_value_set_object (value, priv->path);
      break;
    case PROP_TAB_STATUS:
      g_value_set_flags (value, priv->status);
      break;
    case PROP_TAB_TOOLTIP:
      g_value_set_string (value, priv->tooltip);
      break;
    case PROP_IS_ACTIVE:
      g_value_set_boolean (value, priv->is_active);
      break;
    case PROP_CLOSE_ON_QUIT:
      g_value_set_boolean (value, priv->close_on_quit);
      break;
    case PROP_NEEDS_ATTENTION:
      g_value_set_boolean (value, priv->needs_attention);
      break;
    case PROP_SEARCH_MODE_ENABLED:
      g_value_set_boolean (value, priv->search_mode_enabled);
      break;
    case PROP_RINGING:
      g_value_set_boolean (value, priv->ringing);
      break;
    case PROP_DROPPING:
      g_value_set_boolean (value, priv->dropping);
      break;
    case PROP_WORKING:
      g_value_set_boolean (value, priv->working > 0);
      break;
    case PROP_CANCELLABLE:
      g_value_set_object (value, priv->cancellable);
      break;
    case PROP_INITIAL_TITLE:
      g_value_set_string (value, priv->initial_title);
      break;
    case PROP_INITIAL_PATH:
      g_value_set_object (value, priv->initial_path);
      break;
    KGX_INVALID_PROP (object, property_id, pspec);
  }
}


static void
clear_initial (gpointer data)
{
  KgxTab *self = data;
  KgxTabPrivate *priv;

  g_return_if_fail (KGX_IS_TAB (self));

  priv = kgx_tab_get_instance_private (self);

  priv->initial_timeout = 0;

  g_clear_pointer (&priv->initial_title, g_free);
  g_clear_object (&priv->initial_path);

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_INITIAL_TITLE]);
  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_INITIAL_PATH]);
}


static void inline
schedule_clear_initial (KgxTab *self)
{
  KgxTabPrivate *priv = kgx_tab_get_instance_private (self);

  g_clear_handle_id (&priv->initial_timeout, g_source_remove);
  priv->initial_timeout = g_timeout_add_once (200, clear_initial, self);
  g_source_set_name_by_id (priv->initial_timeout, "[kgx] clear initial title");
}


static void
kgx_tab_set_property (GObject      *object,
                      guint         property_id,
                      const GValue *value,
                      GParamSpec   *pspec)
{
  KgxTab *self = KGX_TAB (object);
  KgxTabPrivate *priv = kgx_tab_get_instance_private (self);

  switch (property_id) {
    case PROP_APPLICATION:
      if (priv->application) {
        g_critical ("Application was already set %p", priv->application);
      }
      priv->application = g_value_dup_object (value);
      kgx_application_add_page (priv->application, self);
      break;
    case PROP_SETTINGS:
      g_set_object (&priv->settings, g_value_get_object (value));
      break;
    case PROP_TERMINAL:
      g_set_object (&priv->terminal, g_value_get_object (value));
      break;
    case PROP_TAB_TITLE:
      kgx_set_str_prop (object, pspec, &priv->title, value);
      break;
    case PROP_TAB_PATH:
      if (g_set_object (&priv->path, g_value_get_object (value))) {
        g_object_notify_by_pspec (object, pspec);
      }
      break;
    case PROP_TAB_STATUS:
      set_status (self, g_value_get_flags (value));
      break;
    case PROP_TAB_TOOLTIP:
      kgx_set_str_prop (object, pspec, &priv->tooltip, value);
      break;
    case PROP_IS_ACTIVE:
      if (kgx_set_boolean_prop (object, pspec, &priv->is_active, value)) {
        if (priv->is_active && priv->notification_id) {
          g_application_withdraw_notification (G_APPLICATION (priv->application),
                                               priv->notification_id);
          g_clear_pointer (&priv->notification_id, g_free);
        }
        g_object_set (self, "needs-attention", FALSE, NULL);
      }
      break;
    case PROP_CLOSE_ON_QUIT:
      kgx_set_boolean_prop (object, pspec, &priv->close_on_quit, value);
      break;
    case PROP_NEEDS_ATTENTION:
      kgx_set_boolean_prop (object, pspec, &priv->needs_attention, value);
      break;
    case PROP_SEARCH_MODE_ENABLED:
      kgx_set_boolean_prop (object, pspec, &priv->search_mode_enabled, value);
      break;
    case PROP_DROPPING:
      kgx_set_boolean_prop (object, pspec, &priv->dropping, value);
      break;
    case PROP_INITIAL_TITLE:
      if (kgx_set_str_prop (object, pspec, &priv->initial_title, value)) {
        schedule_clear_initial (self);
      }
      break;
    case PROP_INITIAL_PATH:
      if (g_set_object (&priv->initial_path, g_value_get_object (value))) {
        schedule_clear_initial (self);
        g_object_notify_by_pspec (object, pspecs[PROP_INITIAL_PATH]);
      }
      break;
    KGX_INVALID_PROP (object, property_id, pspec);
  }
}


static void
drop (KgxDropTarget *target,
      const char    *text,
      KgxTab        *self)
{
  KgxTabPrivate *priv = kgx_tab_get_instance_private (self);

  if (priv->terminal) {
    kgx_terminal_accept_paste (KGX_TERMINAL (priv->terminal), text);
  }
}


static gboolean
kgx_tab_grab_focus (GtkWidget *widget)
{
  KgxTab *self = KGX_TAB (widget);
  KgxTabPrivate *priv = kgx_tab_get_instance_private (self);

  if (priv->terminal) {
    gtk_widget_grab_focus (GTK_WIDGET (priv->terminal));
    return GDK_EVENT_STOP;
  }

  return GTK_WIDGET_CLASS (kgx_tab_parent_class)->grab_focus (widget);
}


static void
kgx_tab_real_start (KgxTab              *tab,
                    GAsyncReadyCallback  callback,
                    gpointer             callback_data)
{
  g_critical ("%s doesn't implement start", G_OBJECT_TYPE_NAME (tab));
}


static KgxTrain *
kgx_tab_real_start_finish (KgxTab        *tab,
                           GAsyncResult  *res,
                           GError       **error)
{
  g_critical ("%s doesn't implement start_finish", G_OBJECT_TYPE_NAME (tab));

  return NULL;
}


static void
kgx_tab_real_died (KgxTab         *self,
                   GtkMessageType  type,
                   const char     *message,
                   gboolean        success)
{
  KgxTabPrivate *priv;

  g_return_if_fail (KGX_IS_TAB (self));

  priv = kgx_tab_get_instance_private (self);

  gtk_label_set_markup (GTK_LABEL (priv->exit_message), message);

  if (type == GTK_MESSAGE_ERROR) {
    gtk_widget_add_css_class (priv->exit_revealer, "error");
  } else {
    gtk_widget_remove_css_class (priv->exit_revealer, "error");
  }

  gtk_revealer_set_reveal_child (GTK_REVEALER (priv->exit_revealer), TRUE);

  g_cancellable_cancel (priv->cancellable);
}


static void
clear_ringing (gpointer data)
{
  KgxTab *self = data;
  KgxTabPrivate *priv;

  g_return_if_fail (KGX_IS_TAB (self));

  priv = kgx_tab_get_instance_private (self);

  priv->ringing_timeout = 0;

  priv->ringing = FALSE;
  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_RINGING]);
}


static void
kgx_tab_real_bell (KgxTab *self)
{
  KgxTabPrivate *priv;

  g_return_if_fail (KGX_IS_TAB (self));

  priv = kgx_tab_get_instance_private (self);

  if (!kgx_settings_get_visual_bell (priv->settings)) {
    return;
  }

  priv->ringing = TRUE;
  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_RINGING]);

  g_clear_handle_id (&priv->ringing_timeout, g_source_remove);
  priv->ringing_timeout = g_timeout_add_once (500, clear_ringing, self);
  g_source_set_name_by_id (priv->ringing_timeout, "[kgx] tab ringing");
}


static gboolean
spad_thrown (KgxSpadSource *source,
             const char    *title,
             GVariant      *bundle,
             KgxTab        *self)
{
  KgxTabPrivate *priv = kgx_tab_get_instance_private (self);
  AdwToast *toast = g_object_new (ADW_TYPE_TOAST,
                                  "title", title,
                                  "button-label", C_("toast-button", "_Details"),
                                  "action-name", "tab.show-spad",
                                  "action-target", bundle,
                                  NULL);

  adw_toast_overlay_add_toast (priv->toasts, toast);

  return TRUE;
}


static void
show_spad (GtkWidget                *widget,
           G_GNUC_UNUSED const char *action,
           GVariant                 *target)
{
  adw_dialog_present (kgx_spad_new_from_bundle (target), widget);
}


static void
kgx_tab_class_init (KgxTabClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS   (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  KgxTabClass    *tab_class    = KGX_TAB_CLASS    (klass);

  object_class->dispose = kgx_tab_dispose;
  object_class->get_property = kgx_tab_get_property;
  object_class->set_property = kgx_tab_set_property;

  widget_class->grab_focus = kgx_tab_grab_focus;

  tab_class->start = kgx_tab_real_start;
  tab_class->start_finish = kgx_tab_real_start_finish;
  tab_class->died = kgx_tab_real_died;
  tab_class->bell = kgx_tab_real_bell;

  /**
   * KgxTab:application:
   *
   * The #KgxApplication this tab is running under
   *
   * Stability: Private
   */
  pspecs[PROP_APPLICATION] =
    g_param_spec_object ("application", NULL, NULL,
                         KGX_TYPE_APPLICATION,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_SETTINGS] =
    g_param_spec_object ("settings", NULL, NULL,
                         KGX_TYPE_SETTINGS,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_TRAIN] =
    g_param_spec_object ("train", NULL, NULL,
                         KGX_TYPE_TRAIN,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_TERMINAL] =
    g_param_spec_object ("terminal", NULL, NULL,
                         KGX_TYPE_TERMINAL,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * KgxTab:tab-title:
   *
   * Title of this tab
   *
   * Stability: Private
   */
  pspecs[PROP_TAB_TITLE] =
    g_param_spec_string ("tab-title", NULL, NULL,
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * KgxTab:tab-path:
   *
   * Current path of this tab
   *
   * Stability: Private
   */
  pspecs[PROP_TAB_PATH] =
    g_param_spec_object ("tab-path", NULL, NULL,
                         G_TYPE_FILE,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_TAB_STATUS] =
    g_param_spec_flags ("tab-status", NULL, NULL,
                        KGX_TYPE_STATUS,
                        KGX_NONE,
                        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_TAB_TOOLTIP] =
    g_param_spec_string ("tab-tooltip", NULL, NULL,
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * KgxTab:is-active:
   *
   * This is the active tab of the active window
   *
   * Stability: Private
   */
  pspecs[PROP_IS_ACTIVE] =
    g_param_spec_boolean ("is-active", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_CLOSE_ON_QUIT] =
    g_param_spec_boolean ("close-on-quit", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_NEEDS_ATTENTION] =
    g_param_spec_boolean ("needs-attention", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_SEARCH_MODE_ENABLED] =
    g_param_spec_boolean ("search-mode-enabled", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_RINGING] =
    g_param_spec_boolean ("ringing", NULL, NULL,
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_DROPPING] =
    g_param_spec_boolean ("dropping", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_WORKING] =
    g_param_spec_boolean ("working", NULL, NULL,
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_CANCELLABLE] =
    g_param_spec_object ("cancellable", NULL, NULL,
                         G_TYPE_CANCELLABLE,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_INITIAL_TITLE] =
    g_param_spec_string ("initial-title", NULL, NULL,
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_INITIAL_PATH] =
    g_param_spec_object ("initial-path", NULL, NULL,
                         G_TYPE_FILE,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);

  signals[SIZE_CHANGED] = g_signal_new ("size-changed",
                                        G_TYPE_FROM_CLASS (klass),
                                        G_SIGNAL_RUN_LAST,
                                        0, NULL, NULL,
                                        kgx_marshals_VOID__UINT_UINT,
                                        G_TYPE_NONE,
                                        2,
                                        G_TYPE_UINT,
                                        G_TYPE_UINT);
  g_signal_set_va_marshaller (signals[SIZE_CHANGED],
                              G_TYPE_FROM_CLASS (klass),
                              kgx_marshals_VOID__UINT_UINTv);

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

  signals[DIED] = g_signal_new ("died",
                                G_TYPE_FROM_CLASS (klass),
                                G_SIGNAL_RUN_LAST,
                                G_STRUCT_OFFSET (KgxTabClass, died),
                                NULL, NULL,
                                kgx_marshals_VOID__ENUM_STRING_BOOLEAN,
                                G_TYPE_NONE,
                                3,
                                GTK_TYPE_MESSAGE_TYPE,
                                G_TYPE_STRING,
                                G_TYPE_BOOLEAN);
  g_signal_set_va_marshaller (signals[DIED],
                              G_TYPE_FROM_CLASS (klass),
                              kgx_marshals_VOID__ENUM_STRING_BOOLEANv);

  signals[BELL] = g_signal_new ("bell",
                                G_TYPE_FROM_CLASS (klass),
                                G_SIGNAL_RUN_LAST,
                                G_STRUCT_OFFSET (KgxTabClass, bell),
                                NULL, NULL,
                                kgx_marshals_VOID__VOID,
                                G_TYPE_NONE,
                                0);
  g_signal_set_va_marshaller (signals[BELL],
                              G_TYPE_FROM_CLASS (klass),
                              kgx_marshals_VOID__VOIDv);

  g_type_ensure (KGX_TYPE_DROP_TARGET);
  g_type_ensure (KGX_TYPE_EMPTY);
  g_type_ensure (KGX_TYPE_TERMINAL);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               KGX_APPLICATION_PATH "kgx-tab.ui");

  gtk_widget_class_bind_template_child_private (widget_class, KgxTab, stack);
  gtk_widget_class_bind_template_child_private (widget_class, KgxTab, exit_revealer);
  gtk_widget_class_bind_template_child_private (widget_class, KgxTab, exit_message);
  gtk_widget_class_bind_template_child_private (widget_class, KgxTab, search_entry);
  gtk_widget_class_bind_template_child_private (widget_class, KgxTab, search_bar);
  gtk_widget_class_bind_template_child_private (widget_class, KgxTab, train_signals);
  gtk_widget_class_bind_template_child_private (widget_class, KgxTab, terminal_signals);
  gtk_widget_class_bind_template_child_private (widget_class, KgxTab, drop_target);
  gtk_widget_class_bind_template_child_private (widget_class, KgxTab, toasts);

  gtk_widget_class_bind_template_callback (widget_class, search_enabled);
  gtk_widget_class_bind_template_callback (widget_class, search_changed);
  gtk_widget_class_bind_template_callback (widget_class, search_next);
  gtk_widget_class_bind_template_callback (widget_class, search_prev);
  gtk_widget_class_bind_template_callback (widget_class, drop);
  gtk_widget_class_bind_template_callback (widget_class, spad_thrown);

  gtk_widget_class_install_action (widget_class,
                                   "tab.show-spad",
                                   "a{sv}",
                                   show_spad);

  gtk_widget_class_set_css_name (widget_class, "kgx-tab");
}


static void
kgx_tab_add_child (GtkBuildable *buildable,
                   GtkBuilder   *builder,
                   GObject      *child,
                   const char   *type)
{
  KgxTab *self = KGX_TAB (buildable);
  KgxTabPrivate *priv;

  g_return_if_fail (KGX_IS_TAB (self));
  g_return_if_fail (GTK_IS_WIDGET (child));

  priv = kgx_tab_get_instance_private (self);

  if (type && g_str_equal (type, "content")) {
    g_set_weak_pointer (&priv->content, GTK_WIDGET (child));
    gtk_stack_add_named (GTK_STACK (priv->stack), GTK_WIDGET (child), "content");
  } else if (GTK_IS_WIDGET (child)) {
    adw_bin_set_child (ADW_BIN (self), GTK_WIDGET (child));
  } else {
    parent_buildable_iface->add_child (buildable, builder, child, type);
  }
}


static void
kgx_tab_buildable_iface_init (GtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = kgx_tab_add_child;
}


static void
pid_died (KgxTrain *train,
          int       status,
          gpointer  user_data)

{
  g_autoptr (GError) error = NULL;
  KgxTab *self = KGX_TAB (user_data);

  /* wait_check will set @error if it got a signal/non-zero exit */
  if (!g_spawn_check_wait_status (status, &error)) {
    g_autofree char *message = NULL;

    /* Translators: <b> </b> marks the text as bold, ensure they are matched please! */
    message = g_strdup_printf (_("<b>Read Only</b> — Command exited with code %i"),
                               status);

    kgx_tab_died (self, GTK_MESSAGE_ERROR, message, TRUE);
  } else {
    kgx_tab_died (self,
                  GTK_MESSAGE_INFO,
    /* Translators: <b> </b> marks the text as bold, ensure they are matched please! */
                  _("<b>Read Only</b> — Command exited"),
                  TRUE);
  }
}


static void
child_removed (KgxTrain   *train,
               KgxProcess *child,
               gpointer    user_data)
{
  KgxTab *self = KGX_TAB (user_data);
  KgxTabPrivate *priv = kgx_tab_get_instance_private (self);

  if (!kgx_tab_is_active (self)) {
    g_autoptr (GNotification) noti = NULL;
    g_autofree char *body = NULL;
    g_autofree char *process_title = NULL;
    g_autofree char *process_subtitle = NULL;

    noti = g_notification_new (_("Command completed"));

    kgx_process_get_title (child, &process_title, &process_subtitle);
    if (process_subtitle) {
      body = g_strdup_printf ("%s, %s",
                              process_title,
                              process_subtitle);
    } else {
      body = g_steal_pointer (&process_title);
    }
    g_notification_set_body (noti, body);

    g_notification_set_default_action_and_target (noti,
                                                  "app.focus-page",
                                                  "u",
                                                  priv->id);

    priv->notification_id = g_strdup_printf ("command-completed-%u", priv->id);
    g_application_send_notification (G_APPLICATION (priv->application),
                                     priv->notification_id,
                                     noti);

    if (!gtk_widget_get_mapped (GTK_WIDGET (self))) {
      g_object_set (self, "needs-attention", TRUE, NULL);
    }
  }
}


static void
size_changed (KgxTerminal *term,
              guint        rows,
              guint        cols,
              KgxTab      *self)
{
  g_signal_emit (self, signals[SIZE_CHANGED], 0, rows, cols);
}


static void
zoom (KgxTerminal *term,
      KgxZoom      zoom,
      KgxTab      *self)
{
  g_signal_emit (self, signals[ZOOM], 0, zoom);
}


static void
bell (KgxTerminal *term,
      KgxTab      *self)
{
  kgx_tab_bell (self);
}


static void
kgx_tab_init (KgxTab *self)
{
  static guint last_id = 0;
  KgxTabPrivate *priv = kgx_tab_get_instance_private (self);

  last_id++;

  priv->id = last_id;

  priv->cancellable = g_cancellable_new ();

  priv->working = 0;

  gtk_widget_init_template (GTK_WIDGET (self));

  g_signal_group_connect_object (priv->train_signals,
                                 "pid-died", G_CALLBACK (pid_died),
                                 self, G_CONNECT_DEFAULT);
  g_signal_group_connect_object (priv->train_signals,
                                 "child-removed", G_CALLBACK (child_removed),
                                 self, G_CONNECT_DEFAULT);

  g_signal_group_connect (priv->terminal_signals,
                          "size-changed", G_CALLBACK (size_changed),
                          self),
  g_signal_group_connect (priv->terminal_signals,
                          "zoom", G_CALLBACK (zoom),
                          self),
  g_signal_group_connect (priv->terminal_signals,
                          "bell", G_CALLBACK (bell),
                          self),
  g_signal_group_connect (priv->terminal_signals,
                          "spad-thrown", G_CALLBACK (spad_thrown),
                          self),

  gtk_search_bar_connect_entry (GTK_SEARCH_BAR (priv->search_bar),
                                GTK_EDITABLE (priv->search_entry));

  kgx_drop_target_mount_on (priv->drop_target, GTK_WIDGET (self));
}


void
kgx_tab_start (KgxTab              *self,
               GAsyncReadyCallback  callback,
               gpointer             callback_data)
{
  g_return_if_fail (KGX_IS_TAB (self));
  g_return_if_fail (KGX_TAB_GET_CLASS (self)->start);

  kgx_tab_mark_working (self);

  KGX_TAB_GET_CLASS (self)->start (self, callback, callback_data);
}


void
kgx_tab_start_finish (KgxTab        *self,
                      GAsyncResult  *res,
                      GError       **error)
{
  g_autoptr (KgxTrain) train = NULL;
  KgxTabPrivate *priv;

  g_return_if_fail (KGX_IS_TAB (self));
  g_return_if_fail (KGX_TAB_GET_CLASS (self)->start);

  priv = kgx_tab_get_instance_private (self);

  train = KGX_TAB_GET_CLASS (self)->start_finish (self, res, error);

  kgx_tab_unmark_working (self);

  gtk_stack_set_visible_child (GTK_STACK (priv->stack), priv->content);
  gtk_widget_grab_focus (GTK_WIDGET (self));

  if (g_set_object (&priv->train, train)) {
    g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_TRAIN]);
  }
}


void
kgx_tab_died (KgxTab         *self,
              GtkMessageType  type,
              const char     *message,
              gboolean        success)
{
  g_signal_emit (self, signals[DIED], 0, type, message, success);
}


void
kgx_tab_bell (KgxTab *self)
{
  KgxTabPrivate *priv;

  g_return_if_fail (KGX_IS_TAB (self));

  priv = kgx_tab_get_instance_private (self);

  if (!priv->is_active && !priv->needs_attention) {
    g_object_set (self, "needs-attention", TRUE, NULL);
  }

  g_signal_emit (self, signals[BELL], 0);
}


/**
 * kgx_tab_get_pages:
 * @self: the #KgxTab
 *
 * Find the #KgxTabs @self is (currently) a memember of
 *
 * Returns: (transfer none): the #KgxTabs
 */
KgxPages *
kgx_tab_get_pages (KgxTab *self)
{
  GtkWidget *parent;

  parent = gtk_widget_get_ancestor (GTK_WIDGET (self), KGX_TYPE_PAGES);

  g_return_val_if_fail (parent, NULL);
  g_return_val_if_fail (KGX_IS_PAGES (parent), NULL);

  return KGX_PAGES (parent);
}


/**
 * kgx_tab_get_id:
 * @self: the #KgxTab
 *
 * Get the unique (in the instance) id for the page
 *
 * Returns: the identifier for the page
 */
guint
kgx_tab_get_id (KgxTab *self)
{
  KgxTabPrivate *priv;

  g_return_val_if_fail (KGX_IS_TAB (self), 0);

  priv = kgx_tab_get_instance_private (self);

  return priv->id;
}


gboolean
kgx_tab_is_active (KgxTab *self)
{
  KgxTabPrivate *priv;

  g_return_val_if_fail (KGX_IS_TAB (self), FALSE);

  priv = kgx_tab_get_instance_private (self);

  return priv->is_active;
}


/**
 * kgx_tab_get_children:
 * @self: the #KgxTab
 *
 * Get a list of child process running in @self
 *
 * NOTE: This doesn't include the shell/root itself
 *
 * Returns: (element-type Kgx.Process) (transfer full): the list of #KgxProcess
 */
GPtrArray *
kgx_tab_get_children (KgxTab *self)
{
  KgxTabPrivate *priv;

  g_return_val_if_fail (KGX_IS_TAB (self), NULL);

  priv = kgx_tab_get_instance_private (self);

  if (G_LIKELY (priv->train)) {
    return kgx_train_get_children (priv->train);
  } else {
    return g_ptr_array_new ();
  }
}


void
kgx_tab_extra_drop (KgxTab       *self,
                    const GValue *value)
{
  KgxTabPrivate *priv;

  g_return_if_fail (KGX_IS_TAB (self));

  priv = kgx_tab_get_instance_private (self);

  kgx_drop_target_extra_drop (priv->drop_target, value);
}


void
kgx_tab_set_initial_title (KgxTab     *self,
                           const char *title,
                           GFile      *path)
{
  g_return_if_fail (KGX_IS_TAB (self));

  g_object_set (self,
                "initial-title", title,
                "initial-path", path,
                NULL);
}


void
kgx_tab_mark_working (KgxTab *self)
{
  KgxTabPrivate *priv;
  gboolean was_not_working;

  g_return_if_fail (KGX_IS_TAB (self));

  priv = kgx_tab_get_instance_private (self);

  was_not_working = priv->working == 0;

  priv->working += 1;

  if (G_LIKELY (was_not_working)) {
    g_application_mark_busy (G_APPLICATION (priv->application));
    g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_WORKING]);
  }
}


void
kgx_tab_unmark_working (KgxTab *self)
{
  KgxTabPrivate *priv;

  g_return_if_fail (KGX_IS_TAB (self));

  priv = kgx_tab_get_instance_private (self);

  g_return_if_fail (priv->working > 0);

  priv->working -= 1;

  if (G_LIKELY (priv->working == 0)) {
    g_application_unmark_busy (G_APPLICATION (priv->application));
    g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_WORKING]);
  }
}
