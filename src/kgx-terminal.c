/* kgx-terminal.c
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
 * SECTION:kgx-terminal
 * @title: KgxTerminal
 * @short_description: The terminal
 *
 * The main terminal widget with various features added such as a context
 * menu (via #GtkPopover) and link detection
 */

#include <glib/gi18n.h>
#include <handy.h>
#include <vte/vte.h>
#define PCRE2_CODE_UNIT_WIDTH 0
#include <pcre2.h>

#include "rgba.h"

#include "kgx-config.h"
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

static const gchar* links[KGX_TERMINAL_N_LINK_REGEX] = {
  SCHEME "//(?:" USERPASS "\\@)?" HOST PORT URLPATH,
  "(?:www|ftp)" HOSTCHARS_CLASS "*\\." HOST PORT URLPATH,
  "(?:callto:|h323:|sip:)" USERCHARS_CLASS "[" USERCHARS ".]*(?:" PORT "/[a-z0-9]+)?\\@" HOST,
  "(?:mailto:)?" USERCHARS_CLASS "[" USERCHARS ".]*\\@" HOSTCHARS_CLASS "+\\." HOST,
  "(?:news:|man:|info:)[[:alnum:]\\Q^_{|}~!\"#$%&'()*+,./;:=?`\\E]+"
};

/*       Regex adapted from TerminalWidget.vala in Pantheon Terminal       */

G_DEFINE_TYPE (KgxTerminal, kgx_terminal, VTE_TYPE_TERMINAL)

enum {
  PROP_0,
  PROP_THEME,
  PROP_OPAQUE,
  PROP_PATH,
  LAST_PROP
};

static GParamSpec *pspecs[LAST_PROP] = { NULL, };

enum {
  SIZE_CHANGED,
  N_SIGNALS
};
static guint signals[N_SIGNALS];


static void
kgx_terminal_dispose (GObject *object)
{
  KgxTerminal *self = KGX_TERMINAL (object);

  g_clear_object (&self->actions);
  g_clear_pointer (&self->current_url, g_free);
  g_clear_object (&self->long_press_gesture);

  if (self->menu) {
    gtk_menu_detach (GTK_MENU (self->menu));
    self->menu = NULL;
  }

  if (self->touch_menu) {
    gtk_popover_set_relative_to (GTK_POPOVER (self->touch_menu), NULL);
    self->touch_menu = NULL;
  }

  G_OBJECT_CLASS (kgx_terminal_parent_class)->dispose (object);
}


static void
update_terminal_colors (KgxTerminal *self)
{
  KgxTheme resolved_theme;
  GdkRGBA fg;
  GdkRGBA bg;

  // Workings of GDK_RGBA prevent this being static
  GdkRGBA palette[16] = {
    GDK_RGBA ("241f31"), // Black
    GDK_RGBA ("c01c28"), // Red
    GDK_RGBA ("2ec27e"), // Green
    GDK_RGBA ("f5c211"), // Yellow
    GDK_RGBA ("1e78e4"), // Blue
    GDK_RGBA ("9841bb"), // Magenta
    GDK_RGBA ("0ab9dc"), // Cyan
    GDK_RGBA ("c0bfbc"), // White
    GDK_RGBA ("5e5c64"), // Bright Black
    GDK_RGBA ("ed333b"), // Bright Red
    GDK_RGBA ("57e389"), // Bright Green
    GDK_RGBA ("f8e45c"), // Bright Yellow
    GDK_RGBA ("51a1ff"), // Bright Blue
    GDK_RGBA ("c061cb"), // Bright Magenta
    GDK_RGBA ("4fd2fd"), // Bright Cyan
    GDK_RGBA ("f6f5f4"), // Bright White
  };

  if (self->theme == KGX_THEME_AUTO) {
    HdyStyleManager *manager = hdy_style_manager_get_default ();

    if (hdy_style_manager_get_dark (manager)) {
      resolved_theme = KGX_THEME_NIGHT;
    } else {
      resolved_theme = KGX_THEME_DAY;
    }
  } else {
    resolved_theme = self->theme;
  }

  switch (resolved_theme) {
    case KGX_THEME_HACKER:
      fg = (GdkRGBA) { 0.1, 1.0, 0.1, 1.0};
      bg = (GdkRGBA) { 0.05, 0.05, 0.05, 0.96 };
      break;
    case KGX_THEME_DAY:
      fg = (GdkRGBA) { 0.0, 0.0, 0.0, 0.0 };
      bg = (GdkRGBA) { 1.0, 1.0, 1.0, 1.0 };
      break;
    case KGX_THEME_NIGHT:
    case KGX_THEME_AUTO:
    default:
      fg = (GdkRGBA) { 1.0, 1.0, 1.0, 1.0};
      bg = (GdkRGBA) { 0.05, 0.05, 0.05, 0.96 };
      break;
  }

  if (self->opaque) {
    bg.alpha = 1.0;
  }

  vte_terminal_set_colors (VTE_TERMINAL (self), &fg, &bg, palette, 16);
}


static void
kgx_terminal_set_theme (KgxTerminal *self,
                        KgxTheme     theme,
                        gboolean     opaque)
{
  if (self->theme == theme && self->opaque == opaque) {
    return;
  }

  if (self->theme != theme) {
    self->theme = theme;
    g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_THEME]);
  }

  if (self->opaque != opaque) {
    self->opaque = opaque;
    g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_OPAQUE]);
  }

  update_terminal_colors (self);
}


static void
kgx_terminal_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  KgxTerminal *self = KGX_TERMINAL (object);

  switch (property_id) {
    case PROP_THEME:
      kgx_terminal_set_theme (self, g_value_get_enum (value), self->opaque);
      break;
    case PROP_OPAQUE:
      kgx_terminal_set_theme (self, self->theme, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
kgx_terminal_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  KgxTerminal *self = KGX_TERMINAL (object);
  const char *uri;
  g_autoptr (GFile) path = NULL;

  switch (property_id) {
    case PROP_THEME:
      g_value_set_enum (value, self->theme);
      break;
    case PROP_OPAQUE:
      g_value_set_boolean (value, self->opaque);
      break;
    case PROP_PATH:
      if ((uri = vte_terminal_get_current_file_uri (VTE_TERMINAL (self)))) {
        path = g_file_new_for_uri (uri);
      } else if ((uri = vte_terminal_get_current_directory_uri (VTE_TERMINAL (self)))) {
        path = g_file_new_for_uri (uri);
      }
      g_value_set_object (value, path);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static gboolean
have_url_under_pointer (KgxTerminal *self, GdkEvent *event)
{
  g_autofree char *hyperlink = NULL;
  g_autofree char *match = NULL;
  int match_id;

  g_clear_pointer (&self->current_url, g_free);

  hyperlink = vte_terminal_hyperlink_check_event (VTE_TERMINAL (self), event);

  if (G_UNLIKELY (hyperlink)) {
    self->current_url = g_steal_pointer (&hyperlink);
  } else {
    match = vte_terminal_match_check_event (VTE_TERMINAL (self),
                                            event,
                                            &match_id);

    for (int i = 0; i < KGX_TERMINAL_N_LINK_REGEX; i++) {
      if (self->match_id[i] == match_id) {
        self->current_url = g_steal_pointer (&match);
        break;
      }
    }
  }

  return self->current_url != NULL;
}


static void
context_menu_detach (KgxTerminal *self,
                     GtkMenu     *menu)
{
  self->menu = NULL;
}


static void
context_menu (GtkWidget *widget,
              int        x,
              int        y,
              gboolean   touch,
              GdkEvent  *event)
{
  KgxTerminal *self = KGX_TERMINAL (widget);
  GAction *act;
  GtkApplication *app;
  GMenu *model;
  gboolean value;

  value = have_url_under_pointer (self, event);

  act = g_action_map_lookup_action (G_ACTION_MAP (self->actions), "open-link");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (act), value);
  act = g_action_map_lookup_action (G_ACTION_MAP (self->actions), "copy-link");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (act), value);

  if (touch) {
    GdkRectangle rect = {x, y, 1, 1};

    if (!self->touch_menu) {
      app = GTK_APPLICATION (g_application_get_default ());
      model = gtk_application_get_menu_by_id (app, "context-menu");

      self->touch_menu = gtk_popover_new_from_model (widget, G_MENU_MODEL (model));
    }

    gtk_popover_set_pointing_to (GTK_POPOVER (self->touch_menu), &rect);
    gtk_popover_popup (GTK_POPOVER (self->touch_menu));
  } else {
    if (!self->menu) {
      app = GTK_APPLICATION (g_application_get_default ());
      model = gtk_application_get_menu_by_id (app, "context-menu");

      self->menu = gtk_menu_new_from_model (G_MENU_MODEL (model));
      gtk_style_context_add_class (gtk_widget_get_style_context (self->menu),
                                   GTK_STYLE_CLASS_CONTEXT_MENU);

      gtk_menu_attach_to_widget (GTK_MENU (self->menu), widget,
                                 (GtkMenuDetachFunc) context_menu_detach);
    }

    if (event && gdk_event_triggers_context_menu (event)) {
      gtk_menu_popup_at_pointer (GTK_MENU (self->menu), event);
    } else {
      gtk_menu_popup_at_widget (GTK_MENU (self->menu),
                                widget,
                                GDK_GRAVITY_SOUTH_WEST,
                                GDK_GRAVITY_NORTH_WEST,
                                event);

      gtk_menu_shell_select_first (GTK_MENU_SHELL (self->menu), FALSE);
    }
  }
}


static gboolean
kgx_terminal_popup_menu (GtkWidget *self)
{
  context_menu (self, 1, 1, FALSE, NULL);

  return TRUE;
}


static void
open_link (KgxTerminal *self, guint32 timestamp)
{
  g_autoptr (GError) error = NULL;

  gtk_show_uri_on_window (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (self))),
                          self->current_url,
                          timestamp,
                          &error);

  if (error) {
    g_warning ("Failed to open link %s", error->message);
  }
}


static gboolean
kgx_terminal_button_press_event (GtkWidget *widget, GdkEventButton *event)
{
  KgxTerminal *self = KGX_TERMINAL (widget);
  GdkModifierType state;

  if (gdk_event_triggers_context_menu ((GdkEvent *) event) &&
      event->type == GDK_BUTTON_PRESS) {
    context_menu (widget, event->x, event->y, FALSE, (GdkEvent *) event);

    return TRUE;
  }

  state = event->state & gtk_accelerator_get_default_mod_mask ();

  if (have_url_under_pointer (self, (GdkEvent *) event) &&
      (event->button == 1 || event->button == 2) &&
      state & GDK_CONTROL_MASK) {
    open_link (self, event->time);

    return GDK_EVENT_STOP;
  }

  return GTK_WIDGET_CLASS (kgx_terminal_parent_class)->button_press_event (widget,
                                                                           event);
}


static void
kgx_terminal_class_init (KgxTerminalClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = kgx_terminal_dispose;
  object_class->set_property = kgx_terminal_set_property;
  object_class->get_property = kgx_terminal_get_property;

  widget_class->popup_menu = kgx_terminal_popup_menu;
  widget_class->button_press_event = kgx_terminal_button_press_event;

  /**
   * KgxTerminal:theme:
   *
   * The palette to use, one of the values of #KgxTheme
   *
   * Officially only "night" exists, "hacker" is just a little fun
   *
   * Bound to #KgxApplication:theme on the #KgxApplication
   *
   * Stability: Private
   */
  pspecs[PROP_THEME] =
    g_param_spec_enum ("theme", "Theme", "Terminal theme",
                       KGX_TYPE_THEME, KGX_THEME_NIGHT,
                       G_PARAM_READWRITE);

  /**
   * KgxTerminal:opaque:
   *
   * Whether to disable transparency
   *
   * Bound to #GtkWindow:is-maximized on the #KgxWindow
   *
   * Stability: Private
   */
  pspecs[PROP_OPAQUE] =
    g_param_spec_boolean ("opaque", "Opaque", "Terminal opaqueness",
                          FALSE,
                          G_PARAM_READWRITE);

  /**
   * KgxTerminal:path:
   *
   *
   * Stability: Private
   */
  pspecs[PROP_PATH] =
    g_param_spec_object ("path", "Path", "Current path",
                         G_TYPE_FILE,
                         G_PARAM_READABLE);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);

  signals[SIZE_CHANGED] = g_signal_new ("size-changed",
                                        G_TYPE_FROM_CLASS (klass),
                                        G_SIGNAL_RUN_LAST,
                                        0, NULL, NULL, NULL,
                                        G_TYPE_NONE,
                                        2,
                                        G_TYPE_UINT,
                                        G_TYPE_UINT);
}


static void
open_link_activated (GSimpleAction *action,
                     GVariant      *parameter,
                     gpointer       data)
{
  open_link (KGX_TERMINAL (data), GDK_CURRENT_TIME);
}


static void
copy_link_activated (GSimpleAction *action,
                     GVariant      *parameter,
                     gpointer       data)
{
  GtkClipboard *cb;
  KgxTerminal *self = KGX_TERMINAL (data);

  cb = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);

  gtk_clipboard_set_text (cb, self->current_url, -1);
}

static void
copy_activated (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       data)
{
  vte_terminal_copy_clipboard_format (VTE_TERMINAL (data), VTE_FORMAT_TEXT);
}

struct Paste {
  VteTerminal *dest;
  char        *text;
};

static void
paste_response (GtkDialog    *dlg,
                int           response,
                struct Paste *paste)
{
  if (dlg && GTK_IS_DIALOG (dlg)) {
    gtk_widget_destroy (GTK_WIDGET (dlg));
  }

  if (response == GTK_RESPONSE_ACCEPT) {
    vte_terminal_paste_text (paste->dest, paste->text);
  }

  g_free (paste->text);
  g_free (paste);
}


static void
got_text (GtkClipboard *clipboard,
          const char   *text,
          gpointer      data)
{
  kgx_terminal_accept_paste (KGX_TERMINAL (data), text);
}


static void
paste_activated (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       data)
{
  GtkClipboard *cb;

  cb = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);

  gtk_clipboard_request_text (cb, got_text, data);
}

static void
select_all_activated (GSimpleAction *action,
                      GVariant      *parameter,
                      gpointer       data)
{
  vte_terminal_select_all (VTE_TERMINAL (data));
}


static void
show_in_files_activated (GSimpleAction *action,
                         GVariant      *parameter,
                         gpointer       data)
{
  KgxTerminal *self = KGX_TERMINAL (data);
  g_autoptr (GDBusProxy) proxy = NULL;
  g_autoptr (GVariant) retval = NULL;
  g_autoptr (GVariantBuilder) builder = NULL;
  g_autoptr (GError) error = NULL;
  const char *uri = NULL;
  const char *method;

  uri = vte_terminal_get_current_file_uri (VTE_TERMINAL (self));
  method = "ShowItems";

  if (uri == NULL) {
    uri = vte_terminal_get_current_directory_uri (VTE_TERMINAL (self));
    method = "ShowFolders";
  }

  if (uri == NULL) {
    g_warning ("win.show-in-files: no file");
    return;
  }

  proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                         G_DBUS_PROXY_FLAGS_NONE,
                                         NULL,
                                         "org.freedesktop.FileManager1",
                                         "/org/freedesktop/FileManager1",
                                         "org.freedesktop.FileManager1",
                                         NULL, &error);

  if (!proxy) {
    g_warning ("win.show-in-files: D-Bus connect failed %s", error->message);
    return;
  }

  builder = g_variant_builder_new (G_VARIANT_TYPE ("as"));
  g_variant_builder_add (builder, "s", uri);

  retval = g_dbus_proxy_call_sync (proxy,
                                   method,
                                   g_variant_new ("(ass)",
                                                  builder,
                                                  ""),
                                   G_DBUS_CALL_FLAGS_NONE,
                                   -1, NULL, &error);

  if (!retval) {
    g_warning ("win.show-in-files: D-Bus call failed %s", error->message);
    return;
  }
}


static GActionEntry term_entries[] = {
  { "open-link", open_link_activated, NULL, NULL, NULL },
  { "copy-link", copy_link_activated, NULL, NULL, NULL },
  { "copy", copy_activated, NULL, NULL, NULL },
  { "paste", paste_activated, NULL, NULL, NULL },
  { "select-all", select_all_activated, NULL, NULL, NULL },
  { "show-in-files", show_in_files_activated, NULL, NULL, NULL },
};


static void
long_pressed (GtkGestureLongPress *gesture,
              gdouble              x,
              gdouble              y,
              KgxTerminal         *self)
{
  context_menu (GTK_WIDGET (self), (int) x, (int) y, TRUE, NULL);
}


static void
selection_changed (KgxTerminal *self)
{
  GAction *act;

  act = g_action_map_lookup_action (self->actions, "copy");

  g_simple_action_set_enabled (G_SIMPLE_ACTION (act),
                               vte_terminal_get_has_selection (VTE_TERMINAL (self)));
}


static void
location_changed (KgxTerminal *self)
{
  GAction *act;
  gboolean value;

  act = g_action_map_lookup_action (self->actions, "show-in-files");

  value = vte_terminal_get_current_file_uri (VTE_TERMINAL (self)) ||
            vte_terminal_get_current_directory_uri (VTE_TERMINAL (self));

  g_simple_action_set_enabled (G_SIMPLE_ACTION (act), value);

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_PATH]);
}


static void
size_changed (GtkWidget    *widget,
              GdkRectangle *allocation)
{
  int          rows;
  int          cols;
  KgxTerminal *self = KGX_TERMINAL (widget);
  VteTerminal *term = VTE_TERMINAL (self);

  rows = vte_terminal_get_row_count (term);
  cols = vte_terminal_get_column_count (term);

  g_signal_emit (self, signals[SIZE_CHANGED], 0, rows, cols);
}


static void
dark_changed (KgxTerminal *self)
{
  if (self->theme == KGX_THEME_AUTO) {
    update_terminal_colors (self);
  }
}


static void
kgx_terminal_init (KgxTerminal *self)
{
  GAction *act;
  GtkGesture *gesture;

  self->actions = G_ACTION_MAP (g_simple_action_group_new ());
  g_action_map_add_action_entries (self->actions,
                                   term_entries,
                                   G_N_ELEMENTS (term_entries),
                                   self);
  gtk_widget_insert_action_group (GTK_WIDGET (self),
                                  "term",
                                  G_ACTION_GROUP (self->actions));

  gesture = gtk_gesture_long_press_new (GTK_WIDGET (self));
  gtk_gesture_single_set_touch_only (GTK_GESTURE_SINGLE (gesture), TRUE);
  gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (gesture),
                                              GTK_PHASE_TARGET);
  g_signal_connect (gesture, "pressed", G_CALLBACK (long_pressed), self);
  self->long_press_gesture = gesture;

  act = g_action_map_lookup_action (self->actions, "open-link");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (act), FALSE);
  act = g_action_map_lookup_action (self->actions, "copy-link");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (act), FALSE);
  act = g_action_map_lookup_action (self->actions, "copy");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (act), FALSE);
  act = g_action_map_lookup_action (self->actions, "show-in-files");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (act), FALSE);

  vte_terminal_set_mouse_autohide (VTE_TERMINAL (self), TRUE);
  vte_terminal_search_set_wrap_around (VTE_TERMINAL (self), TRUE);
  vte_terminal_set_allow_hyperlink (VTE_TERMINAL (self), TRUE);
  vte_terminal_set_enable_fallback_scrolling (VTE_TERMINAL (self), FALSE);
  vte_terminal_set_scroll_unit_is_pixels (VTE_TERMINAL (self), TRUE);

  g_signal_connect (self, "selection-changed",
                    G_CALLBACK (selection_changed), NULL);
  g_signal_connect (self, "current-directory-uri-changed",
                    G_CALLBACK (location_changed), NULL);
  g_signal_connect (self, "current-file-uri-changed",
                    G_CALLBACK (location_changed), NULL);
  g_signal_connect (self, "size-allocate",
                    G_CALLBACK (size_changed), NULL);

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

  g_signal_connect_object (hdy_style_manager_get_default (),
                           "notify::dark", G_CALLBACK (dark_changed),
                           self, G_CONNECT_SWAPPED);

  update_terminal_colors (self);
}


void
kgx_terminal_accept_paste (KgxTerminal *self,
                           const char  *text)
{
  g_autofree char *striped = g_strchug (g_strdup (text));
  struct Paste    *paste = g_new (struct Paste, 1);

  paste->dest = VTE_TERMINAL (self);
  paste->text = g_strdup (text);

  if (g_strstr_len (striped, -1, "sudo") != NULL &&
      g_strstr_len (striped, -1, "\n") != NULL) {
    GtkWidget *accept = NULL;
    GtkWidget *dlg = gtk_message_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (self))),
                                             GTK_DIALOG_MODAL,
                                             GTK_MESSAGE_QUESTION,
                                             GTK_BUTTONS_NONE,
                                             _("You are pasting a command that runs as an administrator"));
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg),
                                              // TRANSLATORS: %s is the command being pasted
                                              _("Make sure you know what the command does:\n%s"),
                                              text);

    g_signal_connect (dlg, "response", G_CALLBACK (paste_response), paste);
    gtk_dialog_add_button (GTK_DIALOG (dlg),
                           _("_Cancel"),
                           GTK_RESPONSE_DELETE_EVENT);
    accept = gtk_dialog_add_button (GTK_DIALOG (dlg),
                                    _("_Paste"),
                                    GTK_RESPONSE_ACCEPT);
    gtk_style_context_add_class (gtk_widget_get_style_context (accept),
                                 "destructive-action");
    gtk_widget_show (dlg);
  } else {
    paste_response (NULL, GTK_RESPONSE_ACCEPT, paste);
  }
}
