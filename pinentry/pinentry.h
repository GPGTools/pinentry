/* pinentry.h - The interface for the PIN entry support library.
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

#ifndef PINENTRY_H
#define PINENTRY_H

#ifdef __cplusplus
extern "C" {
#endif

struct pinentry
{
  /* The description to display, or NULL.  */
  char *description;
  /* The error message to display, or NULL.  */
  char *error;
  /* The prompt to display, or NULL.  */
  char *prompt;
  /* The OK button text to display, or NULL.  */
  char *ok;
  /* The Cancel button text to display, or NULL.  */
  char *cancel;
  /* The buffer to store the secret into.  */
  char *pin;
  /* The length of the buffer.  */
  int pin_len;

  /* The name of the X display to use if X is available and supported.  */
  char *display;
  /* The name of the terminal node to open if X not available or supported.  */
  char *ttyname;
  /* The type of the terminal.  */
  char *ttytype;
  /* The LC_CTYPE value for the terminal.  */
  char *lc_ctype;
  /* The LC_MESSAGES value for the terminal.  */
  char *lc_messages;

  /* True if debug mode is requested.  */
  int debug;
  /* True if enhanced mode is requested.  */
  int enhanced;
  /* True if caller should grab the keyboard.  */
  int grab;

  /* The user should set this to -1 if the user canceled the request,
     and to the length of the PIN stored in pin otherwise.  */
  int result;
};
typedef struct pinentry *pinentry_t;


/* The pinentry command handler type processes the pinentry request
   PIN.  If PIN->pin is zero, request a confirmation, otherwise a PIN
   entry.  On confirmation, the function should return TRUE if
   confirmed, and FALSE otherwise.  On PIN entry, the function should
   return -1 if cancelled and the length of the secret otherwise.  */
typedef int (*pinentry_cmd_handler_t) (pinentry_t pin);

/* Start the pinentry event loop.  The program will start to process
   Assuan commands until it is finished or an error occurs.  If an
   error occurs, -1 is returned and errno indicates the type of an
   error.  Otherwise, 0 is returned.  */
int pinentry_loop (void);

/* Try to make room for at least LEN bytes for the pin in the pinentry
   PIN.  Returns new buffer on success and 0 on failure.  */
char *pinentry_setbufferlen (pinentry_t pin, int len);

/* Initialize the secure memory subsystem, drop privileges and
   return.  Must be called early.  */
void pinentry_init (void);

/* Return true if either DISPLAY is set or ARGV contains the string
   "--display". */
int pinentry_have_display (int argc, char **argv);

/* Parse the command line options.  Returns 1 if user should print
   version and exit.  Can exit the program if only help output is
   requested.  */
int pinentry_parse_opts (int argc, char *argv[]);


/* The caller must define this variable to process assuan commands.  */
extern pinentry_cmd_handler_t pinentry_cmd_handler;

#ifdef __cplusplus
}
#endif

#endif	/* PINENTRY_H */
