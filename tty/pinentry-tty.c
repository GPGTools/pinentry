/* pinentry-tty.c - A minimalist dumb terminal mechanism for PIN entry
 * Copyright (C) 2014 Serge Voilokov
 * Copyright (C) 2015 Daniel Kahn Gillmor <dkg@fifthhorseman.net>
 * Copyright (C) 2015 g10 Code GmbH
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
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <termios.h>
#ifdef HAVE_UTIME_H
#include <utime.h>
#endif /*HAVE_UTIME_H*/
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <gpg-error.h>

#include "pinentry.h"
#include "memory.h"

#ifndef HAVE_DOSISH_SYSTEM
static int timed_out;
#endif

static struct termios n_term;
static struct termios o_term;

static int
terminal_save (int fd)
{
  if ((tcgetattr (fd, &o_term)) == -1)
    return -1;
  return 0;
}

static void
terminal_restore (int fd)
{
  tcsetattr (fd, TCSANOW, &o_term);
}

static int
terminal_setup (int fd, int line_edit)
{
  n_term = o_term;
  if (line_edit)
    n_term.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
  else
    {
      n_term.c_lflag &= ~(ECHO|ICANON);
      n_term.c_lflag |= ISIG;
      n_term.c_cc[VMIN] = 1;
      n_term.c_cc[VTIME]= 0;
    }
  if ((tcsetattr(fd, TCSAFLUSH, &n_term)) == -1)
    return -1;
  return 1;
}

#define UNDERLINE_START "\033[4m"
/* Bold, red.  */
#define ALERT_START "\033[1;31m"
#define NORMAL_RESTORE "\033[0m"

static void
fputs_highlighted (char *text, char *highlight, FILE *ttyfo)
{
  for (; *text; text ++)
    {
      /* Skip accelerator prefix.  */
      if (*text == '_')
        {
          text ++;
          if (! *text)
            break;
        }

      if (text == highlight)
        fputs (UNDERLINE_START, ttyfo);
      fputc (*text, ttyfo);
      if (text == highlight)
        fputs (NORMAL_RESTORE, ttyfo);
    }
}

static char
button (char *text, char *default_text, FILE *ttyfo)
{
  char *highlight;
  int use_default = 0;

  if (! text)
    return 0;

  /* Skip any leading white space.  */
  while (*text == ' ')
    text ++;

  highlight = text;
  while ((highlight = strchr (highlight, '_')))
    {
      highlight = highlight + 1;
      if (*highlight == '_')
        {
          /* Escaped underscore.  Skip both characters.  */
          highlight++;
          continue;
        }
      if (!isalnum (*highlight))
        /* Unusable accelerator.  */
        continue;
      break;
    }

  if (! highlight)
    /* Not accelerator.  Take the first alpha-numeric character.  */
    {
      highlight = text;
      while (*highlight && !isalnum (*highlight))
	highlight ++;
    }

  if (! *highlight)
    /* Hmm, no alpha-numeric characters.  */
    {
      if (! default_text)
	return 0;
      highlight = default_text;
      use_default = 1;
    }

  fputs ("  ", ttyfo);
  fputs_highlighted (text, highlight, ttyfo);
  if (use_default)
    {
      fputs (" (", ttyfo);
      fputs_highlighted (default_text, highlight, ttyfo);
      fputc (')', ttyfo);
    }
  fputc ('\n', ttyfo);

  return tolower (*highlight);
}

static void
dump_error_text (FILE *ttyfo, const char *text)
{
  int lines = 0;

  if (! text || ! *text)
    return;

  for (;;)
    {
      const char *eol = strchr (text, '\n');
      if (! eol)
	eol = text + strlen (text);

      lines ++;

      fwrite ("\n *** ", 6, 1, ttyfo);
      fputs (ALERT_START, ttyfo);
      fwrite (text, (size_t) (eol - text), 1, ttyfo);
      fputs (NORMAL_RESTORE, ttyfo);

      if (! *eol)
	break;

      text = eol + 1;
    }

  if (lines > 1)
    fputc ('\n', ttyfo);
  else
    fwrite (" ***\n", 5, 1, ttyfo);

  fputc ('\n', ttyfo);
}

static int
confirm (pinentry_t pinentry, FILE *ttyfi, FILE *ttyfo)
{
  char *msg;
  char *msgbuffer = NULL;

  char ok = 0;
  char notok = 0;
  char cancel = 0;

  int ret;

  dump_error_text (ttyfo, pinentry->error);

  msg = pinentry->description;
  if (! msg)
    {
      /* If there is no description, fallback to the title.  */
      msg = msgbuffer = pinentry_get_title (pinentry);
    }
  if (! msg)
    msg = "Confirm:";

  if (msg)
    {
      fputs (msg, ttyfo);
      fputc ('\n', ttyfo);
    }
  free (msgbuffer);

  fflush (ttyfo);

  if (pinentry->ok)
    ok = button (pinentry->ok, "OK", ttyfo);
  else if (pinentry->default_ok)
    ok = button (pinentry->default_ok, "OK", ttyfo);
  else
    ok = button ("OK", NULL, ttyfo);

  if (! pinentry->one_button)
    {
      if (pinentry->cancel)
	cancel = button (pinentry->cancel, "Cancel", ttyfo);
      else if (pinentry->default_cancel)
	cancel = button (pinentry->default_cancel, "Cancel", ttyfo);

      if (pinentry->notok)
	notok = button (pinentry->notok, "No", ttyfo);
    }

  while (1)
    {
      int input;

      if (pinentry->one_button)
        fprintf (ttyfo, "Press any key to continue.");
      else
        {
          fputc ('[', ttyfo);
          if (ok)
            fputc (ok, ttyfo);
          if (cancel)
            fputc (cancel, ttyfo);
          if (notok)
            fputc (notok, ttyfo);
          fputs("]? ", ttyfo);
        }
      fflush (ttyfo);

      input = fgetc (ttyfi);

      if (input == EOF)
	{
	  pinentry->close_button = 1;

	  pinentry->canceled = 1;

#ifndef HAVE_DOSISH_SYSTEM
          if (!timed_out && errno == EINTR)
            pinentry->specific_err = gpg_error (GPG_ERR_FULLY_CANCELED);
#endif

	  ret = 0;
	  break;
	}
      else
        {
          fprintf (ttyfo, "%c\n", input);
          input = tolower (input);
        }

      if (pinentry->one_button)
	{
	  ret = 1;
	  break;
	}

      if (cancel && input == cancel)
	{
	  pinentry->canceled = 1;
	  ret = 0;
	  break;
	}
      else if (notok && input == notok)
	{
	  ret = 0;
	  break;
	}
      else if (ok && input == ok)
	{
	  ret = 1;
	  break;
	}
      else
        {
          fprintf (ttyfo, "Invalid selection.\n");
        }
    }

#ifndef HAVE_DOSISH_SYSTEM
  if (timed_out)
    pinentry->specific_err = gpg_error (GPG_ERR_TIMEOUT);
#endif

  return ret;
}

static char *
read_password (pinentry_t pinentry, FILE *ttyfi, FILE *ttyfo)
{
  int done = 0;
  int len = 128;
  int count = 0;
  char *buffer;

  (void) ttyfo;

  buffer = secmem_malloc (len);
  if (! buffer)
    return NULL;

  while (!done)
    {
      int c;

      if (count == len - 1)
	/* Double the buffer's size.  Note: we check if count is len -
	   1 and not len so that we always have space for the NUL
	   character.  */
	{
	  int new_len = 2 * len;
	  char *tmp = secmem_realloc (buffer, new_len);
	  if (! tmp)
	    {
	      secmem_free (tmp);
	      return NULL;
	    }
	  buffer = tmp;
	  len = new_len;
	}

      c = fgetc (ttyfi);
      switch (c)
	{
        case EOF:
          done = -1;
#ifndef HAVE_DOSISH_SYSTEM
          if (!timed_out && errno == EINTR)
            pinentry->specific_err = gpg_error (GPG_ERR_FULLY_CANCELED);
#endif
	  break;

	case '\n':
	  done = 1;
	  break;

	default:
	  buffer[count ++] = c;
	  break;
	}
    }
  buffer[count] = '\0';

  if (done == -1)
    {
      secmem_free (buffer);
      return NULL;
    }

  return buffer;
}


static int
password (pinentry_t pinentry, FILE *ttyfi, FILE *ttyfo)
{
  char *msg;
  char *msgbuffer = NULL;
  int done = 0;

  msg = pinentry->description;
  if (! msg)
    msg = msgbuffer = pinentry_get_title (pinentry);
  if (! msg)
    msg = "Enter your passphrase.";

  dump_error_text (ttyfo, pinentry->error);

  fprintf (ttyfo, "%s\n", msg);
  free (msgbuffer);

  while (! done)
    {
      char *passphrase;

      char *prompt = pinentry->prompt;
      if (! prompt || !*prompt)
	prompt = "PIN";

      fprintf (ttyfo, "%s%s ",
	       prompt,
	       /* Make sure the prompt ends in a : or a question mark.  */
	       (prompt[strlen(prompt) - 1] == ':'
		|| prompt[strlen(prompt) - 1] == '?') ? "" : ":");
      fflush (ttyfo);

      passphrase = read_password (pinentry, ttyfi, ttyfo);
      fputc ('\n', ttyfo);
      if (! passphrase)
	{
	  done = -1;
	  break;
	}

      if (! pinentry->repeat_passphrase)
	done = 1;
      else
	{
	  char *passphrase2;

	  prompt = pinentry->repeat_passphrase;
	  fprintf (ttyfo, "%s%s ",
		   prompt,
		   /* Make sure the prompt ends in a : or a question mark.  */
		   (prompt[strlen(prompt) - 1] == ':'
		    || prompt[strlen(prompt) - 1] == '?') ? "" : ":");
	  fflush (ttyfo);

	  passphrase2 = read_password (pinentry, ttyfi, ttyfo);
	  fputc ('\n', ttyfo);
	  if (! passphrase2)
	    {
	      done = -1;
	      break;
	    }

	  if (strcmp (passphrase, passphrase2) == 0)
	    {
	      pinentry->repeat_okay = 1;
	      done = 1;
	    }
	  else
	    dump_error_text (ttyfo,
			     pinentry->repeat_error_string
			     ?: "Passphrases don't match.");

	  secmem_free (passphrase2);
	}

      if (done == 1)
	pinentry_setbuffer_use (pinentry, passphrase, 0);
      else
	secmem_free (passphrase);
    }

#ifndef HAVE_DOSISH_SYSTEM
  if (timed_out)
    pinentry->specific_err = gpg_error (GPG_ERR_TIMEOUT);
#endif

  return done;
}


/* If a touch has been registered, touch that file.  */
static void
do_touch_file(pinentry_t pinentry)
{
#ifdef HAVE_UTIME_H
  struct stat st;
  time_t tim;

  if (!pinentry->touch_file || !*pinentry->touch_file)
    return;

  if (stat(pinentry->touch_file, &st))
    return; /* Oops.  */

  /* Make sure that we actually update the mtime.  */
  while ((tim = time(NULL)) == st.st_mtime)
    sleep(1);

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
tty_cmd_handler (pinentry_t pinentry)
{
  int rc = 0;
  FILE *ttyfi = stdin;
  FILE *ttyfo = stdout;
  int saved_errno = 0;

#ifndef HAVE_DOSISH_SYSTEM
  timed_out = 0;

  if (pinentry->timeout)
    {
      struct sigaction sa;

      memset (&sa, 0, sizeof(sa));
      sa.sa_handler = catchsig;
      sigaction (SIGALRM, &sa, NULL);
      sigaction (SIGINT, &sa, NULL);
      alarm (pinentry->timeout);
    }
#endif

  if (pinentry->ttyname)
    {
      ttyfi = fopen (pinentry->ttyname, "r");
      if (!ttyfi)
        return -1;

      ttyfo = fopen (pinentry->ttyname, "w");
      if (!ttyfo)
        {
          saved_errno = errno;
          fclose (ttyfi);
          errno = saved_errno;
          return -1;
        }
    }

  if (terminal_save (fileno (ttyfi)) < 0)
    rc = -1;
  else
    {
      if (terminal_setup (fileno (ttyfi), !!pinentry->pin) == -1)
        {
          saved_errno = errno;
          fprintf (stderr, "terminal_setup failure, exiting\n");
          rc = -1;
        }
      else
        {
          if (pinentry->pin)
            rc = password (pinentry, ttyfi, ttyfo);
          else
            rc = confirm (pinentry, ttyfi, ttyfo);

          terminal_restore (fileno (ttyfi));
          do_touch_file (pinentry);
        }
    }

  if (pinentry->ttyname)
    {
      fclose (ttyfi);
      fclose (ttyfo);
    }

  if (saved_errno)
    errno = saved_errno;

  return rc;
}


pinentry_cmd_handler_t pinentry_cmd_handler = tty_cmd_handler;



int
main (int argc, char *argv[])
{
  pinentry_init ("pinentry-tty");

  /* Consumes all arguments.  */
  pinentry_parse_opts(argc, argv);

  if (pinentry_loop ())
    return 1;

  return 0;
}
