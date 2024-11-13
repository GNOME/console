/* kgx-terminal.c
 *
 * Copyright 2019-2024 Zander Brown
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
 * SECTION:kgx-terminal
 * @title: KgxTerminal
 * @short_description: The terminal
 *
 * The main terminal widget with various features added such as a context
 * menu (via #GtkPopover) and link detection
 */

#include "kgx-config.h"

#include <glib/gi18n.h>

#include <vte/vte.h>
#define PCRE2_CODE_UNIT_WIDTH 0
#include <pcre2.h>

#include "kgx-despatcher.h"
#include "kgx-marshals.h"
#include "kgx-paste-dialog.h"
#include "kgx-settings.h"
#include "kgx-shared-closures.h"

#include "kgx-terminal.h"

/*       Regex adapted from TerminalWidget.vala in Pantheon Terminal       */

#define USERCHARS "-[:alnum:]"
#define USERCHARS_CLASS "[" USERCHARS "]"
#define PASSCHARS_CLASS "[-[:alnum:]\\Q,?;.:/!%$^*&~\"#'\\E]"
#define HOSTCHARS_CLASS "[-[:alnum:]]"
#define HOST HOSTCHARS_CLASS "+(\\." HOSTCHARS_CLASS "+)*"
#define PORT "(?:\\:[[:digit:]]{1,5})?"
#define PATHCHARS_CLASS "[-[:alnum:]\\Q_$.+!*,;:@&=?/~#%\\E]"
#define PATHTERM_CLASS "[^\\Q]'.}>) \t\r\n,\"\\E]"
#define SCHEME "(?:news:|telnet:|nntp:|file:\\/|https?:|ftps?:|sftp:|webcal:\n" \
               "|irc:|sftp:|ldaps?:|nfs:|smb:|rsync:|"                          \
               "ssh:|rlogin:|telnet:|git:\n"                                    \
               "|git\\+ssh:|bzr:|bzr\\+ssh:|svn:|svn\\"                         \
               "+ssh:|hg:|mailto:|magnet:)"
#define USERPASS USERCHARS_CLASS "+(?:" PASSCHARS_CLASS "+)?"
#define URLPATH "(?:(/" PATHCHARS_CLASS "+(?:[(]" PATHCHARS_CLASS "*[)])*" PATHCHARS_CLASS "*)*" PATHTERM_CLASS ")?"

#define KGX_TERMINAL_N_LINK_REGEX 5

static const char *links[KGX_TERMINAL_N_LINK_REGEX] = {
  SCHEME "//(?:" USERPASS "\\@)?" HOST PORT URLPATH,
  "(?:www|ftp)" HOSTCHARS_CLASS "*\\." HOST PORT URLPATH,
  "(?:callto:|h323:|sip:)" USERCHARS_CLASS "[" USERCHARS ".]*(?:" PORT "/[a-z0-9]+)?\\@" HOST,
  "(?:mailto:)?" USERCHARS_CLASS "[" USERCHARS ".]*\\@" HOSTCHARS_CLASS "+\\." HOST,
  "(?:news:|man:|info:)[-[:alnum:]\\Q^_{|}~!\"#$%&'()*+,./;:=?`\\E]+"
};

/*       Regex adapted from TerminalWidget.vala in Pantheon Terminal       */

/**
 * KgxTerminal:
 * @current_url: the address under the cursor
 * @match_id: regex ids for finding hyperlinks
 *
 * Stability: Private
 */
struct _KgxTerminal {
  VteTerminal parent_instance;

  KgxSettings   *settings;
  GCancellable  *cancellable;
  KgxPalette    *palette;

  KgxDespatcher *despatcher;

  /* Hyperlinks */
  char       *current_url;
  int         match_id[KGX_TERMINAL_N_LINK_REGEX];
};


G_DEFINE_TYPE (KgxTerminal, kgx_terminal, VTE_TYPE_TERMINAL)

enum {
  PROP_0,
  PROP_SETTINGS,
  PROP_CANCELLABLE,
  PROP_PATH,
  PROP_PALETTE,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };

enum {
  SIZE_CHANGED,
  ZOOM,
  N_SIGNALS
};
static guint signals[N_SIGNALS];


static void
kgx_terminal_dispose (GObject *object)
{
  KgxTerminal *self = KGX_TERMINAL (object);

  g_clear_object (&self->cancellable);

  g_clear_pointer (&self->current_url, g_free);

  g_clear_object (&self->settings);

  g_clear_object (&self->despatcher);

  g_clear_pointer (&self->palette, kgx_palette_unref);

  G_OBJECT_CLASS (kgx_terminal_parent_class)->dispose (object);
}


static void
kgx_terminal_set_palette (KgxTerminal *self, KgxPalette *palette)
{
  GdkRGBA foreground, background;
  const GdkRGBA *colours;
  size_t n_colours;

  if (!kgx_set_palette (&self->palette, palette)) {
    return;
  }

  kgx_palette_get_colours (palette,
                           &foreground,
                           &background,
                           &n_colours,
                           &colours);
  vte_terminal_set_colors (VTE_TERMINAL (self),
                           &foreground,
                           &background,
                           colours,
                           n_colours);

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_PALETTE]);
}


static void
kgx_terminal_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  KgxTerminal *self = KGX_TERMINAL (object);

  switch (property_id) {
    case PROP_SETTINGS:
      if (g_set_object (&self->settings, g_value_get_object (value))) {
        g_object_notify_by_pspec (object, pspec);
      }
      break;
    case PROP_CANCELLABLE:
      if (g_set_object (&self->cancellable, g_value_get_object (value))) {
        g_object_notify_by_pspec (object, pspec);
      }
      break;
    case PROP_PALETTE:
      kgx_terminal_set_palette (self, g_value_get_boxed (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static inline char *
get_uri_prop (VteTerminal *terminal, const char *prop)
{
  g_autoptr (GUri) uri = NULL;

  uri = vte_terminal_ref_termprop_uri (terminal, prop);
  if (!uri) {
    return NULL;
  }

  return g_uri_to_string (uri);
}


static inline char *
get_file_uri_prop (VteTerminal *terminal)
{
  return get_uri_prop (terminal, VTE_TERMPROP_CURRENT_FILE_URI);
}


static inline char *
get_directory_uri_prop (VteTerminal *terminal)
{
  return get_uri_prop (terminal, VTE_TERMPROP_CURRENT_DIRECTORY_URI);
}


static inline char *
get_either_uri_prop (VteTerminal *terminal)
{
  char *res = get_file_uri_prop (terminal);

  if (res) {
    return res;
  }

  return get_directory_uri_prop (terminal);
}


static inline GFile *
get_location (VteTerminal *terminal)
{
  g_autofree char *uri = get_either_uri_prop (terminal);

  if (!uri) {
    return NULL;
  }

  return g_file_new_for_uri (uri);
}


static void
kgx_terminal_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  KgxTerminal *self = KGX_TERMINAL (object);

  switch (property_id) {
    case PROP_SETTINGS:
      g_value_set_object (value, self->settings);
      break;
    case PROP_CANCELLABLE:
      g_value_set_object (value, self->cancellable);
      break;
    case PROP_PATH:
      g_value_take_object (value, get_location (VTE_TERMINAL (self)));
      break;
    case PROP_PALETTE:
      g_value_set_boxed (value, self->palette);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static gboolean
have_url_under_pointer (KgxTerminal *self,
                        double       x,
                        double       y)
{
  g_autofree char *hyperlink = NULL;
  g_autofree char *match = NULL;
  int match_id = -1;
  gboolean current = FALSE;

  hyperlink = vte_terminal_check_hyperlink_at (VTE_TERMINAL (self), x, y);

  if (G_UNLIKELY (hyperlink)) {
    g_set_str (&self->current_url, hyperlink);
    current = TRUE;
  } else {
    match = vte_terminal_check_match_at (VTE_TERMINAL (self), x, y, &match_id);

    if (match) {
      for (int i = 0; i < KGX_TERMINAL_N_LINK_REGEX; i++) {
        if (self->match_id[i] == match_id) {
          g_set_str (&self->current_url, match);
          current = TRUE;
          break;
        }
      }
    }
  }

  return current;
}


static void
did_open (GObject *source, GAsyncResult *res, gpointer user_data)
{
  g_autoptr (KgxTerminal) self = user_data;
  g_autoptr (GError) error = NULL;

  kgx_despatcher_open_finish (KGX_DESPATCHER (source), res, &error);

  if (error) {
    g_warning ("Couldn't open uri: %s\n", error->message);
  }
}


static inline void
ensure_despatcher (KgxTerminal *self)
{
  if (!self->despatcher) {
    self->despatcher = g_object_new (KGX_TYPE_DESPATCHER, NULL);
  }
}


static void
open_link (KgxTerminal *self)
{
  ensure_despatcher (self);

  kgx_despatcher_open (self->despatcher,
                       self->current_url,
                       GTK_WINDOW (gtk_widget_get_root (GTK_WIDGET (self))),
                       self->cancellable,
                       did_open,
                       g_object_ref (self));
}


static void
open_link_activated (KgxTerminal *self)
{
  open_link (self);
}


static void
copy_link_activated (KgxTerminal *self)
{
  GdkClipboard *cb = gtk_widget_get_clipboard (GTK_WIDGET (self));

  gdk_clipboard_set_text (cb, self->current_url);
}


static void
copy_activated (KgxTerminal *self)
{
  GdkClipboard *clipboard = gtk_widget_get_clipboard (GTK_WIDGET (self));
  g_autofree char *text = vte_terminal_get_text_selected (VTE_TERMINAL (self),
                                                          VTE_FORMAT_TEXT);
  gdk_clipboard_set_text (clipboard, text);
}


static void
got_text (GObject      *source,
          GAsyncResult *result,
          gpointer      user_data)
{
  g_autoptr (KgxTerminal) self = user_data;
  g_autoptr (GError) error = NULL;
  g_autofree char *text = NULL;

  /* Get the resulting text of the read operation */
  text = gdk_clipboard_read_text_finish (GDK_CLIPBOARD (source), result, &error);

  if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
    return;
  } else if (error) {
    g_critical ("Couldn't paste text: %s\n", error->message);
    return;
  }

  kgx_terminal_accept_paste (self, text);
}


static void
paste_activated (KgxTerminal *self)
{
  GdkClipboard *cb = gtk_widget_get_clipboard (GTK_WIDGET (self));

  gdk_clipboard_read_text_async (cb,
                                 self->cancellable,
                                 got_text,
                                 g_object_ref (self));
}


static void
select_all_activated (KgxTerminal *self)
{
  vte_terminal_select_all (VTE_TERMINAL (self));
}


static void
did_show_item (GObject *source, GAsyncResult *res, gpointer user_data)
{
  g_autoptr (GError) error = NULL;

  kgx_despatcher_show_item_finish (KGX_DESPATCHER (source), res, &error);

  if (error) {
    g_warning ("term.show-in-files: show item failed: %s", error->message);
  }
}


static void
did_show_folder (GObject *source, GAsyncResult *res, gpointer user_data)
{
  g_autoptr (GError) error = NULL;

  kgx_despatcher_show_folder_finish (KGX_DESPATCHER (source), res, &error);

  if (error) {
    g_warning ("term.show-in-files: show folder failed: %s", error->message);
  }
}


static void
show_in_files_activated (KgxTerminal *self)
{
  g_autofree char *uri = NULL;
  GtkWindow *parent = GTK_WINDOW (gtk_widget_get_root (GTK_WIDGET (self)));

  ensure_despatcher (self);

  uri = get_file_uri_prop (VTE_TERMINAL (self));

  if (G_UNLIKELY (uri)) {
    kgx_despatcher_show_item (self->despatcher,
                              uri,
                              parent,
                              self->cancellable,
                              did_show_item,
                              NULL);
    return;
  }

  uri = get_directory_uri_prop (VTE_TERMINAL (self));

  if (G_UNLIKELY (!uri)) {
    g_warning ("term.show-in-files: no file");
    return;
  }

  kgx_despatcher_show_folder (self->despatcher,
                              uri,
                              parent,
                              self->cancellable,
                              did_show_folder,
                              NULL);
}


static void
kgx_terminal_size_allocate (GtkWidget *widget,
                            int        width,
                            int        height,
                            int        baseline)
{
  int          rows;
  int          cols;
  KgxTerminal *self = KGX_TERMINAL (widget);
  VteTerminal *term = VTE_TERMINAL (self);

  GTK_WIDGET_CLASS (kgx_terminal_parent_class)->size_allocate (widget, width, height, baseline);

  rows = vte_terminal_get_row_count (term);
  cols = vte_terminal_get_column_count (term);

  g_signal_emit (self, signals[SIZE_CHANGED], 0, rows, cols);
}


static gboolean
kgx_terminal_query_tooltip (GtkWidget  *widget,
                            int         x,
                            int         y,
                            gboolean    keyboard_tooltip,
                            GtkTooltip *tooltip)
{
  KgxTerminal *self = KGX_TERMINAL (widget);
  g_autofree char *text = NULL;

  if (keyboard_tooltip || !have_url_under_pointer (self, x, y)) {
    return GTK_WIDGET_CLASS (kgx_terminal_parent_class)->query_tooltip (widget,
                                                                        x,
                                                                        y,
                                                                        keyboard_tooltip,
                                                                        tooltip);
  }

  text = g_strdup_printf (_("Ctrl-click to open:\n%s"), self->current_url);

  gtk_tooltip_set_text (tooltip, text);

  return TRUE;
}


static void
kgx_terminal_selection_changed (VteTerminal *self)
{
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "term.copy",
                                 vte_terminal_get_has_selection (self));
}


static void
kgx_terminal_increase_font_size (VteTerminal *self)
{
  g_signal_emit (self, signals[ZOOM], 0, KGX_ZOOM_IN);
}


static void
kgx_terminal_decrease_font_size (VteTerminal *self)
{
  g_signal_emit (self, signals[ZOOM], 0, KGX_ZOOM_OUT);
}


static KgxPalette *
resolve_livery (KgxTerminal *self,
                KgxLivery   *livery,
                gboolean     is_day,
                gboolean     translucency)
{
  return kgx_livery_resolve (livery, is_day, translucency);
}


static KgxTheme
resolve_theme (KgxTerminal *self,
               KgxSettings *settings,
               gboolean     dark_environment)
{
  return kgx_settings_resolve_theme (settings, dark_environment);
}


static gboolean
enum_is (KgxTerminal *self, int a, int b)
{
  return a == b;
}


static void
location_changed (KgxTerminal *self)
{
  g_autofree char *uri = get_either_uri_prop (VTE_TERMINAL (self));

  gtk_widget_action_set_enabled (GTK_WIDGET (self),
                                 "term.show-in-files",
                                 uri != NULL);

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_PATH]);
}


static void
setup_context_menu (KgxTerminal *self, VteEventContext *context)
{
  gboolean enabled;

  if (context) {
    double x, y;

    vte_event_context_get_coordinates (context, &x, &y);

    enabled = have_url_under_pointer (self, x, y);
  } else {
    enabled = FALSE;
  }

  gtk_widget_action_set_enabled (GTK_WIDGET (self), "term.open-link", enabled);
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "term.copy-link", enabled);
}


static void
pressed (GtkGestureClick *gesture,
         int              n_presses,
         double           x,
         double           y,
         KgxTerminal     *self)
{
  GdkModifierType state;
  guint button;

  if (n_presses > 1) {
    gtk_gesture_set_state (GTK_GESTURE (gesture), GTK_EVENT_SEQUENCE_DENIED);
    return;
  }

  state = gtk_event_controller_get_current_event_state (GTK_EVENT_CONTROLLER (gesture));
  button = gtk_gesture_single_get_current_button (GTK_GESTURE_SINGLE (gesture));

  if (have_url_under_pointer (self, x, y) &&
      (button == GDK_BUTTON_PRIMARY || button == GDK_BUTTON_MIDDLE) &&
      state & GDK_CONTROL_MASK) {

    open_link (self);
    gtk_gesture_set_state (GTK_GESTURE (gesture), GTK_EVENT_SEQUENCE_CLAIMED);

    return;
  }

  gtk_gesture_set_state (GTK_GESTURE (gesture), GTK_EVENT_SEQUENCE_DENIED);
}


static gboolean
scroll (GtkEventControllerScroll *scroll,
        double                   dx,
        double                   dy,
        KgxTerminal              *self)
{
  GdkModifierType mods = gtk_event_controller_get_current_event_state (GTK_EVENT_CONTROLLER (scroll));

  if ((mods & GDK_CONTROL_MASK) == 0 || dy == 0) {
    return FALSE;
  }

  g_signal_emit (self, signals[ZOOM], 0, dy > 0 ? KGX_ZOOM_OUT : KGX_ZOOM_IN);

  return TRUE;
}


static void
kgx_terminal_class_init (KgxTerminalClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  VteTerminalClass *term_class = VTE_TERMINAL_CLASS (klass);

  object_class->dispose = kgx_terminal_dispose;
  object_class->set_property = kgx_terminal_set_property;
  object_class->get_property = kgx_terminal_get_property;

  widget_class->size_allocate = kgx_terminal_size_allocate;
  widget_class->query_tooltip = kgx_terminal_query_tooltip;

  term_class->selection_changed = kgx_terminal_selection_changed;
  term_class->increase_font_size = kgx_terminal_increase_font_size;
  term_class->decrease_font_size = kgx_terminal_decrease_font_size;

  pspecs[PROP_SETTINGS] =
    g_param_spec_object ("settings", NULL, NULL,
                         KGX_TYPE_SETTINGS,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  pspecs[PROP_CANCELLABLE] =
    g_param_spec_object ("cancellable", NULL, NULL,
                         G_TYPE_CANCELLABLE,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  pspecs[PROP_PATH] =
    g_param_spec_object ("path", NULL, NULL,
                         G_TYPE_FILE,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_PALETTE] =
    g_param_spec_boxed ("palette", NULL, NULL,
                        KGX_TYPE_PALETTE,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

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

  gtk_widget_class_set_template_from_resource (widget_class,
                                               KGX_APPLICATION_PATH "kgx-terminal.ui");

  gtk_widget_class_bind_template_callback (widget_class, resolve_livery);
  gtk_widget_class_bind_template_callback (widget_class, resolve_theme);
  gtk_widget_class_bind_template_callback (widget_class, enum_is);
  gtk_widget_class_bind_template_callback (widget_class, location_changed);
  gtk_widget_class_bind_template_callback (widget_class, setup_context_menu);
  gtk_widget_class_bind_template_callback (widget_class, pressed);
  gtk_widget_class_bind_template_callback (widget_class, scroll);
  gtk_widget_class_bind_template_callback (widget_class, kgx_style_manager_for_display);

  gtk_widget_class_install_action (widget_class,
                                   "term.open-link",
                                   NULL,
                                   (GtkWidgetActionActivateFunc) open_link_activated);
  gtk_widget_class_install_action (widget_class,
                                   "term.copy-link",
                                   NULL,
                                   (GtkWidgetActionActivateFunc) copy_link_activated);
  gtk_widget_class_install_action (widget_class,
                                   "term.copy",
                                   NULL,
                                   (GtkWidgetActionActivateFunc) copy_activated);
  gtk_widget_class_install_action (widget_class,
                                   "term.paste",
                                   NULL,
                                   (GtkWidgetActionActivateFunc) paste_activated);
  gtk_widget_class_install_action (widget_class,
                                   "term.select-all",
                                   NULL,
                                   (GtkWidgetActionActivateFunc) select_all_activated);
  gtk_widget_class_install_action (widget_class,
                                   "term.show-in-files",
                                   NULL,
                                   (GtkWidgetActionActivateFunc) show_in_files_activated);
}


static void
kgx_terminal_init (KgxTerminal *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_widget_action_set_enabled (GTK_WIDGET (self), "term.open-link", FALSE);
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "term.copy-link", FALSE);
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "term.copy", FALSE);
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "term.show-in-files", FALSE);

  vte_terminal_set_mouse_autohide (VTE_TERMINAL (self), TRUE);
  vte_terminal_search_set_wrap_around (VTE_TERMINAL (self), TRUE);

  for (int i = 0; i < KGX_TERMINAL_N_LINK_REGEX; i++) {
    g_autoptr (VteRegex) regex = NULL;
    g_autoptr (GError) error = NULL;

    regex = vte_regex_new_for_match (links[i], -1, PCRE2_MULTILINE, &error);

    if (error) {
      g_warning ("link regex failed: %s", error->message);
      continue;
    }

    self->match_id[i] = vte_terminal_match_add_regex (VTE_TERMINAL (self),
                                                      regex,
                                                      0);

    vte_terminal_match_set_cursor_name (VTE_TERMINAL (self),
                                        self->match_id[i],
                                        "pointer");
  }
}


static void
got_paste (GObject      *source,
           GAsyncResult *res,
           gpointer      user_data)
{
  g_autoptr (KgxPasteDialog) dialogue = KGX_PASTE_DIALOG (source);
  g_autoptr (KgxTerminal) self = user_data;
  g_autoptr (GError) error = NULL;
  KgxPasteDialogResult result;
  const char *content;

  result = kgx_paste_dialog_run_finish (dialogue, res, &content, &error);

  if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
    result = KGX_PASTE_CANCELLED;
  } else if (error) {
    g_critical ("terminal [paste]: Unexpected: %s", error->message);
    return;
  }

  if (result == KGX_PASTE_CANCELLED) {
    return;
  }

  vte_terminal_paste_text (VTE_TERMINAL (self), content);
}


void
kgx_terminal_accept_paste (KgxTerminal *self,
                           const char  *text)
{
  g_autofree char *striped = g_strchug (g_strdup (text));
  size_t len;

  g_return_if_fail (KGX_IS_TERMINAL (self));

  if (G_UNLIKELY (!text || !text[0])) {
    return;
  }

  len = strlen (striped);

  if (g_strstr_len (striped, len, "sudo") != NULL &&
      g_strstr_len (striped, len, "\n") != NULL) {
    kgx_paste_dialog_run (g_object_new (KGX_TYPE_PASTE_DIALOG,
                                        "content", text,
                                        NULL),
                          GTK_WIDGET (self),
                          self->cancellable,
                          got_paste,
                          g_object_ref (self));
  } else {
    vte_terminal_paste_text (VTE_TERMINAL (self), text);
  }
}
