/* pinentry-gtk-2.c
   Copyright (C) 1999 Robert Bihlmeyer <robbe@orcus.priv.at>
   Copyright (C) 2001, 2002 g10 Code GmbH
   Copyright (C) 2004 by Albrecht Dreﬂ <albrecht.dress@arcor.de>

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
#include <gtk/gtk.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
#include "getopt.h"
#endif				/* HAVE_GETOPT_H */

#include "gtksecentry.h"
#include "pinentry.h"

#ifdef FALLBACK_CURSES
#include "pinentry-curses.h"
#endif


#include "padlock-keyhole.xpm"

#define PGMNAME "pinentry-gtk2"

#ifndef VERSION
#  define VERSION
#endif

static pinentry_t pinentry;
static int passphrase_ok;
static int confirm_yes;

static GtkWidget *entry;
static GtkWidget *insure;
static GtkWidget *time_out;

/* Gnome hig small and large space in pixels.  */
#define HIG_SMALL      6
#define HIG_LARGE     12


/* Constrain size of the window the window should not shrink beyond
   the requisition, and should not grow vertically.  */
static void
constrain_size (GtkWidget *win, GtkRequisition *req, gpointer data)
{
  static gint width, height;
  GdkGeometry geo;

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


/* Grab the keyboard for maximum security */
static void
grab_keyboard (GtkWidget *win, GdkEvent *event, gpointer data)
{
  if (!pinentry->grab)
    return;

  if (gdk_keyboard_grab (win->window, FALSE, gdk_event_get_time (event)))
    g_error ("could not grab keyboard");
}

/* Remove grab.  */
static void
ungrab_keyboard (GtkWidget *win, GdkEvent *event, gpointer data)
{
  gdk_keyboard_ungrab (gdk_event_get_time (event));
}


static int
delete_event (GtkWidget *widget, GdkEvent *event, gpointer data)
{
  gtk_main_quit ();
  return TRUE;
}


static void
button_clicked (GtkWidget *widget, gpointer data)
{
  if (data)
    {
      const char *s;

      /* Okay button or enter used in text field.  */

      if (pinentry->enhanced)
	printf ("Options: %s\nTimeout: %d\n\n",
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (insure))
		? "insure" : "",
		gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (time_out)));

      s = gtk_secure_entry_get_text (GTK_SECURE_ENTRY (entry));
      if (!s)
	s = "";
      passphrase_ok = 1;
      pinentry_setbufferlen (pinentry, strlen (s) + 1);
      if (pinentry->pin)
	strcpy (pinentry->pin, s);
    }
  gtk_main_quit ();
}


static void
enter_callback (GtkWidget *widget, GtkWidget *anentry)
{
  button_clicked (widget, "ok");
}


static void
confirm_button_clicked (GtkWidget *widget, gpointer data)
{
  if (data)
    /* Okay button.  */
    confirm_yes = 1;

  gtk_main_quit ();
}


static gchar *
pinentry_utf8_validate (gchar *text)
{
  gchar *result;

  if (!text)
    return NULL;

  if (g_utf8_validate (text, -1, NULL))
    return g_strdup (text);

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


static GtkWidget *
create_window (int confirm_mode)
{
  GtkWidget *w;
  GtkWidget *win, *box, *ebox;
  GtkWidget *wvbox, *chbox, *bbox;
  GtkAccelGroup *acc;
  GdkPixbuf *padlock_keyhole;
  gchar *msg;

  /* FIXME: check the grabbing code against the one we used with the
     old gpg-agent */
  win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
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
      g_signal_connect (G_OBJECT (win),
			pinentry->grab ? "map-event" : "focus-in-event",
			G_CALLBACK (grab_keyboard), NULL);
      g_signal_connect (G_OBJECT (win),
			pinentry->grab ? "unmap-event" : "focus-out-event",
			G_CALLBACK (ungrab_keyboard), NULL);
    }
  gtk_window_add_accel_group (GTK_WINDOW (win), acc);
  
  wvbox = gtk_vbox_new (FALSE, HIG_LARGE * 2);
  gtk_container_add (GTK_CONTAINER (win), wvbox);
  gtk_container_set_border_width (GTK_CONTAINER (wvbox), HIG_LARGE);

  chbox = gtk_hbox_new (FALSE, HIG_LARGE);
  gtk_box_pack_start (GTK_BOX (wvbox), chbox, FALSE, FALSE, 0);

  padlock_keyhole = gdk_pixbuf_new_from_xpm_data (padlock_keyhole_xpm);
  w = gtk_image_new_from_pixbuf (padlock_keyhole);
  gtk_misc_set_alignment (GTK_MISC (w), 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (chbox), w, FALSE, FALSE, 0);

  box = gtk_vbox_new (FALSE, HIG_SMALL);
  gtk_box_pack_start (GTK_BOX (chbox), box, TRUE, TRUE, 0);

  if (pinentry->description)
    {
      msg = pinentry_utf8_validate (pinentry->description);
      w = gtk_label_new (msg);
      g_free (msg);
      gtk_misc_set_alignment (GTK_MISC (w), 0.0, 0.5);
      gtk_label_set_line_wrap (GTK_LABEL (w), TRUE);
	gtk_box_pack_start (GTK_BOX (box), w, TRUE, FALSE, 0);
    }
  if (pinentry->error && !confirm_mode)
    {
      GdkColor color = { 0, 0xffff, 0, 0 };

      msg = pinentry_utf8_validate (pinentry->error);
      w = gtk_label_new (msg);
      g_free (msg);
      gtk_misc_set_alignment (GTK_MISC (w), 0.0, 0.5);
      gtk_label_set_line_wrap (GTK_LABEL (w), TRUE);
      gtk_box_pack_start (GTK_BOX (box), w, TRUE, FALSE, 0);
      gtk_widget_modify_fg (w, GTK_STATE_NORMAL, &color);
    }

  ebox = gtk_hbox_new (FALSE, HIG_SMALL);
  gtk_box_pack_start (GTK_BOX(box), ebox, FALSE, FALSE, 0);

  if (!confirm_mode)
    {
      if (pinentry->prompt)
	{
	  msg = pinentry_utf8_validate (pinentry->prompt);
	  w = gtk_label_new (msg);
	  g_free (msg);
	  gtk_box_pack_start (GTK_BOX (ebox), w, FALSE, FALSE, 0);
	}
      entry = gtk_secure_entry_new ();
      gtk_widget_set_size_request (entry, 200, -1);
      g_signal_connect (G_OBJECT (entry), "activate",
			G_CALLBACK (enter_callback), entry);
      gtk_box_pack_start (GTK_BOX (ebox), entry, TRUE, TRUE, 0);
      gtk_widget_grab_focus (entry);
      gtk_widget_show (entry);

      if (pinentry->enhanced)
	{
	  GtkWidget *sbox = gtk_hbox_new (FALSE, HIG_SMALL);
	  gtk_box_pack_start (GTK_BOX (box), sbox, FALSE, FALSE, 0);

	  w = gtk_label_new ("Forget secret after");
	  gtk_box_pack_start (GTK_BOX (sbox), w, FALSE, FALSE, 0);
	  gtk_widget_show (w);

	  time_out = gtk_spin_button_new
	    (GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, HUGE_VAL, 1, 60, 60)),
	     2, 0);
	  gtk_box_pack_start (GTK_BOX (sbox), time_out, FALSE, FALSE, 0);
	  gtk_widget_show (time_out);
	  
	  w = gtk_label_new ("seconds");
	  gtk_box_pack_start (GTK_BOX (sbox), w, FALSE, FALSE, 0);
	  gtk_widget_show (w);
	  gtk_widget_show (sbox);

	  insure = gtk_check_button_new_with_label ("ask before giving out "
						    "secret");
	  gtk_box_pack_start (GTK_BOX (box), insure, FALSE, FALSE, 0);
	  gtk_widget_show (insure);
	}
    }

  bbox = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_END);
  gtk_box_set_spacing (GTK_BOX (bbox), 6);
  gtk_box_pack_start (GTK_BOX (wvbox), bbox, TRUE, FALSE, 0);

  if (pinentry->cancel)
    {
      msg = pinentry_utf8_validate (pinentry->cancel);
      w = gtk_button_new_with_label (msg);
      g_free (msg);
    }
  else
    w = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_container_add (GTK_CONTAINER (bbox), w);
  g_signal_connect (G_OBJECT (w), "clicked",
		    G_CALLBACK (confirm_mode ? confirm_button_clicked
				: button_clicked), NULL);
  GTK_WIDGET_SET_FLAGS (w, GTK_CAN_DEFAULT);
  
  if (pinentry->ok)
    {
      msg = pinentry_utf8_validate (pinentry->ok);
      w = gtk_button_new_with_label (msg);
      g_free (msg);
    }
  else
    w = gtk_button_new_from_stock (GTK_STOCK_OK);
  gtk_container_add (GTK_CONTAINER(bbox), w);
  if (!confirm_mode)
    {
      g_signal_connect (G_OBJECT (w), "clicked",
			G_CALLBACK (button_clicked), "ok");
      GTK_WIDGET_SET_FLAGS (w, GTK_CAN_DEFAULT);
      gtk_widget_grab_default (w);
      g_signal_connect_object (G_OBJECT (entry), "focus_in_event",
				G_CALLBACK (gtk_widget_grab_default),
			       G_OBJECT (w), 0);
    }
  else
    {
      g_signal_connect (G_OBJECT (w), "clicked",
			G_CALLBACK(confirm_button_clicked), "ok");
      GTK_WIDGET_SET_FLAGS (w, GTK_CAN_DEFAULT);
    }

  gtk_window_set_position (GTK_WINDOW (win), GTK_WIN_POS_CENTER);
  
  gtk_widget_show_all(win);
  
  return win;
}


static int
gtk_cmd_handler (pinentry_t pe)
{
  GtkWidget *w;
  int want_pass = !!pe->pin;

  pinentry = pe;
  confirm_yes = 0;
  passphrase_ok = 0;
  w = create_window (want_pass ? 0 : 1);
  gtk_main ();
  gtk_widget_destroy (w);
  while (gtk_events_pending ())
    gtk_main_iteration ();

  pinentry = NULL;
  if (want_pass)
    {
      if (passphrase_ok && pe->pin)
	return strlen (pe->pin);
      else
	return -1;
    }
  else
    return confirm_yes;
}


pinentry_cmd_handler_t pinentry_cmd_handler = gtk_cmd_handler;


int
main (int argc, char *argv[])
{
  pinentry_init (PGMNAME);
    
#ifdef FALLBACK_CURSES
  if (pinentry_have_display (argc, argv))
    gtk_init (&argc, &argv);
  else
    pinentry_cmd_handler = curses_cmd_handler;
#else
  gtk_init (&argc, &argv);
#endif

  /* Consumes all arguments.  */
  if (pinentry_parse_opts (argc, argv))
    {
      printf(PGMNAME " " VERSION "\n");
      exit(EXIT_SUCCESS);
    }
  
  if (pinentry_loop ())
    return 1;
  
  return 0;
}
