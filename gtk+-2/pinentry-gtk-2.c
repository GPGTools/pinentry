/* pinentry-gtk-2.c
   Copyright (C) 1999 Robert Bihlmeyer <robbe@orcus.priv.at>
   Copyright (C) 2001, 2002, 2007, 2015 g10 Code GmbH
   Copyright (C) 2004 by Albrecht Dreß <albrecht.dress@arcor.de>

   pinentry-gtk-2 is a pinentry application for the Gtk+-2 widget set.
   It tries to follow the Gnome Human Interface Guide as close as
   possible.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <gdk/gdkkeysyms.h>
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7 )
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wstrict-prototypes"
#endif
#include <gtk/gtk.h>
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7 )
# pragma GCC diagnostic pop
#endif
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gpg-error.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
#include "getopt.h"
#endif				/* HAVE_GETOPT_H */

#include "pinentry.h"

#ifdef FALLBACK_CURSES
#include "pinentry-curses.h"
#endif


#define PGMNAME "pinentry-gtk2"

#ifndef VERSION
#  define VERSION
#endif

static pinentry_t pinentry;
static int grab_failed;
static int passphrase_ok;
typedef enum { CONFIRM_CANCEL, CONFIRM_OK, CONFIRM_NOTOK } confirm_value_t;
static confirm_value_t confirm_value;

static GtkWindow *mainwindow;
static GtkWidget *entry;
static GtkWidget *repeat_entry;
static GtkWidget *error_label;
static GtkWidget *qualitybar;
#if !GTK_CHECK_VERSION (2, 12, 0)
static GtkTooltips *tooltips;
#endif
static gboolean got_input;
static guint timeout_source;
static int confirm_mode;

/* Gnome hig small and large space in pixels.  */
#define HIG_TINY       2
#define HIG_SMALL      6
#define HIG_LARGE     12

/* The text shown in the quality bar when no text is shown.  This is not
 * the empty string, because with an empty string the height of
 * the quality bar is less than with a non-empty string.  This results
 * in ugly layout changes when the text changes from non-empty to empty
 * and vice versa.  */
#define QUALITYBAR_EMPTY_TEXT " "


/* Constrain size of the window the window should not shrink beyond
   the requisition, and should not grow vertically.  */
static void
constrain_size (GtkWidget *win, GtkRequisition *req, gpointer data)
{
  static gint width, height;
  GdkGeometry geo;

  (void)data;

  if (req->width == width && req->height == height)
    return;
  width = req->width;
  height = req->height;
  geo.min_width = width;
  /* This limit is arbitrary, but INT_MAX breaks other things */
  geo.max_width = 10000;
  geo.min_height = geo.max_height = height;
  gtk_window_set_geometry_hints (GTK_WINDOW (win), NULL, &geo,
				 GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE);
}


/* Realize the window as transient if we grab the keyboard.  This
   makes the window a modal dialog to the root window, which helps the
   window manager.  See the following quote from:
   https://standards.freedesktop.org/wm-spec/wm-spec-1.4.html#id2512420

   Implementing enhanced support for application transient windows

   If the WM_TRANSIENT_FOR property is set to None or Root window, the
   window should be treated as a transient for all other windows in
   the same group. It has been noted that this is a slight ICCCM
   violation, but as this behavior is pretty standard for many
   toolkits and window managers, and is extremely unlikely to break
   anything, it seems reasonable to document it as standard.  */

static void
make_transient (GtkWidget *win, GdkEvent *event, gpointer data)
{
  GdkScreen *screen;
  GdkWindow *root;

  (void)event;
  (void)data;

  if (! pinentry->grab)
    return;

  /* Make window transient for the root window.  */
  screen = gdk_screen_get_default ();
  root = gdk_screen_get_root_window (screen);
  gdk_window_set_transient_for (gtk_widget_get_window (win), root);
}


/* Convert GdkGrabStatus to string.  */
static const char *
grab_strerror (GdkGrabStatus status)
{
  switch (status) {
  case GDK_GRAB_SUCCESS: return "success";
  case GDK_GRAB_ALREADY_GRABBED: return "already grabbed";
  case GDK_GRAB_INVALID_TIME: return "invalid time";
  case GDK_GRAB_NOT_VIEWABLE: return "not viewable";
  case GDK_GRAB_FROZEN: return "frozen";
  }
  return "unknown";
}


/* Grab the keyboard for maximum security */
static int
grab_keyboard (GtkWidget *win, GdkEvent *event, gpointer data)
{
  GdkGrabStatus err;
  int tries = 0, max_tries = 4096;
  (void)data;

  if (! pinentry->grab)
    return FALSE;

  do
    err = gdk_keyboard_grab (gtk_widget_get_window (win),
                             FALSE, gdk_event_get_time (event));
  while (tries++ < max_tries && err == GDK_GRAB_NOT_VIEWABLE);

  if (err)
    {
      g_critical ("could not grab keyboard: %s (%d)",
                  grab_strerror (err), err);
      grab_failed = 1;
      gtk_main_quit ();
    }

  if (tries > 1)
    g_warning ("it took %d tries to grab the keyboard", tries);

  return FALSE;
}


/* Grab the pointer to prevent the user from accidentally locking
   herself out of her graphical interface.  */
static int
grab_pointer (GtkWidget *win, GdkEvent *event, gpointer data)
{
  GdkGrabStatus err;
  GdkCursor *cursor;
  int tries = 0, max_tries = 4096;
  (void)data;

  /* Change the cursor for the duration of the grab to indicate that
     something is going on.  */
  /* XXX: It would be nice to have a key cursor, unfortunately there
     is none readily available.  */
  cursor = gdk_cursor_new_for_display (gtk_widget_get_display (win),
                                       GDK_DOT);

  do
    err = gdk_pointer_grab (gtk_widget_get_window (win),
                            TRUE, 0 /* event mask */,
                            NULL /* confine to */,
                            cursor,
                            gdk_event_get_time (event));
  while (tries++ < max_tries && err == GDK_GRAB_NOT_VIEWABLE);

  if (err)
    {
      g_critical ("could not grab pointer: %s (%d)",
                  grab_strerror (err), err);
      grab_failed = 1;
      gtk_main_quit ();
    }

  if (tries > 1)
    g_warning ("it took %d tries to grab the pointer", tries);

  return FALSE;
}


/* Remove all grabs and restore the windows transient state.  */
static int
ungrab_inputs (GtkWidget *win, GdkEvent *event, gpointer data)
{
  (void)data;
  gdk_keyboard_ungrab (gdk_event_get_time (event));
  gdk_pointer_ungrab (gdk_event_get_time (event));
  /* Unmake window transient for the root window.  */
  /* gdk_window_set_transient_for cannot be used with parent = NULL to
     unset transient hint (unlike gtk_ version which can).  Replacement
     code is taken from gtk_window_transient_parent_unrealized.  */
  gdk_property_delete (gtk_widget_get_window (win),
                       gdk_atom_intern_static_string ("WM_TRANSIENT_FOR"));
  return FALSE;
}


static int
delete_event (GtkWidget *widget, GdkEvent *event, gpointer data)
{
  (void)widget;
  (void)event;
  (void)data;

  pinentry->close_button = 1;
  gtk_main_quit ();
  return TRUE;
}


/* A button was clicked.  DATA indicates which button was clicked
   (i.e., the appropriate action) and is either CONFIRM_CANCEL,
   CONFIRM_OK or CONFIRM_NOTOK.  */
static void
button_clicked (GtkWidget *widget, gpointer data)
{
  (void)widget;

  if (confirm_mode)
    {
      confirm_value = (confirm_value_t) data;
      gtk_main_quit ();

      return;
    }

  if (data)
    {
      const char *s, *s2;

      /* Okay button or enter used in text field.  */
      s = gtk_entry_get_text (GTK_ENTRY (entry));
      if (!s)
	s = "";

      if (pinentry->repeat_passphrase && repeat_entry)
        {
          s2 = gtk_entry_get_text (GTK_ENTRY (repeat_entry));
          if (!s2)
            s2 = "";
          if (strcmp (s, s2))
            {
              gtk_label_set_text (GTK_LABEL (error_label),
                                  pinentry->repeat_error_string?
                                  pinentry->repeat_error_string:
                                  "not correctly repeated");
              gtk_widget_grab_focus (entry);
              return; /* again */
            }
          pinentry->repeat_okay = 1;
        }

      passphrase_ok = 1;
      pinentry_setbufferlen (pinentry, strlen (s) + 1);
      if (pinentry->pin)
	strcpy (pinentry->pin, s);
    }
  gtk_main_quit ();
}


static void
enter_callback (GtkWidget *widget, GtkWidget *next_widget)
{
  if (next_widget)
    gtk_widget_grab_focus (next_widget);
  else
    button_clicked (widget, (gpointer) CONFIRM_OK);
}


static void
cancel_callback (GtkAccelGroup *acc, GObject *accelerable,
                 guint keyval, GdkModifierType modifier, gpointer data)
{
  (void)acc;
  (void)keyval;
  (void)modifier;
  (void)data;

  button_clicked (GTK_WIDGET (accelerable), (gpointer)CONFIRM_CANCEL);
}



static gchar *
pinentry_utf8_validate (gchar *text)
{
  gchar *result;

  if (!text)
    return NULL;

  if (g_utf8_validate (text, -1, NULL))
    return g_strdup (text);

  /* Failure: Assume that it was encoded in the current locale and
     convert it to utf-8.  */
  result = g_locale_to_utf8 (text, -1, NULL, NULL, NULL);
  if (!result)
    {
      gchar *p;

      result = p = g_strdup (text);
      while (!g_utf8_validate (p, -1, (const gchar **) &p))
	*p = '?';
    }
  return result;
}


/* Handler called for "changed".   We use it to update the quality
   indicator.  */
static void
changed_text_handler (GtkWidget *widget)
{
  char textbuf[50];
  const char *s;
  int length;
  int percent;
  GdkColor color = { 0, 0, 0, 0};

  got_input = TRUE;

  if (pinentry->repeat_passphrase && repeat_entry)
    {
      gtk_entry_set_text (GTK_ENTRY (repeat_entry), "");
      gtk_label_set_text (GTK_LABEL (error_label), "");
    }

  if (!qualitybar || !pinentry->quality_bar)
    return;

  s = gtk_entry_get_text (GTK_ENTRY (widget));
  if (!s)
    s = "";
  length = strlen (s);
  percent = length? pinentry_inq_quality (pinentry, s, length) : 0;
  if (!length)
    {
      strcpy(textbuf, QUALITYBAR_EMPTY_TEXT);
      color.red = 0xffff;
    }
  else if (percent < 0)
    {
      snprintf (textbuf, sizeof textbuf, "(%d%%)", -percent);
      textbuf[sizeof textbuf -1] = 0;
      color.red = 0xffff;
      percent = -percent;
    }
  else
    {
      snprintf (textbuf, sizeof textbuf, "%d%%", percent);
      textbuf[sizeof textbuf -1] = 0;
      color.green = 0xffff;
    }
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (qualitybar),
                                 (double)percent/100.0);
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (qualitybar), textbuf);
  gtk_widget_modify_bg (qualitybar, GTK_STATE_PRELIGHT, &color);
}


#ifdef HAVE_LIBSECRET
static void
may_save_passphrase_toggled (GtkWidget *widget, gpointer data)
{
  GtkToggleButton *button = GTK_TOGGLE_BUTTON (widget);
  pinentry_t ctx = (pinentry_t) data;

  ctx->may_cache_password = gtk_toggle_button_get_active (button);
}
#endif


/* Return TRUE if it is okay to unhide the entry.  */
static int
confirm_unhiding (void)
{
  const char *s;
  GtkWidget *dialog;
  int result;
  char *message, *show_btn_label;

  s = gtk_entry_get_text (GTK_ENTRY (entry));
  if (!s || !*s)
    return TRUE;  /* Nothing entered - go ahead an unhide.  */

  message = pinentry_utf8_validate (pinentry->default_cf_visi);
  if (!message)
    {
      message = g_strdup ("Do you really want to make "
                          "your passphrase visible on the screen?");
    }

  show_btn_label = pinentry_utf8_validate (pinentry->default_tt_visi);
  if (!show_btn_label)
    {
      show_btn_label = g_strdup ("Make passphrase visible");
    }

  dialog = gtk_message_dialog_new
    (GTK_WINDOW (mainwindow),
     GTK_DIALOG_MODAL,
     GTK_MESSAGE_WARNING,
     GTK_BUTTONS_NONE,
     "%s", message);
  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                          show_btn_label, GTK_RESPONSE_OK,
                          NULL);
  result = (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK);
  gtk_widget_destroy (dialog);
  g_free (message);
  g_free (show_btn_label);

  return result;
}


static void
show_hide_button_toggled (GtkWidget *widget, gpointer data)
{
  GtkToggleButton *button = GTK_TOGGLE_BUTTON (widget);
  GtkWidget *label = data;
  const char *text;
  char *tooltip;
  gboolean reveal;

  if (!gtk_toggle_button_get_active (button) || !confirm_unhiding ())
    {
      text = "<span font=\"Monospace\" size=\"xx-small\">abc</span>";
      tooltip = pinentry_utf8_validate (pinentry->default_tt_visi);
      if (!tooltip)
        {
          tooltip = g_strdup ("Make the passphrase visible");
        }
      gtk_toggle_button_set_active (button, FALSE);
      reveal = FALSE;
    }
  else
    {
      text = "<span font=\"Monospace\" size=\"xx-small\">***</span>";
      tooltip = pinentry_utf8_validate (pinentry->default_tt_hide);
      if (!tooltip)
        {
          tooltip = g_strdup ("Hide the passphrase");
        }
      reveal = TRUE;
    }

  gtk_entry_set_visibility (GTK_ENTRY (entry), reveal);
  if (repeat_entry)
    {
      gtk_entry_set_visibility (GTK_ENTRY (repeat_entry), reveal);
    }

  gtk_label_set_markup (GTK_LABEL(label), text);
  gtk_widget_set_tooltip_text (GTK_WIDGET(button), tooltip);
  g_free (tooltip);
}


static gboolean
timeout_cb (gpointer data)
{
  pinentry_t pe = (pinentry_t)data;
  if (!got_input)
    {
      gtk_main_quit ();
      if (pe)
        pe->specific_err = gpg_error (GPG_ERR_TIMEOUT);
    }

  /* Don't run again.  */
  timeout_source = 0;
  return FALSE;
}


static GtkWidget *
create_show_hide_button (void)
{
  GtkWidget *button, *label;

  label = gtk_label_new (NULL);
  button = gtk_toggle_button_new ();
  show_hide_button_toggled (button, label);
  gtk_container_add (GTK_CONTAINER (button), label);
  g_signal_connect (G_OBJECT (button), "toggled",
                    G_CALLBACK (show_hide_button_toggled),
                    label);

  return button;
}


static GtkWidget *
create_window (pinentry_t ctx)
{
  GtkWidget *w;
  GtkWidget *win, *box;
  GtkWidget *wvbox, *chbox, *bbox;
  GtkAccelGroup *acc;
  gchar *msg;

  repeat_entry = NULL;

#if !GTK_CHECK_VERSION (2, 12, 0)
  tooltips = gtk_tooltips_new ();
#endif

  /* FIXME: check the grabbing code against the one we used with the
     old gpg-agent */
  win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  mainwindow = GTK_WINDOW (win);
  acc = gtk_accel_group_new ();

  g_signal_connect (G_OBJECT (win), "delete_event",
		    G_CALLBACK (delete_event), NULL);

#if 0
  g_signal_connect (G_OBJECT (win), "destroy", G_CALLBACK (gtk_main_quit),
		    NULL);
#endif
  g_signal_connect (G_OBJECT (win), "size-request",
		    G_CALLBACK (constrain_size), NULL);
  if (!confirm_mode)
    {
      if (pinentry->grab)
	g_signal_connect (G_OBJECT (win),
			  "realize", G_CALLBACK (make_transient), NULL);

      /* We need to grab the keyboard when its visible! not when its
         mapped (there is a difference)  */
      g_object_set (G_OBJECT(win), "events",
                    GDK_VISIBILITY_NOTIFY_MASK | GDK_STRUCTURE_MASK, NULL);

      g_signal_connect (G_OBJECT (win),
			pinentry->grab
                        ? "visibility-notify-event"
                        : "focus-in-event",
			G_CALLBACK (grab_keyboard), NULL);
      if (pinentry->grab)
        g_signal_connect (G_OBJECT (win),
                          "visibility-notify-event",
                          G_CALLBACK (grab_pointer), NULL);
      g_signal_connect (G_OBJECT (win),
			pinentry->grab ? "unmap-event" : "focus-out-event",
			G_CALLBACK (ungrab_inputs), NULL);
    }
  gtk_window_add_accel_group (GTK_WINDOW (win), acc);

  wvbox = gtk_vbox_new (FALSE, HIG_LARGE * 2);
  gtk_container_add (GTK_CONTAINER (win), wvbox);
  gtk_container_set_border_width (GTK_CONTAINER (wvbox), HIG_LARGE);

  chbox = gtk_hbox_new (FALSE, HIG_LARGE);
  gtk_box_pack_start (GTK_BOX (wvbox), chbox, FALSE, FALSE, 0);

  w = gtk_image_new_from_stock (GTK_STOCK_DIALOG_AUTHENTICATION,
					       GTK_ICON_SIZE_DIALOG);
  gtk_misc_set_alignment (GTK_MISC (w), 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (chbox), w, FALSE, FALSE, 0);

  box = gtk_vbox_new (FALSE, HIG_SMALL);
  gtk_box_pack_start (GTK_BOX (chbox), box, TRUE, TRUE, 0);

  if (pinentry->title)
    {
      msg = pinentry_utf8_validate (pinentry->title);
      gtk_window_set_title (GTK_WINDOW(win), msg);
    }
  if (pinentry->description)
    {
      msg = pinentry_utf8_validate (pinentry->description);
      w = gtk_label_new (msg);
      g_free (msg);
      gtk_misc_set_alignment (GTK_MISC (w), 0.0, 0.5);
      gtk_label_set_line_wrap (GTK_LABEL (w), TRUE);
      gtk_box_pack_start (GTK_BOX (box), w, TRUE, FALSE, 0);
    }
  if (!confirm_mode && (pinentry->error || pinentry->repeat_passphrase))
    {
      /* With the repeat passphrase option we need to create the label
         in any case so that it may later be updated by the error
         message.  */
      GdkColor color = { 0, 0xffff, 0, 0 };

      if (pinentry->error)
        msg = pinentry_utf8_validate (pinentry->error);
      else
        msg = "";
      error_label = gtk_label_new (msg);
      if (pinentry->error)
        g_free (msg);
      gtk_misc_set_alignment (GTK_MISC (error_label), 0.0, 0.5);
      gtk_label_set_line_wrap (GTK_LABEL (error_label), TRUE);
      gtk_box_pack_start (GTK_BOX (box), error_label, TRUE, FALSE, 0);
      gtk_widget_modify_fg (error_label, GTK_STATE_NORMAL, &color);
    }

  qualitybar = NULL;

  if (!confirm_mode)
    {
      int nrow;
      GtkWidget *table, *hbox;

      nrow = 1;
      if (pinentry->quality_bar)
        nrow++;
      if (pinentry->repeat_passphrase)
        nrow++;

      table = gtk_table_new (nrow, 2, FALSE);
      nrow = 0;
      gtk_box_pack_start (GTK_BOX (box), table, FALSE, FALSE, 0);

      if (pinentry->prompt)
	{
	  msg = pinentry_utf8_validate (pinentry->prompt);
	  w = gtk_label_new_with_mnemonic (msg);
	  g_free (msg);
	  gtk_misc_set_alignment (GTK_MISC (w), 1.0, 0.5);
	  gtk_table_attach (GTK_TABLE (table), w, 0, 1, nrow, nrow+1,
			    GTK_FILL, GTK_FILL, 4, 0);
	}

      entry = gtk_entry_new ();
      gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
      /* Allow the user to set a narrower invisible character than the
         large dot currently used by GTK.  Examples are "•★Ⓐ" */
      if (pinentry->invisible_char)
        {
          gunichar *uch;
          /*""*/
          uch = g_utf8_to_ucs4 (pinentry->invisible_char, -1, NULL, NULL, NULL);
          if (uch)
            {
              gtk_entry_set_invisible_char (GTK_ENTRY (entry), *uch);
              g_free (uch);
            }
        }

      gtk_widget_set_size_request (entry, 200, -1);
      g_signal_connect (G_OBJECT (entry), "changed",
                        G_CALLBACK (changed_text_handler), entry);
      hbox = gtk_hbox_new (FALSE, HIG_TINY);
      gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
      /* There was a wish in issue #2139 that this button should not
         be part of the tab order (focus_order).
         This should still be added. */
      w = create_show_hide_button ();
      gtk_box_pack_end (GTK_BOX (hbox), w, FALSE, FALSE, 0);
      gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, nrow, nrow+1,
                        GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 0, 0);
      gtk_widget_show (entry);
      nrow++;

      if (pinentry->quality_bar)
	{
          msg = pinentry_utf8_validate (pinentry->quality_bar);
	  w = gtk_label_new (msg);
          g_free (msg);
	  gtk_misc_set_alignment (GTK_MISC (w), 1.0, 0.5);
	  gtk_table_attach (GTK_TABLE (table), w, 0, 1, nrow, nrow+1,
			    GTK_FILL, GTK_FILL, 4, 0);
	  qualitybar = gtk_progress_bar_new();
	  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (qualitybar),
				     QUALITYBAR_EMPTY_TEXT);
	  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (qualitybar), 0.0);
          if (pinentry->quality_bar_tt)
	    {
#if !GTK_CHECK_VERSION (2, 12, 0)
	      gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips), qualitybar,
				    pinentry->quality_bar_tt, "");
#else
	      gtk_widget_set_tooltip_text (qualitybar,
					   pinentry->quality_bar_tt);
#endif
	    }
	  gtk_table_attach (GTK_TABLE (table), qualitybar, 1, 2, nrow, nrow+1,
	  		    GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 0, 0);
          nrow++;
	}


      if (pinentry->repeat_passphrase)
        {
	  msg = pinentry_utf8_validate (pinentry->repeat_passphrase);
	  w = gtk_label_new (msg);
	  g_free (msg);
	  gtk_misc_set_alignment (GTK_MISC (w), 1.0, 0.5);
	  gtk_table_attach (GTK_TABLE (table), w, 0, 1, nrow, nrow+1,
			    GTK_FILL, GTK_FILL, 4, 0);

          repeat_entry = gtk_entry_new ();
	  gtk_entry_set_visibility (GTK_ENTRY (repeat_entry), FALSE);
          gtk_widget_set_size_request (repeat_entry, 200, -1);
          gtk_table_attach (GTK_TABLE (table), repeat_entry, 1, 2, nrow, nrow+1,
                            GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 0, 0);
          gtk_widget_show (repeat_entry);
          nrow++;

	  g_signal_connect (G_OBJECT (repeat_entry), "activate",
			    G_CALLBACK (enter_callback), NULL);
        }

      /* When the user presses enter in the entry widget, the widget
	 is activated.  If we have a repeat entry, send the focus to
	 it.  Otherwise, activate the "Ok" button.  */
      g_signal_connect (G_OBJECT (entry), "activate",
			G_CALLBACK (enter_callback), repeat_entry);
    }

  bbox = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_END);
  gtk_box_set_spacing (GTK_BOX (bbox), 6);
  gtk_box_pack_start (GTK_BOX (wvbox), bbox, TRUE, FALSE, 0);

#ifdef HAVE_LIBSECRET
  if (ctx->allow_external_password_cache && ctx->keyinfo)
    /* Only show this if we can cache passwords and we have a stable
       key identifier.  */
    {
      if (pinentry->default_pwmngr)
        {
          msg = pinentry_utf8_validate (pinentry->default_pwmngr);
          w = gtk_check_button_new_with_mnemonic (msg);
          g_free (msg);
        }
      else
        w = gtk_check_button_new_with_label ("Save passphrase using libsecret");

      /* Make sure it is off by default.  */
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), FALSE);

      gtk_box_pack_start (GTK_BOX (box), w, TRUE, FALSE, 0);
      gtk_widget_show (w);

      g_signal_connect (G_OBJECT (w), "toggled",
                        G_CALLBACK (may_save_passphrase_toggled),
			(gpointer) ctx);
    }
#endif

  if (!pinentry->one_button)
    {
      if (pinentry->cancel)
        {
          msg = pinentry_utf8_validate (pinentry->cancel);
          w = gtk_button_new_with_mnemonic (msg);
          g_free (msg);
        }
      else if (pinentry->default_cancel)
        {
          GtkWidget *image;

          msg = pinentry_utf8_validate (pinentry->default_cancel);
          w = gtk_button_new_with_mnemonic (msg);
          g_free (msg);
          image = gtk_image_new_from_stock (GTK_STOCK_CANCEL,
                                            GTK_ICON_SIZE_BUTTON);
          if (image)
            gtk_button_set_image (GTK_BUTTON (w), image);
        }
      else
	w = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
      gtk_container_add (GTK_CONTAINER (bbox), w);
      g_signal_connect (G_OBJECT (w), "clicked",
                        G_CALLBACK (button_clicked),
			(gpointer) CONFIRM_CANCEL);

      gtk_accel_group_connect (acc, GDK_KEY_Escape, 0, 0,
			       g_cclosure_new (G_CALLBACK (cancel_callback),
					       NULL, NULL));
    }

  if (confirm_mode && !pinentry->one_button && pinentry->notok)
    {
      msg = pinentry_utf8_validate (pinentry->notok);
      w = gtk_button_new_with_mnemonic (msg);
      g_free (msg);

      gtk_container_add (GTK_CONTAINER (bbox), w);
      g_signal_connect (G_OBJECT (w), "clicked",
                        G_CALLBACK (button_clicked),
			(gpointer) CONFIRM_NOTOK);
    }

  if (pinentry->ok)
    {
      msg = pinentry_utf8_validate (pinentry->ok);
      w = gtk_button_new_with_mnemonic (msg);
      g_free (msg);
    }
  else if (pinentry->default_ok)
    {
      GtkWidget *image;

      msg = pinentry_utf8_validate (pinentry->default_ok);
      w = gtk_button_new_with_mnemonic (msg);
      g_free (msg);
      image = gtk_image_new_from_stock (GTK_STOCK_OK,
                                        GTK_ICON_SIZE_BUTTON);
      if (image)
        gtk_button_set_image (GTK_BUTTON (w), image);
    }
  else
    w = gtk_button_new_from_stock (GTK_STOCK_OK);
  gtk_container_add (GTK_CONTAINER(bbox), w);
  if (!confirm_mode)
    {
      GTK_WIDGET_SET_FLAGS (w, GTK_CAN_DEFAULT);
      gtk_widget_grab_default (w);
    }

  g_signal_connect (G_OBJECT (w), "clicked",
		    G_CALLBACK(button_clicked),
		    (gpointer) CONFIRM_OK);

  gtk_window_set_position (GTK_WINDOW (win), GTK_WIN_POS_CENTER);
  gtk_window_set_keep_above (GTK_WINDOW (win), TRUE);
  gtk_widget_show_all (win);
  gtk_window_present (GTK_WINDOW (win));  /* Make sure it has the focus.  */

  if (pinentry->timeout > 0)
    timeout_source = g_timeout_add (pinentry->timeout*1000, timeout_cb, pinentry);

  return win;
}


static int
gtk_cmd_handler (pinentry_t pe)
{
  GtkWidget *w;
  int want_pass = !!pe->pin;

  got_input = FALSE;
  pinentry = pe;
  confirm_value = CONFIRM_CANCEL;
  passphrase_ok = 0;
  confirm_mode = want_pass ? 0 : 1;
  w = create_window (pe);
  gtk_main ();
  gtk_widget_destroy (w);
  while (gtk_events_pending ())
    gtk_main_iteration ();

  if (timeout_source)
    /* There is a timer running.  Cancel it.  */
    {
      g_source_remove (timeout_source);
      timeout_source = 0;
    }

  if (confirm_value == CONFIRM_CANCEL || grab_failed)
    pe->canceled = 1;

  pinentry = NULL;
  if (want_pass)
    {
      if (passphrase_ok && pe->pin)
	return strlen (pe->pin);
      else
	return -1;
    }
  else
    return (confirm_value == CONFIRM_OK) ? 1 : 0;
}


pinentry_cmd_handler_t pinentry_cmd_handler = gtk_cmd_handler;


int
main (int argc, char *argv[])
{
  pinentry_init (PGMNAME);

#ifdef FALLBACK_CURSES
  if (pinentry_have_display (argc, argv))
    {
      if (! gtk_init_check (&argc, &argv))
	pinentry_cmd_handler = curses_cmd_handler;
    }
  else
    pinentry_cmd_handler = curses_cmd_handler;
#else
  gtk_init (&argc, &argv);
#endif

  pinentry_parse_opts (argc, argv);

  if (pinentry_loop ())
    return 1;

  return 0;
}
