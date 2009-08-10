/* gpinentry.c 
 * Copyright (C) 2001, 2002 g10 Code GmbH
 * Copyright (C) 1999 Robert Bihlmeyer <robbe@orcus.priv.at>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/X.h>


#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else 
#include "getopt.h"
#endif /* HAVE_GETOPT_H */

#include "gtksecentry.h"
#include "pinentry.h"
#include "memory.h"


#ifdef FALLBACK_CURSES
#include "pinentry-curses.h"
#endif

#define PGMNAME "pinentry-gtk"

static pinentry_t pinentry;
static int passphrase_ok = 0;
static int confirm_yes = 0;

static GtkWidget *entry, *insure, *time_out;

#if 0 /* not used */
/* ok - Return to the command handler routine.  */
static void 
ok (GtkWidget *w, gpointer data)
{
  gtk_main_quit ();
}
#endif /* not used */

/* unselect - work around a bug in GTK+ that permits word-selection to
   work on the invisible passphrase */
static void 
unselect(GtkWidget *w, GdkEventButton *e)
{
  static gint lastpos;

  if (e->button == 1) {
    if (e->type == GDK_BUTTON_PRESS) {
      lastpos = gtk_editable_get_position(GTK_EDITABLE(entry));
    } else if (e->type == GDK_2BUTTON_PRESS || e->type == GDK_3BUTTON_PRESS) {
      gtk_secure_entry_set_position(GTK_SECURE_ENTRY(w), lastpos);
      gtk_secure_entry_select_region(GTK_SECURE_ENTRY(w), 0, 0);
    }
  }
}

/* constrain_size - constrain size of the window the window should not
   shrink beyond the requisition, and should not grow vertically */
static void 
constrain_size(GtkWidget *win, GtkRequisition *req, gpointer data)
{
  static gint width, height;
  GdkGeometry geo;

  if (req->width == width && req->height == height)
    return; 
  width = req->width;
  height = req->height;
  geo.min_width = width;
  geo.max_width = 10000;	/* this limit is arbitrary,
				   but INT_MAX breaks other things */
  geo.min_height = geo.max_height = height;
  gtk_window_set_geometry_hints(GTK_WINDOW(win), NULL, &geo,
  				GDK_HINT_MIN_SIZE|GDK_HINT_MAX_SIZE);
}

/* grab_keyboard - grab the keyboard for maximum security */
static void 
grab_keyboard(GtkWidget *win, GdkEvent *event, gpointer data)
{
  if (!pinentry->grab)
    return;
  if (gdk_keyboard_grab(win->window, FALSE, gdk_event_get_time(event))) 
    {
      g_error("could not grab keyboard");
    }
}

/* ungrab_keyboard - remove grab */
static void 
ungrab_keyboard(GtkWidget *win, GdkEvent *event, gpointer data)
{
  gdk_keyboard_ungrab(gdk_event_get_time(event));
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
    { /* Okay button hit or Enter used in the text field. */
      const char *s;
      char *s_utf8;
      char *s_buffer;

#ifdef ENABLE_ENHANCED
      if (pinentry->enhanced)
        {
          printf("Options: %s\nTimeout: %d\n\n",
                 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(insure)) ?
                 "insure"
                 : "",
                 gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(time_out)));
        }
#endif

      pinentry->locale_err = 1;
      s = gtk_secure_entry_get_text (GTK_SECURE_ENTRY(entry));
      if (!s)
        s = "";
      s_buffer = secmem_malloc (strlen (s) + 1);
      if (s_buffer)
        {
          strcpy (s_buffer, s);
          s_utf8 = pinentry_local_to_utf8 (pinentry->lc_ctype, s_buffer, 1);
          secmem_free (s_buffer);
          if (s_utf8)
            {
              passphrase_ok = 1;
              pinentry_setbufferlen (pinentry, strlen (s_utf8) + 1);
              if (pinentry->pin)
                strcpy (pinentry->pin, s_utf8);
              secmem_free (s_utf8);
              pinentry->locale_err = 0;
            }
        }
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
    { /* okay button */
      confirm_yes = 1;
    }
  gtk_main_quit ();
}


static GtkWidget *
create_utf8_label (char *text)
{
  GtkWidget *w;
  char *buf;

  buf = pinentry_utf8_to_local (pinentry->lc_ctype, text);
  if (buf)
    {
      w = gtk_label_new (buf);
      free (buf);
    }
  else /* no core - simply don't convert */
    w = gtk_label_new (text);
  return w;
}


static GtkWidget *
create_window (int confirm_mode)
{
  GtkWidget *w;
  GtkWidget *win, *box, *ebox;
  GtkWidget *sbox, *bbox;
  GtkAccelGroup *acc;

  /* fixme: check the grabbing code against the one we used with the
     old gpg-agent */
  win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  acc = gtk_accel_group_new ();

  gtk_signal_connect (GTK_OBJECT (win), "delete_event",
                      GTK_SIGNAL_FUNC (delete_event), NULL);

#if 0
  gtk_signal_connect (GTK_OBJECT(win), "destroy", gtk_main_quit, NULL);
#endif
  gtk_signal_connect (GTK_OBJECT(win), "size-request", constrain_size, NULL);
  if (!confirm_mode)
    {
      gtk_signal_connect (GTK_OBJECT(win),
                          pinentry->grab ? "map-event" : "focus-in-event",
                          grab_keyboard, NULL);
      gtk_signal_connect (GTK_OBJECT(win),
                          pinentry->grab ? "unmap-event" : "focus-out-event",
                          ungrab_keyboard, NULL);
    }
  gtk_accel_group_attach(acc, GTK_OBJECT(win));

  box = gtk_vbox_new (FALSE, 5);
  gtk_container_add (GTK_CONTAINER(win), box);
  gtk_container_set_border_width (GTK_CONTAINER (box), 5);

  if (pinentry->description)
    {
      w = create_utf8_label (pinentry->description);
      gtk_box_pack_start (GTK_BOX(box), w, TRUE, FALSE, 0);
    }
  if (pinentry->error && !confirm_mode)
    {
      GtkStyle *style;
      GdkColormap *cmap;
      GdkColor color[1] = {{0, 0xffff, 0, 0}};
      gboolean success[1];

      w = create_utf8_label (pinentry->error);
      gtk_box_pack_start (GTK_BOX(box), w, TRUE, FALSE, 5);

      /* fixme: Do we need to release something, or is there a more
         easy way to set a text color? */
      gtk_widget_realize (win);
      cmap = gdk_window_get_colormap (win->window);
      assert (cmap);
      gdk_colormap_alloc_colors (cmap, color, 1, FALSE, TRUE, success);
      if (success[0])
        {
          gtk_widget_ensure_style(w);
          style = gtk_style_copy(gtk_widget_get_style(w));
          style->fg[GTK_STATE_NORMAL] = color[0];
          gtk_widget_set_style(w, style);
        } 
    }

  ebox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX(box), ebox, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (ebox), 5);


  if (!confirm_mode)
    {
      if (pinentry->prompt)
        {
          w = create_utf8_label (pinentry->prompt);
          gtk_box_pack_start (GTK_BOX(ebox), w, FALSE, FALSE, 0);
        }
      entry = gtk_secure_entry_new ();
      gtk_signal_connect (GTK_OBJECT (entry), "activate",
                          GTK_SIGNAL_FUNC (enter_callback), entry);
      gtk_box_pack_start (GTK_BOX(ebox), entry, TRUE, TRUE, 0);
      gtk_secure_entry_set_visibility (GTK_SECURE_ENTRY(entry), FALSE);
      gtk_signal_connect_after (GTK_OBJECT(entry), "button_press_event",
                                unselect, NULL);
      gtk_widget_grab_focus (entry);
      gtk_widget_show (entry);

#ifdef ENABLE_ENHANCED
      if (pinentry->enhanced)
        {
          sbox = gtk_hbox_new(FALSE, 5);
          gtk_box_pack_start(GTK_BOX(box), sbox, FALSE, FALSE, 0);
          
          w = gtk_label_new ("Forget secret after");
          gtk_box_pack_start(GTK_BOX(sbox), w, FALSE, FALSE, 0);
          gtk_widget_show(w);
          
          time_out = gtk_spin_button_new
            (GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, HUGE_VAL,
                                               1, 60, 60)),2,0);
          gtk_box_pack_start (GTK_BOX(sbox), time_out, FALSE, FALSE, 0);
          gtk_widget_show (time_out);
	
          w = gtk_label_new ("seconds");
          gtk_box_pack_start (GTK_BOX(sbox), w, FALSE, FALSE, 0); 
          gtk_widget_show (w);
          gtk_widget_show (sbox);
          
          insure = gtk_check_button_new_with_label
            ("ask before giving out secret");
          gtk_box_pack_start (GTK_BOX(box), insure, FALSE, FALSE, 0);
          gtk_widget_show (insure);
        }
#endif
    }


  bbox = gtk_hbutton_box_new();
  gtk_box_pack_start (GTK_BOX(box), bbox, TRUE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (bbox), 5);
  
  w = gtk_button_new_with_label (pinentry->ok ? pinentry->ok : "OK");
  gtk_container_add (GTK_CONTAINER(bbox), w);
  if (!confirm_mode)
    {
      gtk_signal_connect (GTK_OBJECT(w), "clicked", button_clicked, "ok");
      GTK_WIDGET_SET_FLAGS (w, GTK_CAN_DEFAULT);
      gtk_widget_grab_default (w);
      gtk_signal_connect_object (GTK_OBJECT (entry), "focus_in_event",
                                 GTK_SIGNAL_FUNC (gtk_widget_grab_default),
                                 GTK_OBJECT (w));
    }
  else
    {
      gtk_signal_connect (GTK_OBJECT(w), "clicked",
                          confirm_button_clicked, "ok");
      GTK_WIDGET_SET_FLAGS (w, GTK_CAN_DEFAULT);
    }

  gtk_widget_show (w);
  
  if (!pinentry->one_button)
    {
      w = gtk_button_new_with_label (pinentry->cancel 
                                     ? pinentry->cancel : "Cancel");
      gtk_container_add (GTK_CONTAINER(bbox), w);
      gtk_accel_group_add (acc, GDK_Escape, 0, 0, GTK_OBJECT(w), "clicked");
      gtk_signal_connect (GTK_OBJECT(w), "clicked", 
                          confirm_mode? confirm_button_clicked: button_clicked,
                          NULL);
      GTK_WIDGET_SET_FLAGS (w, GTK_CAN_DEFAULT);
    }

  gtk_window_set_position (GTK_WINDOW (win), GTK_WIN_POS_CENTER);

  gtk_widget_show_all (win);

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
  gtk_main();
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
      printf ("pinentry-gtk (pinentry) " VERSION "\n");
      exit (EXIT_SUCCESS);
    }

  if (pinentry_loop ())
    return 1;

  return 0;
}

