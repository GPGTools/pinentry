/* pinentry-curses.c - A secure curses dialog for PIN entry, library version
   Copyright (C) 2014 Serge Voilokov
   Copyright (C) 2015 Daniel Kahn Gillmor <dkg@fifthhorseman.net>
   Copyright (C) 2015 g10 Code GmbH

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
   along with this program; if not, see <http://www.gnu.org/licenses/>.
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

#include "pinentry.h"

#ifndef HAVE_DOSISH_SYSTEM
static int timed_out;
#endif

static struct termios n_term;
static struct termios o_term;

static int
cbreak (int fd)
{
  if ((tcgetattr(fd, &o_term)) == -1)
    return -1;
  n_term = o_term;
  n_term.c_lflag = n_term.c_lflag & ~(ECHO|ICANON);
  n_term.c_cc[VMIN] = 1;
  n_term.c_cc[VTIME]= 0;
  if ((tcsetattr(fd, TCSAFLUSH, &n_term)) == -1)
    return -1;
  return 1;
}

static int
confirm (pinentry_t pinentry, FILE *ttyfi, FILE *ttyfo)
{
  char buf[32], *ret;
  pinentry->canceled = 1;
  fprintf (ttyfo, "%s [y/N]? ", pinentry->ok ? pinentry->ok : "OK");
  fflush (ttyfo);
  buf[0] = '\0';
  ret = fgets (buf, sizeof(buf), ttyfi);
  if (ret && (buf[0] == 'y' || buf[0] == 'Y'))
    {
      pinentry->canceled = 0;
      return 1;
    }
  return 0;
}


static int
read_password (pinentry_t pinentry, FILE *ttyfi, FILE *ttyfo)
{
  int count;
  int done;
  char *prompt = NULL;

  if (cbreak (fileno (ttyfi)) == -1)
    {
      int err = errno;
      fprintf (stderr, "cbreak failure, exiting\n");
      errno = err;
      return -1;
    }

  prompt = pinentry->prompt;
  if (! prompt || !*prompt)
    prompt = "PIN";

  fprintf (ttyfo, "%s\n%s%s ",
           pinentry->description? pinentry->description:"",
           prompt,
	   /* Make sure the prompt ends in a : or a question mark.  */
	   (prompt[strlen(prompt) - 1] == ':'
	    || prompt[strlen(prompt) - 1] == '?') ? "" : ":");
  fflush (ttyfo);

  memset (pinentry->pin, 0, pinentry->pin_len);

  done = count = 0;
  while (!done && count < pinentry->pin_len - 1)
    {
      char c = fgetc (ttyfi);

      switch (c)
	{
	case '\n':
	  done = 1;
	  break;

	case 0x7f:
	  /* Backspace.  */
	  if (count > 0)
	    count --;
	  break;

	default:
	  pinentry->pin[count ++] = c;
	  break;
	}
    }
  pinentry->pin[count] = '\0';
  fputc('\n', stdout);

  tcsetattr (fileno(ttyfi), TCSANOW, &o_term);
  return strlen (pinentry->pin);
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
catchsig(int sig)
{
  if (sig == SIGALRM)
    timed_out = 1;
}
#endif

int
tty_cmd_handler(pinentry_t pinentry)
{
  int rc = 0;
  FILE *ttyfi = stdin;
  FILE *ttyfo = stdout;

#ifndef HAVE_DOSISH_SYSTEM
  timed_out = 0;

  if (pinentry->timeout)
    {
      struct sigaction sa;

      memset(&sa, 0, sizeof(sa));
      sa.sa_handler = catchsig;
      sigaction(SIGALRM, &sa, NULL);
      alarm(pinentry->timeout);
    }
#endif

  if (pinentry->ttyname)
    {
      ttyfi = fopen (pinentry->ttyname, "r");
      if (!ttyfi)
        rc = -1;
      else
        {
          ttyfo = fopen (pinentry->ttyname, "w");
          if (!ttyfo)
            {
              int err = errno;
              fclose (ttyfi);
              errno = err;
              rc = -1;
            }
        }
    }

  if (rc == 0)
    {
      if (pinentry->pin)
        rc = read_password (pinentry, ttyfi, ttyfo);
      else
        {
          fprintf (ttyfo, "%s\n",
                   pinentry->description? pinentry->description:"");
          fflush (ttyfo);

	  /* If pinentry->one_button is set, then
	     pinentry->description contains an informative message,
	     which the user needs to dismiss.  Since we are showing
	     this in a terminal, there is no window to dismiss.  */
          if (! pinentry->one_button)
            rc = confirm (pinentry, ttyfi, ttyfo);
        }
      do_touch_file (pinentry);
    }

  if (pinentry->ttyname)
    {
      fclose (ttyfi);
      fclose (ttyfo);
    }

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
