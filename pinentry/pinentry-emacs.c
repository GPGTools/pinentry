/* pinentry-emacs.c - A secure emacs dialog for PIN entry, library version
 * Copyright (C) 2015 Daiki Ueno
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
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#include <assert.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#ifdef HAVE_UTIME_H
#include <utime.h>
#endif /*HAVE_UTIME_H*/

#include <assuan.h>

#include "pinentry-emacs.h"
#include "memory.h"
#include "secmem-util.h"

/* The communication mechanism is similar to emacsclient, but there
   are a few differences:

   - To avoid unnecessary character escaping and encoding conversion,
     we use a subset of the Pinentry Assuan protocol, instead of the
     emacsclient protocol.

   - We only use a Unix domain socket, while emacsclient has an
     ability to use a TCP socket.  The socket file is located at
     ${TMPDIR-/tmp}/emacs$(id -u)/pinentry (i.e., under the same
     directory as the socket file used by emacsclient, so the same
     permission and file owner settings apply).

   - The server implementation can be found in pinentry.el, which is
     available in Emacs 25+ or from ELPA.  */

#define LINELENGTH ASSUAN_LINELENGTH
#define SEND_BUFFER_SIZE 4096
#define INITIAL_TIMEOUT 60

static int initial_timeout = INITIAL_TIMEOUT;

#undef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))

#undef MAX
#define MAX(x, y) ((x) < (y) ? (y) : (x))

#ifndef SUN_LEN
# define SUN_LEN(ptr) ((size_t) (((struct sockaddr_un *) 0)->sun_path) \
                       + strlen ((ptr)->sun_path))
#endif

/* FIXME: We could use the I/O functions in Assuan directly, once
   Pinentry links to libassuan.  */
static int emacs_socket = -1;
static char send_buffer[SEND_BUFFER_SIZE + 1];
static int send_buffer_length; /* Fill pointer for the send buffer.  */

static pinentry_cmd_handler_t fallback_cmd_handler;

#ifndef HAVE_DOSISH_SYSTEM
static int timed_out;
#endif

static int
set_socket (const char *socket_name)
{
  struct sockaddr_un unaddr;
  struct stat statbuf;
  const char *tmpdir;
  char *tmpdir_storage = NULL;
  char *socket_name_storage = NULL;
  uid_t uid;

  unaddr.sun_family = AF_UNIX;

  /* We assume 32-bit UIDs, which can be represented with 10 decimal
     digits.  */
  uid = getuid ();
  if (uid != (uint32_t) uid)
    {
      fprintf (stderr, "UID is too large\n");
      return 0;
    }

  tmpdir = getenv ("TMPDIR");
  if (!tmpdir)
    {
#ifdef _CS_DARWIN_USER_TEMP_DIR
      size_t n = confstr (_CS_DARWIN_USER_TEMP_DIR, NULL, (size_t) 0);
      if (n > 0)
	{
	  tmpdir = tmpdir_storage = malloc (n);
	  if (!tmpdir)
	    {
	      fprintf (stderr, "out of core\n");
	      return 0;
	    }
	  confstr (_CS_DARWIN_USER_TEMP_DIR, tmpdir_storage, n);
	}
      else
#endif
	tmpdir = "/tmp";
    }

  socket_name_storage = malloc (strlen (tmpdir)
				+ strlen ("/emacs") + 10 + strlen ("/")
				+ strlen (socket_name)
				+ 1);
  if (!socket_name_storage)
    {
      fprintf (stderr, "out of core\n");
      free (tmpdir_storage);
      return 0;
    }

  sprintf (socket_name_storage, "%s/emacs%u/%s", tmpdir,
	   (uint32_t) uid, socket_name);
  free (tmpdir_storage);

  if (strlen (socket_name_storage) >= sizeof (unaddr.sun_path))
    {
      fprintf (stderr, "socket name is too long\n");
      free (socket_name_storage);
      return 0;
    }

  strcpy (unaddr.sun_path, socket_name_storage);
  free (socket_name_storage);

  /* See if the socket exists, and if it's owned by us. */
  if (stat (unaddr.sun_path, &statbuf) == -1)
    {
      perror ("stat");
      return 0;
    }

  if (statbuf.st_uid != geteuid ())
    {
      fprintf (stderr, "socket is not owned by the same user\n");
      return 0;
    }

  emacs_socket = socket (AF_UNIX, SOCK_STREAM, 0);
  if (emacs_socket < 0)
    {
      perror ("socket");
      return 0;
    }

  if (connect (emacs_socket, (struct sockaddr *) &unaddr,
	       SUN_LEN (&unaddr)) < 0)
    {
      perror ("connect");
      close (emacs_socket);
      emacs_socket = -1;
      return 0;
    }

  return 1;
}

/* Percent-escape control characters in DATA.  Return a newly
   allocated string.  */
static char *
escape (const char *data)
{
  char *buffer, *out_p;
  size_t length, buffer_length;
  size_t offset;
  size_t count = 0;

  length = strlen (data);
  for (offset = 0; offset < length; offset++)
    {
      switch (data[offset])
	{
	case '%': case '\n': case '\r':
	  count++;
	  break;
	default:
	  break;
	}
    }

  buffer_length = length + count * 2;
  buffer = malloc (buffer_length + 1);
  if (!buffer)
    return NULL;

  out_p = buffer;
  for (offset = 0; offset < length; offset++)
    {
      int c = data[offset];
      switch (c)
	{
	case '%': case '\n': case '\r':
	  sprintf (out_p, "%%%02X", c);
	  out_p += 3;
	  break;
	default:
	  *out_p++ = c;
	  break;
	}
    }
  *out_p = '\0';

  return buffer;
}

/* The inverse of escape.  Unlike escape, it removes quoting in string
   DATA by modifying the string in place, to avoid copying of secret
   data sent from Emacs.  */
static char *
unescape (char *data)
{
  char *p = data, *q = data;

  while (*p)
    {
      if (*p == '%' && p[1] && p[2])
        {
          p++;
          *q++ = xtoi_2 (p);
	  p += 2;
        }
      else
	*q++ = *p++;
    }
  *q = 0;
  return data;
}

/* Let's send the data to Emacs when either
   - the data ends in "\n", or
   - the buffer is full (but this shouldn't happen)
   Otherwise, we just accumulate it.  */
static int
send_to_emacs (int s, const char *buffer)
{
  size_t length;

  length = strlen (buffer);
  while (*buffer)
    {
      size_t part = MIN (length, SEND_BUFFER_SIZE - send_buffer_length);
      memcpy (&send_buffer[send_buffer_length], buffer, part);
      buffer += part;
      send_buffer_length += part;

      if (send_buffer_length == SEND_BUFFER_SIZE
	  || (send_buffer_length > 0
	      && send_buffer[send_buffer_length-1] == '\n'))
	{
	  int sent = send (s, send_buffer, send_buffer_length, 0);
	  if (sent < 0)
	    {
	      fprintf (stderr, "failed to send %d bytes to socket: %s\n",
		       send_buffer_length, strerror (errno));
	      send_buffer_length = 0;
	      return 0;
	    }
	  if (sent != send_buffer_length)
	    memmove (send_buffer, &send_buffer[sent],
		     send_buffer_length - sent);
	  send_buffer_length -= sent;
	}

      length -= part;
    }

  return 1;
}

/* Read a server response.  If the response contains data, it will be
   stored in BUFFER with a terminating NUL byte.  BUFFER must be
   at least as large as CAPACITY.  */
static gpg_error_t
read_from_emacs (int s, int timeout, char *buffer, size_t capacity)
{
  struct timeval tv;
  fd_set rfds;
  int retval;
  /* Offset in BUFFER.  */
  size_t offset = 0;
  int got_response = 0;
  char read_buffer[LINELENGTH + 1];
  /* Offset in READ_BUFFER.  */
  size_t read_offset = 0;
  gpg_error_t result = 0;

  tv.tv_sec = timeout;
  tv.tv_usec = 0;

  FD_ZERO (&rfds);
  FD_SET (s, &rfds);
  retval = select (s + 1, &rfds, NULL, NULL, &tv);
  if (retval == -1)
    {
      perror ("select");
      return gpg_error (GPG_ERR_ASS_GENERAL);
    }
  else if (retval == 0)
    {
      timed_out = 1;
      return gpg_error (GPG_ERR_TIMEOUT);
    }

  /* Loop until we get either OK or ERR.  */
  while (!got_response)
    {
      int rl = 0;
      char *p, *end_p;
      do
	{
	  errno = 0;
	  rl = recv (s, read_buffer + read_offset, LINELENGTH - read_offset, 0);
	}
      /* If we receive a signal (e.g. SIGWINCH, which we pass
	 through to Emacs), on some OSes we get EINTR and must retry. */
      while (rl < 0 && errno == EINTR);

      if (rl < 0)
	{
	  perror ("recv");
	  return gpg_error (GPG_ERR_ASS_GENERAL);;
	}
      if (rl == 0)
	break;

      read_offset += rl;
      read_buffer[read_offset] = '\0';

      end_p = strchr (read_buffer, '\n');

      /* If the buffer is filled without NL, throw away the content
	 and start over the buffering.

	 FIXME: We could return ASSUAN_Line_Too_Long or
	 ASSUAN_Line_Not_Terminated here.  */
      if (!end_p && read_offset == sizeof (read_buffer) - 1)
	{
	  read_offset = 0;
	  continue;
	}

      /* Loop over all NL-terminated messages.  */
      for (p = read_buffer; end_p; p = end_p + 1, end_p = strchr (p, '\n'))
	{
	  *end_p = '\0';
	  if (!strncmp ("D ", p, 2))
	    {
	      char *data;
	      size_t data_length;
	      size_t needed_capacity;

	      data = p + 2;
	      data_length = end_p - data;
	      if (data_length > 0)
		{
		  needed_capacity = offset + data_length + 1;

		  /* Check overflow.  This is unrealistic but can
		     happen since OFFSET is cumulative.  */
		  if (needed_capacity < offset)
		    return gpg_error (GPG_ERR_ASS_GENERAL);;

		  if (needed_capacity > capacity)
		    return gpg_error (GPG_ERR_ASS_GENERAL);;

		  memcpy (&buffer[offset], data, data_length);
		  offset += data_length;
		  buffer[offset] = 0;
		}
	    }
          else if (!strcmp ("OK", p) || !strncmp ("OK ", p, 3))
            {
	      got_response = 1;
	      break;
            }
          else if (!strncmp ("ERR ", p, 4))
            {
	      unsigned long code = strtoul (p + 4, NULL, 10);
	      if (code == ULONG_MAX && errno == ERANGE)
		return gpg_error (GPG_ERR_ASS_GENERAL);
	      else
		result = code;
	      got_response = 1;
	      break;
            }
	  else if (*p == '#')
	    ;
	  else
	    fprintf (stderr, "invalid response: %s\n", p);
	}

      if (!got_response)
	{
	  size_t length = &read_buffer[read_offset] - p;
	  memmove (read_buffer, p, length);
	  read_offset = length;
	}
    }

  return result;
}

int
set_label (pinentry_t pe, const char *name, const char *value)
{
  char buffer[16], *escaped;
  gpg_error_t error;
  int retval;

  if (!send_to_emacs (emacs_socket, name)
      || !send_to_emacs (emacs_socket, " "))
    return 0;

  escaped = escape (value);
  if (!escaped)
    return 0;

  retval = send_to_emacs (emacs_socket, escaped)
    && send_to_emacs (emacs_socket, "\n");

  free (escaped);
  if (!retval)
    return 0;

  error = read_from_emacs (emacs_socket, pe->timeout, buffer, sizeof (buffer));
  return error == 0;
}

static void
set_labels (pinentry_t pe)
{
  char *p;

  p = pinentry_get_title (pe);
  if (p)
    {
      set_label (pe, "SETTITLE", p);
      free (p);
    }
  if (pe->description)
    set_label (pe, "SETDESC", pe->description);
  if (pe->error)
    set_label (pe, "SETERROR", pe->error);
  if (pe->prompt)
    set_label (pe, "SETPROMPT", pe->prompt);
  else if (pe->default_prompt)
    set_label (pe, "SETPROMPT", pe->default_prompt);
  if (pe->repeat_passphrase)
    set_label (pe, "SETREPEAT", pe->repeat_passphrase);
  if (pe->repeat_error_string)
    set_label (pe, "SETREPEATERROR", pe->repeat_error_string);

  /* XXX: pe->quality_bar and pe->quality_bar_tt are not supported.  */

  /* Buttons.  */
  if (pe->ok)
    set_label (pe, "SETOK", pe->ok);
  else if (pe->default_ok)
    set_label (pe, "SETOK", pe->default_ok);
  if (pe->cancel)
    set_label (pe, "SETCANCEL", pe->cancel);
  else if (pe->default_ok)
    set_label (pe, "SETCANCEL", pe->default_cancel);
  if (pe->notok)
    set_label (pe, "SETNOTOK", pe->notok);
}

static int
do_password (pinentry_t pe)
{
  char *buffer, *password;
  size_t length = LINELENGTH;
  gpg_error_t error;

  set_labels (pe);

  if (!send_to_emacs (emacs_socket, "GETPIN\n"))
    return -1;

  buffer = secmem_malloc (length);
  if (!buffer)
    {
      pe->specific_err = gpg_error (GPG_ERR_ENOMEM);
      return -1;
    }

  error = read_from_emacs (emacs_socket, pe->timeout, buffer, length);
  if (error != 0)
    {
      if (gpg_err_code (error) == GPG_ERR_CANCELED)
	pe->canceled = 1;

      secmem_free (buffer);
      pe->specific_err = error;
      return -1;
    }

  password = unescape (buffer);
  pinentry_setbufferlen (pe, strlen (password) + 1);
  if (pe->pin)
    strcpy (pe->pin, password);
  secmem_free (buffer);

  if (pe->repeat_passphrase)
    pe->repeat_okay = 1;

  /* XXX: we don't support external password cache (yet).  */

  return 1;
}

static int
do_confirm (pinentry_t pe)
{
  char buffer[16];
  gpg_error_t error;

  set_labels (pe);

  if (!send_to_emacs (emacs_socket, "CONFIRM\n"))
    return 0;

  error = read_from_emacs (emacs_socket, pe->timeout, buffer, sizeof (buffer));
  if (error != 0)
    {
      if (gpg_err_code (error) == GPG_ERR_CANCELED)
	pe->canceled = 1;

      pe->specific_err = error;
      return 0;
    }

  return 1;
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
emacs_cmd_handler (pinentry_t pe)
{
  int rc;

#ifndef HAVE_DOSISH_SYSTEM
  timed_out = 0;

  if (pe->timeout)
    {
      struct sigaction sa;

      memset (&sa, 0, sizeof(sa));
      sa.sa_handler = catchsig;
      sigaction (SIGALRM, &sa, NULL);
      alarm (pe->timeout);
    }
#endif

  if (pe->pin)
    rc = do_password (pe);
  else
    rc = do_confirm (pe);

  do_touch_file (pe);
  return rc;
}

static int
initial_emacs_cmd_handler (pinentry_t pe)
{
  /* Let the select() call in pinentry_emacs_init honor the timeout
     value set through an Assuan option.  */
  initial_timeout = pe->timeout;

  if (emacs_socket < 0)
    pinentry_emacs_init ();

  /* If we have successfully connected to Emacs, swap
     pinentry_cmd_handler to emacs_cmd_handler, so further
     interactions will be forwarded to Emacs.  Otherwise, set it back
     to the original command handler saved as
     fallback_cmd_handler.  */
  if (emacs_socket < 0)
    pinentry_cmd_handler = fallback_cmd_handler;
  else
    {
      pinentry_cmd_handler = emacs_cmd_handler;
      pinentry_set_flavor_flag ("emacs");
    }

  return (* pinentry_cmd_handler) (pe);
}

void
pinentry_enable_emacs_cmd_handler (void)
{
  const char *envvar;

  /* Check if pinentry_cmd_handler is already prepared for Emacs.  */
  if (pinentry_cmd_handler == initial_emacs_cmd_handler
      || pinentry_cmd_handler == emacs_cmd_handler)
    return;

  /* Check if INSIDE_EMACS envvar is set.  */
  envvar = getenv ("INSIDE_EMACS");
  if (!envvar || !*envvar)
    return;

  /* Save the original command handler as fallback_cmd_handler, and
     swap pinentry_cmd_handler to initial_emacs_cmd_handler.  */
  fallback_cmd_handler = pinentry_cmd_handler;
  pinentry_cmd_handler = initial_emacs_cmd_handler;
}

int
pinentry_emacs_init (void)
{
  char buffer[256];
  gpg_error_t error;

  assert (emacs_socket < 0);

  /* Check if we can connect to the Emacs server socket.  */
  if (!set_socket ("pinentry"))
    return 0;

  /* Check if the server responds.  */
  error = read_from_emacs (emacs_socket, initial_timeout,
			   buffer, sizeof (buffer));
  if (error != 0)
    {
      close (emacs_socket);
      emacs_socket = -1;
      return 0;
    }
  return 1;
}
