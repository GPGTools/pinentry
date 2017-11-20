/* pinentry-curses.c - A secure curses dialog for PIN entry, library version
 * Copyright (C) 2002, 2015 g10 Code GmbH
 *
 * This file is part of PINENTRY.
 *
 * PINENTRY is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * PINENTRY is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <assert.h>
#include <curses.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <locale.h>
#include <iconv.h>
#include <langinfo.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UTIME_H
#include <utime.h>
#endif /*HAVE_UTIME_H*/

#include <memory.h>

#ifdef HAVE_WCHAR_H
#include <wchar.h>
#endif /*HAVE_WCHAR_H*/

#include <assuan.h>

#include "pinentry.h"

#if GPG_ERROR_VERSION_NUMBER < 0x011900 /* 1.25 */
# define GPG_ERR_WINDOW_TOO_SMALL 301
# define GPG_ERR_MISSING_ENVVAR   303
#endif


/* FIXME: We should allow configuration of these button labels and in
   any case use the default_ok, default_cancel values if available.
   However, I have no clue about curses and localization.  */
#define STRING_OK "<OK>"
#define STRING_NOTOK "<No>"
#define STRING_CANCEL "<Cancel>"

#define USE_COLORS		(has_colors () && COLOR_PAIRS >= 2)
static short pinentry_color[] = { -1, -1, COLOR_BLACK, COLOR_RED,
				  COLOR_GREEN, COLOR_YELLOW, COLOR_BLUE,
				  COLOR_MAGENTA, COLOR_CYAN, COLOR_WHITE };
static int init_screen;
#ifndef HAVE_DOSISH_SYSTEM
static int timed_out;
#endif

typedef enum
  {
    DIALOG_POS_NONE,
    DIALOG_POS_PIN,
    DIALOG_POS_OK,
    DIALOG_POS_NOTOK,
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
  int pin_max;
  /* Length of PIN.  */
  int pin_len;

  int ok_y;
  int ok_x;
  char *ok;
  int cancel_y;
  int cancel_x;
  char *cancel;
  int notok_y;
  int notok_x;
  char *notok;

  pinentry_t pinentry;
};
typedef struct dialog *dialog_t;


#ifdef HAVE_NCURSESW
typedef wchar_t CH;
#define STRLEN(x) wcslen (x)
#define ADDCH(x) addnwstr (&x, 1);
#define CHWIDTH(x) wcwidth (x)
#define NULLCH L'\0'
#define NLCH L'\n'
#define SPCH L' '
#else
typedef char CH;
#define STRLEN(x) strlen (x)
#define ADDCH(x) addch ((unsigned char) x)
#define CHWIDTH(x) 1
#define NULLCH '\0'
#define NLCH '\n'
#define SPCH ' '
#endif

/* Return the next line up to MAXLEN columns wide in START and LEN.
   The first invocation should have 0 as *LEN.  If the line ends with
   a \n, it is a normal line that will be continued.  If it is a '\0'
   the end of the text is reached after this line.  In all other cases
   there is a forced line break.  A full line is returned and will be
   continued in the next line.  */
static void
collect_line (int maxwidth, CH **start_p, int *len_p)
{
  int last_space = 0;
  int len = *len_p;
  int width = 0;
  CH *end;

  /* Skip to next line.  */
  *start_p += len;
  /* Skip leading space.  */
  while (**start_p == SPCH)
    (*start_p)++;

  end = *start_p;
  len = 0;

  while (width < maxwidth - 1 && *end != NULLCH && *end != NLCH)
    {
      len++;
      end++;
      if (*end == SPCH)
	last_space = len;
      width += CHWIDTH (*end);
    }

  if (*end != NULLCH && *end != NLCH && last_space != 0)
    {
      /* We reached the end of the available space, but still have
	 characters to go in this line.  We can break the line into
	 two parts at a space.  */
      len = last_space;
      (*start_p)[len] = NLCH;
    }
  *len_p = len + 1;
}

#ifdef HAVE_NCURSESW
static CH *
utf8_to_local (char *lc_ctype, char *string)
{
  mbstate_t ps;
  size_t len;
  char *local;
  const char *p;
  wchar_t *wcs = NULL;
  char *old_ctype = NULL;

  local = pinentry_utf8_to_local (lc_ctype, string);
  if (!local)
    return NULL;

  old_ctype = strdup (setlocale (LC_CTYPE, NULL));
  setlocale (LC_CTYPE, lc_ctype? lc_ctype : "");

  p = local;
  memset (&ps, 0, sizeof(mbstate_t));
  len = mbsrtowcs (NULL, &p, strlen (string), &ps);
  if (len == (size_t)-1)
    {
      free (local);
      goto leave;
    }
  wcs = calloc (len + 1, sizeof(wchar_t));
  if (!wcs)
    {
      free (local);
      goto leave;
    }

  p = local;
  memset (&ps, 0, sizeof(mbstate_t));
  mbsrtowcs (wcs, &p, len, &ps);

  free (local);

 leave:
  if (old_ctype)
    {
      setlocale (LC_CTYPE, old_ctype);
      free (old_ctype);
    }

  return wcs;
}
#else
static CH *
utf8_to_local (const char *lc_ctype, const char *string)
{
  return pinentry_utf8_to_local (lc_ctype, string);
}
#endif

static int
dialog_create (pinentry_t pinentry, dialog_t dialog)
{
  int err = 0;
  int size_y;
  int size_x;
  int y;
  int x;
  int ypos;
  int xpos;
  int description_x = 0;
  int error_x = 0;
  CH *description = NULL;
  CH *error = NULL;
  CH *prompt = NULL;

  dialog->pinentry = pinentry;

#define COPY_OUT(what)							\
  do									\
    if (pinentry->what)							\
      {									\
        what = utf8_to_local (pinentry->lc_ctype, pinentry->what);	\
        if (!what)							\
	  {								\
	    err = 1;							\
            pinentry->specific_err = gpg_error (GPG_ERR_LOCALE_PROBLEM); \
            pinentry->specific_err_loc = "dialog_create_copy";          \
	    goto out;							\
	  }								\
      }									\
  while (0)

  COPY_OUT (description);
  COPY_OUT (error);
  COPY_OUT (prompt);

  /* There is no pinentry->default_notok.  Map it to
     pinentry->notok.  */
#define default_notok notok
#define MAKE_BUTTON(which,default)					\
  do									\
    {									\
      char *new = NULL;							\
      if (pinentry->default_##which || pinentry->which)			\
        {								\
	  int len;							\
	  char *msg;							\
	  int i, j;							\
									\
	  msg = pinentry->which;					\
	  if (! msg)							\
	    msg = pinentry->default_##which;				\
          len = strlen (msg);						\
									\
          new = malloc (len + 3);				       	\
	  if (!new)							\
	    {								\
	      err = 1;							\
              pinentry->specific_err = gpg_error_from_syserror ();	\
              pinentry->specific_err_loc = "dialog_create_mk_button";   \
	      goto out;							\
	    }								\
									\
	  new[0] = '<'; 						\
	  for (i = 0, j = 1; i < len; i ++, j ++)			\
	    {								\
	      if (msg[i] == '_')					\
		{							\
		  i ++;							\
		  if (msg[i] == 0)					\
		    /* _ at end of string.  */				\
		    break;						\
		}							\
	      new[j] = msg[i];						\
	    }								\
									\
	  new[j] = '>';							\
	  new[j + 1] = 0;						\
        }								\
      dialog->which = pinentry_utf8_to_local (pinentry->lc_ctype,	\
					      new ? new : default);	\
      if (!dialog->which)						\
        {								\
	  err = 1;							\
          pinentry->specific_err = gpg_error (GPG_ERR_LOCALE_PROBLEM);	\
          pinentry->specific_err_loc = "dialog_create_utf8conv";        \
	  goto out;							\
	}								\
    }									\
  while (0)

  MAKE_BUTTON (ok, STRING_OK);
  if (!pinentry->one_button)
    MAKE_BUTTON (cancel, STRING_CANCEL);
  else
    dialog->cancel = NULL;
  if (!pinentry->one_button && pinentry->notok)
    MAKE_BUTTON (notok, STRING_NOTOK);
  else
    dialog->notok = NULL;

  getmaxyx (stdscr, size_y, size_x);

  /* Check if all required lines fit on the screen.  */
  y = 1;		/* Top frame.  */
  if (description)
    {
      CH *start = description;
      int len = 0;

      do
	{
	  collect_line (size_x - 4, &start, &len);
	  if (len > description_x)
	    description_x = len;
	  y++;
	}
      while (start[len - 1]);
      y++;
    }

  if (pinentry->pin)
    {
      if (error)
	{
	  CH *p = error;
	  int err_x = 0;

	  while (*p)
	    {
	      if (*(p++) == '\n')
		{
		  if (err_x > error_x)
		    error_x = err_x;
		  y++;
		  err_x = 0;
		}
	      else
		err_x++;
	    }
	  if (err_x > error_x)
	    error_x = err_x;
	  y += 2;	/* Error message.  */
	}
      y += 2;		/* Pin entry field.  */
    }
  y += 2;		/* OK/Cancel and bottom frame.  */

  if (y > size_y)
    {
      err = 1;
      pinentry->specific_err = gpg_error (size_y < 0? GPG_ERR_MISSING_ENVVAR
                                          /* */     : GPG_ERR_WINDOW_TOO_SMALL);
      pinentry->specific_err_loc = "dialog_create";
      goto out;
    }

  /* Check if all required columns fit on the screen.  */
  x = 0;
  if (description)
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

      if (error)
	{
	  new_x = error_x;
	  if (new_x > size_x - 4)
	    new_x = size_x - 4;
	  if (new_x > x)
	    x = new_x;
	}

      new_x = MIN_PINENTRY_LENGTH;
      if (prompt)
	{
	  new_x += STRLEN (prompt) + 1;	/* One space after prompt.  */
	}
      if (new_x > size_x - 4)
	new_x = size_x - 4;
      if (new_x > x)
	x = new_x;
    }
  /* We position the buttons after the first, second and third fourth
     of the width.  Account for rounding.  */
  if (x < 3 * strlen (dialog->ok))
    x = 3 * strlen (dialog->ok);
  if (dialog->cancel)
    if (x < 3 * strlen (dialog->cancel))
      x = 3 * strlen (dialog->cancel);
  if (dialog->notok)
    if (x < 3 * strlen (dialog->notok))
      x = 3 * strlen (dialog->notok);

  /* Add the frame.  */
  x += 4;

  if (x > size_x)
    {
      err = 1;
      pinentry->specific_err = gpg_error (size_x < 0? GPG_ERR_MISSING_ENVVAR
                                          /* */     : GPG_ERR_WINDOW_TOO_SMALL);
      pinentry->specific_err_loc = "dialog_create";
      goto out;
    }

  dialog->pos = DIALOG_POS_NONE;
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
  if (description)
    {
      CH *start = description;
      int len = 0;

      do
	{
	  int i;

	  move (ypos, xpos);
	  addch (ACS_VLINE);
	  addch (' ');
	  collect_line (size_x - 4, &start, &len);
	  for (i = 0; i < len - 1; i++)
	    {
	      ADDCH (start[i]);
	    }
	  if (start[len - 1] != NULLCH && start[len - 1] != NLCH)
	    ADDCH (start[len - 1]);
	  ypos++;
	}
      while (start[len - 1]);
      move (ypos, xpos);
      addch (ACS_VLINE);
      ypos++;
    }
  if (pinentry->pin)
    {
      int i;

      if (error)
	{
	  CH *p = error;
	  i = 0;

	  while (*p)
	    {
	      move (ypos, xpos);
	      addch (ACS_VLINE);
	      addch (' ');
	      if (USE_COLORS && pinentry->color_so != PINENTRY_COLOR_NONE)
		{
		  attroff (COLOR_PAIR (1) | (pinentry->color_fg_bright ? A_BOLD : 0));
		  attron (COLOR_PAIR (2) | (pinentry->color_so_bright ? A_BOLD : 0));
		}
	      else
		standout ();
	      for (;*p && *p != NLCH; p++)
		if (i < x - 4)
		  {
		    i++;
		    ADDCH (*p);
		  }
	      if (USE_COLORS && pinentry->color_so != PINENTRY_COLOR_NONE)
		{
		  attroff (COLOR_PAIR (2) | (pinentry->color_so_bright ? A_BOLD : 0));
		  attron (COLOR_PAIR (1) | (pinentry->color_fg_bright ? A_BOLD : 0));
		}
	      else
		standend ();
	      if (*p == '\n')
		p++;
	      i = 0;
	      ypos++;
	    }
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
      if (prompt)
	{
	  CH *p = prompt;
	  i = STRLEN (prompt);
	  if (i > x - 4 - MIN_PINENTRY_LENGTH)
	    i = x - 4 - MIN_PINENTRY_LENGTH;
	  dialog->pin_x += i + 1;
	  dialog->pin_size -= i + 1;
	  while (i-- > 0)
	    {
	      ADDCH (*(p++));
	    }
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

  if (dialog->cancel || dialog->notok)
    {
      dialog->ok_y = ypos;
      /* Calculating the left edge of the left button, rounding down.  */
      dialog->ok_x = xpos + 2 + ((x - 4) / 3 - strlen (dialog->ok)) / 2;
      move (dialog->ok_y, dialog->ok_x);
      addstr (dialog->ok);

      if (! pinentry->pin && dialog->notok)
	{
	  dialog->notok_y = ypos;
	  /* Calculating the left edge of the middle button, rounding up.  */
	  dialog->notok_x = xpos + x / 2 - strlen (dialog->notok) / 2;
	  move (dialog->notok_y, dialog->notok_x);
	  addstr (dialog->notok);
	}
      if (dialog->cancel)
	{
	  dialog->cancel_y = ypos;
	  /* Calculating the left edge of the right button, rounding up.  */
	  dialog->cancel_x = xpos + x - 2 - ((x - 4) / 3 + strlen (dialog->cancel)) / 2;
	  move (dialog->cancel_y, dialog->cancel_x);
	  addstr (dialog->cancel);
	}
    }
  else
    {
      dialog->ok_y = ypos;
      /* Calculating the left edge of the OK button, rounding down.  */
      dialog->ok_x = xpos + x / 2 - strlen (dialog->ok) / 2;
      move (dialog->ok_y, dialog->ok_x);
      addstr (dialog->ok);
    }

 out:
  if (description)
    free (description);
  if (error)
    free (error);
  if (prompt)
    free (prompt);
  return err;
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

static int
dialog_switch_pos (dialog_t diag, dialog_pos_t new_pos)
{
  if (new_pos != diag->pos)
    {
      switch (diag->pos)
	{
	case DIALOG_POS_OK:
	  move (diag->ok_y, diag->ok_x);
	  addstr (diag->ok);
	  break;
	case DIALOG_POS_NOTOK:
          if (diag->notok)
            {
              move (diag->notok_y, diag->notok_x);
              addstr (diag->notok);
            }
	  break;
	case DIALOG_POS_CANCEL:
          if (diag->cancel)
            {
              move (diag->cancel_y, diag->cancel_x);
              addstr (diag->cancel);
            }
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
	  set_cursor_state (0);
	  move (diag->ok_y, diag->ok_x);
	  standout ();
	  addstr (diag->ok);
	  standend ();
	  move (diag->ok_y, diag->ok_x);
	  break;
	case DIALOG_POS_NOTOK:
          if (diag->notok)
            {
              set_cursor_state (0);
              move (diag->notok_y, diag->notok_x);
              standout ();
              addstr (diag->notok);
              standend ();
              move (diag->notok_y, diag->notok_x);
            }
	  break;
	case DIALOG_POS_CANCEL:
          if (diag->cancel)
            {
              set_cursor_state (0);
              move (diag->cancel_y, diag->cancel_x);
              standout ();
              addstr (diag->cancel);
              standend ();
              move (diag->cancel_y, diag->cancel_x);
            }
	  break;
	case DIALOG_POS_NONE:
	  set_cursor_state (0);
	  break;
	}
      refresh ();
    }
  return 0;
}

/* XXX Assume that field width is at least > 5.  */
static void
dialog_input (dialog_t diag, int alt, int chr)
{
  int old_loc = diag->pin_loc;
  assert (diag->pinentry->pin);
  assert (diag->pos == DIALOG_POS_PIN);

  if (alt && chr == KEY_BACKSPACE)
    /* Remap alt-backspace to control-W.  */
    chr = 'w' - 'a' + 1;

  switch (chr)
    {
    case KEY_BACKSPACE:
      /* control-h.  */
    case 'h' - 'a' + 1:
      /* ASCII DEL.  What Mac OS X apparently emits when the "delete"
	 (backspace) key is pressed.  */
    case 127:
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

    case 'l' - 'a' + 1: /* control-l */
      /* Refresh the screen.  */
      endwin ();
      refresh ();
      break;

    case 'u' - 'a' + 1: /* control-u */
      /* Erase the whole line.  */
      if (diag->pin_len > 0)
	{
	  diag->pin_len = 0;
	  diag->pin_loc = 0;
	}
      break;

    case 'w' - 'a' + 1: /* control-w.  */
      while (diag->pin_len > 0
	     && diag->pinentry->pin[diag->pin_len - 1] == ' ')
	{
	  diag->pin_len --;
	  diag->pin_loc --;
	  if (diag->pin_loc < 0)
	    {
	      diag->pin_loc += diag->pin_size;
	      if (diag->pin_loc > diag->pin_len)
		diag->pin_loc = diag->pin_len;
	    }
	}
      while (diag->pin_len > 0
	     && diag->pinentry->pin[diag->pin_len - 1] != ' ')
	{
	  diag->pin_len --;
	  diag->pin_loc --;
	  if (diag->pin_loc < 0)
	    {
	      diag->pin_loc += diag->pin_size;
	      if (diag->pin_loc > diag->pin_len)
		diag->pin_loc = diag->pin_len;
	    }
	}

      break;

    default:
      if (chr > 0 && chr < 256 && diag->pin_len < diag->pin_max)
	{
	  /* Make sure there is enough room for this character and a
	     following NUL byte.  */
	  if (! pinentry_setbufferlen (diag->pinentry, diag->pin_len + 2))
	    {
	      /* Bail.  Here we use a simple approach.  It would be
                 better to have a pinentry_bug function.  */
              assert (!"setbufferlen failed");
              abort ();
	    }

	  diag->pinentry->pin[diag->pin_len] = (char) chr;
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
  int confirm_mode = !pinentry->pin;
  struct dialog diag;
  FILE *ttyfi = NULL;
  FILE *ttyfo = NULL;
  SCREEN *screen = 0;
  int done = 0;
  char *pin_utf8;
  int alt = 0;
#ifndef HAVE_DOSISH_SYSTEM
  int no_input = 1;
#endif

#ifdef HAVE_NCURSESW
  char *old_ctype = NULL;

  if (pinentry->lc_ctype)
    {
      old_ctype = strdup (setlocale (LC_CTYPE, NULL));
      setlocale (LC_CTYPE, pinentry->lc_ctype);
    }
  else
    setlocale (LC_CTYPE, "");
#endif

  /* Open the desired terminal if necessary.  */
  if (tty_name)
    {
      ttyfi = fopen (tty_name, "r");
      if (!ttyfi)
        {
          pinentry->specific_err = gpg_error_from_syserror ();
          pinentry->specific_err_loc = "open_tty_for_read";
          return confirm_mode? 0 : -1;
        }
      ttyfo = fopen (tty_name, "w");
      if (!ttyfo)
	{
	  int err = errno;
	  fclose (ttyfi);
	  errno = err;
          pinentry->specific_err = gpg_error_from_syserror ();
          pinentry->specific_err_loc = "open_tty_for_write";
	  return confirm_mode? 0 : -1;
	}
      screen = newterm (tty_type, ttyfo, ttyfi);
      set_term (screen);
    }
  else
    {
      if (!init_screen)
	{
          if (!(isatty(fileno(stdin)) && isatty(fileno(stdout))))
            {
              errno = ENOTTY;
              pinentry->specific_err = gpg_error_from_syserror ();
              pinentry->specific_err_loc = "isatty";
              return confirm_mode? 0 : -1;
            }
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

  if (pinentry->ttyalert)
    {
      if (! strcmp(pinentry->ttyalert, "beep"))
	beep ();
      else if (! strcmp(pinentry->ttyalert, "flash"))
	flash ();
    }

  if (has_colors ())
    {
      start_color ();
#ifdef NCURSES_VERSION
      use_default_colors ();
#endif

      if (pinentry->color_so == PINENTRY_COLOR_DEFAULT)
	{
	  pinentry->color_so = PINENTRY_COLOR_RED;
	  pinentry->color_so_bright = 1;
	}
      if (COLOR_PAIRS >= 2)
	{
	  init_pair (1, pinentry_color[pinentry->color_fg],
		     pinentry_color[pinentry->color_bg]);
	  init_pair (2, pinentry_color[pinentry->color_so],
		     pinentry_color[pinentry->color_bg]);

	  bkgd (COLOR_PAIR (1));
	  attron (COLOR_PAIR (1) | (pinentry->color_fg_bright ? A_BOLD : 0));
	}
    }
  refresh ();

  /* Create the dialog.  */
  if (dialog_create (pinentry, &diag))
    {
      /* Note: pinentry->specific_err has already been set.  */
      endwin ();
      if (screen)
        delscreen (screen);

#ifdef HAVE_NCURSESW
      if (old_ctype)
        {
          setlocale (LC_CTYPE, old_ctype);
          free (old_ctype);
        }
#endif
      if (ttyfi)
        fclose (ttyfi);
      if (ttyfo)
        fclose (ttyfo);
      return -2;
    }
  dialog_switch_pos (&diag, confirm_mode? DIALOG_POS_OK : DIALOG_POS_PIN);

#ifndef HAVE_DOSISH_SYSTEM
  wtimeout (stdscr, 70);
#endif

  do
    {
      int c;

      c = wgetch (stdscr);     /* Refresh, accept single keystroke of input.  */
#ifndef HAVE_DOSISH_SYSTEM
      if (timed_out && no_input)
	{
	  done = -2;
          pinentry->specific_err = gpg_error (GPG_ERR_TIMEOUT);
	  break;
	}
#endif

      switch (c)
	{
	case ERR:
#ifndef HAVE_DOSISH_SYSTEM
	  continue;
#else
          done = -2;
          break;
#endif

	case 27: /* Alt was pressed.  */
	  alt = 1;
	  /* Get the next key press.  */
	  continue;

	case KEY_LEFT:
	case KEY_UP:
	  switch (diag.pos)
	    {
	    case DIALOG_POS_OK:
	      if (!confirm_mode)
		dialog_switch_pos (&diag, DIALOG_POS_PIN);
	      break;
	    case DIALOG_POS_NOTOK:
	      dialog_switch_pos (&diag, DIALOG_POS_OK);
	      break;
	    case DIALOG_POS_CANCEL:
	      if (diag.notok)
		dialog_switch_pos (&diag, DIALOG_POS_NOTOK);
	      else
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
	      if (diag.notok)
		dialog_switch_pos (&diag, DIALOG_POS_NOTOK);
	      else
		dialog_switch_pos (&diag, DIALOG_POS_CANCEL);
	      break;
	    case DIALOG_POS_NOTOK:
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
	      if (diag.notok)
		dialog_switch_pos (&diag, DIALOG_POS_NOTOK);
	      else
		dialog_switch_pos (&diag, DIALOG_POS_CANCEL);
	      break;
	    case DIALOG_POS_NOTOK:
	      dialog_switch_pos (&diag, DIALOG_POS_CANCEL);
	      break;
	    case DIALOG_POS_CANCEL:
	      if (confirm_mode)
		dialog_switch_pos (&diag, DIALOG_POS_OK);
	      else
		dialog_switch_pos (&diag, DIALOG_POS_PIN);
	      break;
	    default:
	      break;
	    }
	  break;

	case '\005':
	  done = -2;
	  break;

	case '\r':
	  switch (diag.pos)
	    {
	    case DIALOG_POS_PIN:
	    case DIALOG_POS_OK:
	      done = 1;
	      break;
	    case DIALOG_POS_NOTOK:
	      done = -1;
	      break;
	    case DIALOG_POS_CANCEL:
	      done = -2;
	      break;
            case DIALOG_POS_NONE:
              break;
	    }
	  break;

	default:
	  if (diag.pos == DIALOG_POS_PIN)
	    dialog_input (&diag, alt, c);
	}
#ifndef HAVE_DOSISH_SYSTEM
      no_input = 0;
#endif
      if (c != -1)
	alt = 0;
    }
  while (!done);

  if (!confirm_mode)
    {
      /* NUL terminate the passphrase.  dialog_run makes sure there is
         enough space for the terminating NUL byte.  */
      diag.pinentry->pin[diag.pin_len] = 0;
    }

  set_cursor_state (1);
  endwin ();
  if (screen)
    delscreen (screen);

#ifdef HAVE_NCURSESW
  if (old_ctype)
    {
      setlocale (LC_CTYPE, old_ctype);
      free (old_ctype);
    }
#endif
  if (ttyfi)
    fclose (ttyfi);
  if (ttyfo)
    fclose (ttyfo);
  /* XXX Factor out into dialog_release or something.  */
  free (diag.ok);
  if (diag.cancel)
    free (diag.cancel);
  if (diag.notok)
    free (diag.notok);

  if (!confirm_mode)
    {
      pinentry->locale_err = 1;
      pin_utf8 = pinentry_local_to_utf8 (pinentry->lc_ctype, pinentry->pin, 1);
      if (pin_utf8)
	{
	  pinentry_setbufferlen (pinentry, strlen (pin_utf8) + 1);
	  if (pinentry->pin)
	    strcpy (pinentry->pin, pin_utf8);
	  secmem_free (pin_utf8);
	  pinentry->locale_err = 0;
	}
    }

  if (done == -2)
    pinentry->canceled = 1;

  /* In confirm mode return cancel instead of error.  */
  if (confirm_mode)
    return done < 0 ? 0 : 1;

  return done < 0 ? -1 : diag.pin_len;
}


/* If a touch has been registered, touch that file.  */
static void
do_touch_file (pinentry_t pinentry)
{
#ifdef HAVE_UTIME_H
  struct stat st;
  time_t tim;

  if (!pinentry->touch_file || !*pinentry->touch_file)
    return;

  if (stat (pinentry->touch_file, &st))
    return; /* Oops.  */

  /* Make sure that we actually update the mtime.  */
  while ( (tim = time (NULL)) == st.st_mtime )
    sleep (1);

  /* Update but ignore errors as we can't do anything in that case.
     Printing error messages may even clubber the display further. */
  utime (pinentry->touch_file, NULL);
#endif /*HAVE_UTIME_H*/
}

#ifndef HAVE_DOSISH_SYSTEM
static void
catchsig (int sig)
{
  if (sig == SIGALRM)
    timed_out = 1;
}
#endif

int
curses_cmd_handler (pinentry_t pinentry)
{
  int rc;

#ifndef HAVE_DOSISH_SYSTEM
  timed_out = 0;

  if (pinentry->timeout)
    {
      struct sigaction sa;

      memset (&sa, 0, sizeof(sa));
      sa.sa_handler = catchsig;
      sigaction (SIGALRM, &sa, NULL);
      alarm (pinentry->timeout);
    }
#endif

  rc = dialog_run (pinentry, pinentry->ttyname, pinentry->ttytype);
  do_touch_file (pinentry);
  return rc;
}
