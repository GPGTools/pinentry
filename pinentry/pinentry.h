/* pinentry.h - The interface for the PIN entry support library.
   Copyright (C) 2002, 2003, 2010 g10 Code GmbH
   
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

#ifndef PINENTRY_H
#define PINENTRY_H

#ifdef __cplusplus
extern "C" {
#if 0 
}
#endif
#endif

#undef ENABLE_ENHANCED

typedef enum {
  PINENTRY_COLOR_NONE, PINENTRY_COLOR_DEFAULT,
  PINENTRY_COLOR_BLACK, PINENTRY_COLOR_RED,
  PINENTRY_COLOR_GREEN, PINENTRY_COLOR_YELLOW,
  PINENTRY_COLOR_BLUE, PINENTRY_COLOR_MAGENTA,
  PINENTRY_COLOR_CYAN, PINENTRY_COLOR_WHITE
} pinentry_color_t;

struct pinentry
{
  /* The window title, or NULL.  */
  char *title;
  /* The description to display, or NULL.  */
  char *description;
  /* The error message to display, or NULL.  */
  char *error;
  /* The prompt to display, or NULL.  */
  char *prompt;
  /* The OK button text to display, or NULL.  */
  char *ok;
  /* The Not-OK button text to display, or NULL.  */
  char *notok;
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

  /* The number of seconds before giving up while waiting for user input. */
  int timeout;

#ifdef ENABLE_ENHANCED
  /* True if enhanced mode is requested.  */
  int enhanced;
#endif

  /* True if caller should grab the keyboard.  */
  int grab;
  /* The window ID of the parent window over which the pinentry window
     should be displayed.  */
  int parent_wid;

  /* The name of an optional file which will be touched after a curses
     entry has been displayed.  */
  char *touch_file;

  /* The user should set this to -1 if the user canceled the request,
     and to the length of the PIN stored in pin otherwise.  */
  int result;

  /* The user should set this if the NOTOK button was pressed.  */
  int canceled;

  /* The user should set this to true if an error with the local
     conversion occured. */
  int locale_err;

  /* The user should set this to true if the window close button has
     been used.  This flag is used in addition to a regular return
     value.  */
  int close_button;

  /* The caller should set this to true if only one button is
     required.  This is useful for notification dialogs where only a
     dismiss button is required. */
  int one_button;

  /* If this is not NULL, a passphrase quality indicator is shown.
     There will also be an inquiry back to the caller to get an
     indication of the quality for the passphrase entered so far.  The
     string is used as a label for the quality bar.  */
  char *quality_bar;

  /* The tooltip to be show for the qualitybar.  Malloced or NULL.  */
  char *quality_bar_tt;

  /* For the curses pinentry, the color of error messages.  */
  pinentry_color_t color_fg;
  int color_fg_bright;
  pinentry_color_t color_bg;
  pinentry_color_t color_so;
  int color_so_bright;

  /* Malloced and i18ned default strings or NULL.  These strings may
     include an underscore character to indicate an accelerator key.
     A double underscore represents a plain one.  */
  char *default_ok;
  char *default_cancel;
  char *default_prompt;

  /* For the quality indicator we need to do an inquiry.  Thus we need
     to save the assuan ctx.  */
  void *ctx_assuan;

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

/* The same as above but allows to specify the i/o descriptors. */
int pinentry_loop2 (int infd, int outfd);


/* Convert the UTF-8 encoded string TEXT to the encoding given in
   LC_CTYPE.  Return NULL on error. */
char *pinentry_utf8_to_local (char *lc_ctype, char *text);

/* Convert TEXT which is encoded according to LC_CTYPE to UTF-8.  With
   SECURE set to true, use secure memory for the returned buffer.
   Return NULL on error. */
char *pinentry_local_to_utf8 (char *lc_ctype, char *text, int secure);


/* Run a quality inquiry for PASSPHRASE of LENGTH. */
int pinentry_inq_quality (pinentry_t pin, 
                          const char *passphrase, size_t length);

/* Try to make room for at least LEN bytes for the pin in the pinentry
   PIN.  Returns new buffer on success and 0 on failure.  */
char *pinentry_setbufferlen (pinentry_t pin, int len);

/* Initialize the secure memory subsystem, drop privileges and
   return.  Must be called early.  */
void pinentry_init (const char *pgmname);

/* Return true if either DISPLAY is set or ARGV contains the string
   "--display". */
int pinentry_have_display (int argc, char **argv);

/* Parse the command line options.  Returns 1 if user should print
   version and exit.  Can exit the program if only help output is
   requested.  */
int pinentry_parse_opts (int argc, char *argv[]);


/* The caller must define this variable to process assuan commands.  */
extern pinentry_cmd_handler_t pinentry_cmd_handler;





#ifdef HAVE_W32_SYSTEM
/* Windows declares sleep as obsolete, but provides a definition for
   _sleep but non for the still existing sleep.  */
#define sleep(a) _sleep ((a))
#endif /*HAVE_W32_SYSTEM*/



#if 0 
{
#endif
#ifdef __cplusplus
}
#endif

#endif	/* PINENTRY_H */
