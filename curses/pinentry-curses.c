/* pinentry-curses.c - A secure curses dialog for PIN entry.
   Copyright (C) 2002 g10 Code GmbH
   
   This file is part of PINENTRY.
   
   PINENTRY is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 
   PINENTRY is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
 
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA  */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <assert.h>
#include <curses.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include "pinentry.h"

#define STRING_OK "<OK>"
#define STRING_CANCEL "<Cancel>"

int init_screen;

typedef enum
  {
    DIALOG_POS_NONE,
    DIALOG_POS_PIN,
    DIALOG_POS_OK,
    DIALOG_POS_CANCEL
  }
dialog_pos_t;

struct dialog
{
  dialog_pos_t pos;
  int pin_y;
  int pin_x;
  /* Width of the PIN field.  */
  int pin_size;
  /* Cursor location in PIN field.  */
  int pin_loc;
  char *pin;
  int pin_max;
  /* Length of PIN.  */
  int pin_len;

  int ok_y;
  int ok_x;
  int cancel_y;
  int cancel_x;
};
typedef struct dialog *dialog_t;


static int
dialog_create (pinentry_t pinentry, dialog_t dialog)
{
  int size_y;
  int size_x;
  int y;
  int x;
  int ypos;
  int xpos;
  int description_x = 0;

  getmaxyx (stdscr, size_y, size_x);

  /* Check if all required lines fit on the screen.  */
  y = 1;		/* Top frame.  */
  if (pinentry->description)
    {
      char *p = pinentry->description;
      int desc_x = 0;

      while (*p)
	{
	  if (*(p++) == '\n')
	    {
	      if (desc_x > description_x)
		description_x = desc_x;
	      y++;
	      desc_x = 0;
	    }
	  else
	    desc_x++;
	}
      if (desc_x > description_x)
	description_x = desc_x;
      y += 2;		/* Description.  */
    }
      
  if (pinentry->pin)
    {
      if (pinentry->error)
	y += 2;		/* Error message.  */
      y += 2;		/* Pin entry field.  */
    }
  y += 2;		/* OK/Cancel and bottom frame.  */
  
  if (y > size_y)
    return 1;

  /* Check if all required columns fit on the screen.  */
  x = 0;
  if (pinentry->description)
    {
      int new_x = description_x;
      if (new_x > size_x - 4)
	new_x = size_x - 4;
      if (new_x > x)
	x = new_x;
    }
  if (pinentry->pin)
    {
#define MIN_PINENTRY_LENGTH 40
      int new_x;

      if (pinentry->error)
	{
	  new_x = strlen (pinentry->error);
	  if (new_x > size_x - 4)
	    new_x = size_x - 4;
	  if (new_x > x)
	    x = new_x;
	}

      new_x = MIN_PINENTRY_LENGTH;
      if (pinentry->prompt)
	new_x += strlen (pinentry->prompt) + 1;	/* One space after prompt.  */
      if (new_x > size_x - 4)
	new_x = size_x - 4;
      if (new_x > x)
	x = new_x;
    }
  /* We position the Buttons after the first and second third of the
     width.  Account for rounding when calculating the necessary*/
  if (x < 2 * (sizeof (STRING_OK) - 1))
    x = 2 * (sizeof (STRING_OK) - 1);
  if (x < 2 * (sizeof (STRING_CANCEL) - 1))
    x = 2 * (sizeof (STRING_CANCEL) - 1);

  /* Add the frame.  */
  x += 4;

  if (x > size_x)
    return 1;

  dialog->pos = DIALOG_POS_NONE;
  dialog->pin = pinentry->pin;
  dialog->pin_max = pinentry->pin_len;
  dialog->pin_loc = 0;
  dialog->pin_len = 0;
  ypos = (size_y - y) / 2;
  xpos = (size_x - x) / 2;
  move (ypos, xpos);
  addch (ACS_ULCORNER);
  hline (0, x - 2);
  move (ypos, xpos + x - 1);
  addch (ACS_URCORNER);
  move (ypos + 1, xpos + x - 1);
  vline (0, y - 2);
  move (ypos + y - 1, xpos);
  addch (ACS_LLCORNER);
  hline (0, x - 2);
  move (ypos + y - 1, xpos + x - 1);
  addch (ACS_LRCORNER);
  ypos++;
  if (pinentry->description)
    {
      char *p = pinentry->description;
      int i = 0;

      while (*p)
	{
	  move (ypos, xpos);
	  addch (ACS_VLINE);
	  addch (' ');
	  while (*p && *p != '\n')
	    if (i < x - 4)
	      {
		i++;
		addch (*(p++));
	      }
	  if (*p == '\n')
	    p++;
	  i = 0;
	  ypos++;
	}
      move (ypos, xpos);
      addch (ACS_VLINE);
      ypos++;
    }
  if (pinentry->pin)
    {
      int i;

      if (pinentry->error)
	{
	  char *p = pinentry->error;
	  i = strlen (pinentry->error);
	  move (ypos, xpos);
	  addch (ACS_VLINE);
	  addch (' ');
	  if (i > x - 4)
	    i = x - 4;
	  if (has_colors () && COLOR_PAIRS >= 1)
	    attron (COLOR_PAIR(1) | A_BOLD);
	  else
	    standout ();
	  while (i-- > 0)
	    addch (*(p++));
	  if (has_colors () && COLOR_PAIRS >= 1)
	    attroff (COLOR_PAIR(1) | A_BOLD);
	  else
	    standend ();
	  ypos++;
	  move (ypos, xpos);
	  addch (ACS_VLINE);
	  ypos++;
	}

      move (ypos, xpos);
      addch (ACS_VLINE);
      addch (' ');

      dialog->pin_y = ypos;
      dialog->pin_x = xpos + 2;
      dialog->pin_size = x - 4;
      if (pinentry->prompt)
	{
	  char *p = pinentry->prompt;
	  i = strlen (pinentry->prompt);
	  if (i > x - 4 - MIN_PINENTRY_LENGTH)
	    i = x - 4 - MIN_PINENTRY_LENGTH;
	  dialog->pin_x += i + 1;
	  dialog->pin_size -= i + 1;
	  while (i-- > 0)
	    addch (*(p++));
	  addch (' ');
	}
      for (i = 0; i < dialog->pin_size; i++)
	addch ('_');
      ypos++;
      move (ypos, xpos);
      addch (ACS_VLINE);
      ypos++;
    }
  move (ypos, xpos);
  addch (ACS_VLINE);
  dialog->ok_y = ypos;
  /* Calculating the left edge of the left button, rounding down.  */
  dialog->ok_x = xpos + 2 + ((x - 4) / 2 - (sizeof (STRING_OK) - 1)) / 2;
  move (dialog->ok_y, dialog->ok_x);
  addstr (STRING_OK);
  dialog->cancel_y = ypos;
  /* Calculating the left edge of the right button, rounding up.  */
  dialog->cancel_x = xpos + x - 2 - ((x - 4) / 2 + (sizeof (STRING_CANCEL) - 1)) / 2;
  move (dialog->cancel_y, dialog->cancel_x);
  addstr (STRING_CANCEL);
  return 0;
}


static void
set_cursor_state (int on)
{
  static int normal_state = -1;
  static int on_last;

  if (normal_state < 0 && !on)
    {
      normal_state = curs_set (0);
      on_last = on;
    }
  else if (on != on_last)
    {
      curs_set (on ? normal_state : 0);
      on_last = on;
    }
}

int
dialog_switch_pos (dialog_t diag, dialog_pos_t new_pos)
{
  if (new_pos != diag->pos)
    {
      switch (diag->pos)
	{
	case DIALOG_POS_OK:
	  move (diag->ok_y, diag->ok_x);
	  addstr (STRING_OK);
	  break;
	case DIALOG_POS_CANCEL:
	  move (diag->cancel_y, diag->cancel_x);
	  addstr (STRING_CANCEL);
	  break;
	default:
	  break;
	}
      diag->pos = new_pos;
      switch (diag->pos)
	{
	case DIALOG_POS_PIN:
	  move (diag->pin_y, diag->pin_x + diag->pin_loc);
	  set_cursor_state (1);
	  break;
	case DIALOG_POS_OK:
	  move (diag->ok_y, diag->ok_x);
	  standout ();
	  addstr (STRING_OK);
	  standend ();
	  set_cursor_state (0);
	  break;
	case DIALOG_POS_CANCEL:
	  move (diag->cancel_y, diag->cancel_x);
	  standout ();
	  addstr (STRING_CANCEL);
	  standend ();
	  set_cursor_state (0);
	  break;
	case DIALOG_POS_NONE:
	  set_cursor_state (0);
	  break;
	}
      refresh ();
    }
}

/* XXX Assume that field width is at least > 5.  */
static void
dialog_input (dialog_t diag, int chr)
{
  int old_loc = diag->pin_loc;
  assert (diag->pin);
  assert (diag->pos == DIALOG_POS_PIN);

  switch (chr)
    {
    case KEY_BACKSPACE:
      if (diag->pin_len > 0)
	{
	  diag->pin_len--;
	  diag->pin_loc--;
	  if (diag->pin_loc == 0 && diag->pin_len > 0)
	    {
	      diag->pin_loc = diag->pin_size - 5;
	      if (diag->pin_loc > diag->pin_len)
		diag->pin_loc = diag->pin_len;
	    }
	}
      break;

    default:
      if (chr > 0 && chr < 256 && diag->pin_len < diag->pin_max)
	{
	  diag->pin[diag->pin_len] = (char) chr;
	  diag->pin_len++;
	  diag->pin_loc++;
	  if (diag->pin_loc == diag->pin_size && diag->pin_len < diag->pin_max)
	    {
	      diag->pin_loc = 5;
	      if (diag->pin_loc < diag->pin_size - (diag->pin_max + 1 - diag->pin_len))
		diag->pin_loc = diag->pin_size - (diag->pin_max + 1 - diag->pin_len);
	    }
	}
      break;
    }

  if (old_loc < diag->pin_loc)
    {
      move (diag->pin_y, diag->pin_x + old_loc);
      while (old_loc++ < diag->pin_loc)
	addch ('*');
    }
  else if (old_loc > diag->pin_loc)
    {
      move (diag->pin_y, diag->pin_x + diag->pin_loc);
      while (old_loc-- > diag->pin_loc)
	addch ('_');
    }
  move (diag->pin_y, diag->pin_x + diag->pin_loc);
}

static int
dialog_run (pinentry_t pinentry, const char *tty_name, const char *tty_type)
{
  struct dialog diag;
  FILE *ttyfi = 0;
  FILE *ttyfo = 0;
  SCREEN *screen = 0;
  int done = 0;

  /* Open the desired terminal if necessary.  */
  if (tty_name)
    {
      ttyfi = fopen (tty_name, "r");
      if (ttyfi < 0)
	return -1;
      ttyfo = fopen (tty_name, "w");
      if (ttyfo < 0)
	{
	  int err = errno;
	  fclose (ttyfi);
	  errno = err;
	  return -1;
	}
      screen = newterm (tty_type, ttyfo, ttyfi);
      set_term (screen);
    }
  else
    {
      if (!init_screen)
	{
	  init_screen = 1;
	  initscr ();
	}
      else
	clear ();
    }
  
  keypad (stdscr, TRUE); /* Enable keyboard mapping.  */
  nonl ();		/* Tell curses not to do NL->CR/NL on output.  */
  cbreak ();		/* Take input chars one at a time, no wait for \n.  */
  noecho ();		/* Don't echo input - in color.  */
  refresh ();

  if (has_colors ())
    {
      start_color ();

      if (COLOR_PAIRS >= 1)
	init_pair (1, COLOR_RED, COLOR_BLACK);
    }

  /* XXX */
  if (dialog_create (pinentry, &diag))
    return -2;
  dialog_switch_pos (&diag, diag.pin ? DIALOG_POS_PIN : DIALOG_POS_OK);

  do
    {
      int c;

      c = getch ();     /* Refresh, accept single keystroke of input.  */

      switch (c)
	{
	case KEY_LEFT:
	case KEY_UP:
	  switch (diag.pos)
	    {
	    case DIALOG_POS_OK:
	      if (diag.pin)
		dialog_switch_pos (&diag, DIALOG_POS_PIN);
	      break;
	    case DIALOG_POS_CANCEL:
	      dialog_switch_pos (&diag, DIALOG_POS_OK);
	      break;
	    default:
	      break;
	    }
	  break;

	case KEY_RIGHT:
	case KEY_DOWN:
	  switch (diag.pos)
	    {
	    case DIALOG_POS_PIN:
	      dialog_switch_pos (&diag, DIALOG_POS_OK);
	      break;
	    case DIALOG_POS_OK:
	      dialog_switch_pos (&diag, DIALOG_POS_CANCEL);
	      break;
	    default:
	      break;
	    }
	  break;

	case '\t':
	  switch (diag.pos)
	    {
	    case DIALOG_POS_PIN:
	      dialog_switch_pos (&diag, DIALOG_POS_OK);
	      break;
	    case DIALOG_POS_OK:
	      dialog_switch_pos (&diag, DIALOG_POS_CANCEL);
	      break;
	    case DIALOG_POS_CANCEL:
	      if (diag.pin)
		dialog_switch_pos (&diag, DIALOG_POS_PIN);
	      else
		dialog_switch_pos (&diag, DIALOG_POS_OK);
	      break;
	    default:
	      break;
	    }
	  break;
  
	case '\e':
	  done = -2;
	  break;

	case '\r':
	  switch (diag.pos)
	    {
	    case DIALOG_POS_PIN:
	    case DIALOG_POS_OK:
	      done = 1;
	      break;
	    case DIALOG_POS_CANCEL:
	      done = -2;
	      break;
	    }
	  break;

	default:
	  if (diag.pos == DIALOG_POS_PIN)
	    dialog_input (&diag, c);
	}
    }
  while (!done);

  set_cursor_state (1);
  endwin ();
  if (screen)
    delscreen (screen);

  if (ttyfi)
    fclose (ttyfi);
  if (ttyfo)
    fclose (ttyfo);
  return diag.pin ? (done < 0 ? -1 : diag.pin_len) : (done < 0 ? 0 : 1);
}

static int
dialog_cmd_handler (pinentry_t pinentry)
{
  return dialog_run (pinentry, pinentry->ttyname, pinentry->ttytype);
}

pinentry_cmd_handler_t pinentry_cmd_handler = dialog_cmd_handler;


int 
main (int argc, char *argv[])
{
  pinentry_init ();

  /* Consumes all arguments.  */
  if (pinentry_parse_opts (argc, argv))
    {
      printf ("pinentry-curses " VERSION "\n");
      exit (EXIT_SUCCESS);
    }

  if (pinentry_loop ())
    return 1;

  return 0;
}
