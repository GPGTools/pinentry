/* pinentry-gnome3.c
   Copyright (C) 2015 g10 Code GmbH

   pinentry-gnome-3 is a pinentry application for GNOME 3.  It tries
   to follow the Gnome Human Interface Guide as close as possible.

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
# include "config.h"
#endif

#include <gtk/gtk.h>
#include <gcr/gcr-base.h>

#include <string.h>

#include "assuan.h"

#include "memory.h"

#include "pinentry.h"

#ifdef FALLBACK_CURSES
#include "pinentry-curses.h"
#endif


#define PGMNAME "pinentry-gnome3"

#ifndef VERSION
#  define VERSION
#endif

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

static GcrPrompt *
create_prompt (pinentry_t pe, int confirm)
{
  GcrPrompt *prompt;
  GError *error = NULL;
  char *msg;

  /* Create the prompt.  */
  prompt = GCR_PROMPT (gcr_system_prompt_open (-1, NULL, &error));
  if (! prompt)
    {
      g_warning ("couldn't create prompt for gnupg passphrase: %s",
		 error->message);
      g_error_free (error);
      return NULL;
    }

  /* Set the messages for the various buttons, etc.  */
  if (pe->title)
    {
      msg = pinentry_utf8_validate (pe->title);
      gcr_prompt_set_title (prompt, msg);
      g_free (msg);
    }

  if (pe->description)
    {
      msg = pinentry_utf8_validate (pe->description);
      gcr_prompt_set_description (prompt, msg);
      g_free (msg);
    }

  /* An error occured during the last prompt.  */
  if (pe->error)
    {
      msg = pinentry_utf8_validate (pe->error);
      gcr_prompt_set_warning (prompt, msg);
      g_free (msg);
    }

  if (! pe->prompt && confirm)
    gcr_prompt_set_message (prompt, "Message");
  else if (! pe->prompt && ! confirm)
    gcr_prompt_set_message (prompt, "Enter Passphrase");
  else
    {
      msg = pinentry_utf8_validate (pe->prompt);
      gcr_prompt_set_message (prompt, msg);
      g_free (msg);
    }

  if (! confirm)
    gcr_prompt_set_password_new (prompt, !!pe->repeat_passphrase);

  if (pe->ok || pe->default_ok)
    {
      msg = pinentry_utf8_validate (pe->ok ?: pe->default_ok);
      gcr_prompt_set_continue_label (prompt, msg);
      g_free (msg);
    }
  /* XXX: Disable this button if pe->one_button is set.  */
  if (pe->cancel || pe->default_cancel)
    {
      msg = pinentry_utf8_validate (pe->cancel ?: pe->default_cancel);
      gcr_prompt_set_cancel_label (prompt, msg);
      g_free (msg);
    }

  if (confirm && pe->notok)
    {
      /* XXX: Add support for the third option.  */
    }

  /* XXX: gcr expects a string; we have a int.  */
  // gcr_prompt_set_caller_window (prompt, pe->parent_wid);

  if (! confirm && pe->allow_external_password_cache && pe->keyinfo)
    {
      if (pe->default_pwmngr)
	{
	  msg = pinentry_utf8_validate (pe->default_pwmngr);
	  gcr_prompt_set_choice_label (prompt, msg);
	  g_free (msg);
	}
      else
	gcr_prompt_set_choice_label
	  (prompt, "Automatically unlock this key, whenever I'm logged in");
    }

  return prompt;
}

static int
gnome3_cmd_handler (pinentry_t pe)
{
  GcrPrompt *prompt = NULL;
  GError *error = NULL;
  int ret = -1;

  if (pe->pin)
    /* Passphrase mode.  */
    {
      const char *password;

      prompt = create_prompt (pe, 0);
      if (! prompt)
	/* Something went wrong.  */
	{
	  pe->canceled = 1;
	  return -1;
	}

      /* "The returned password is valid until the next time a method
	 is called to display another prompt."  */
      password = gcr_prompt_password_run (prompt, NULL, &error);
      if (error)
	/* Error.  */
	{
	  pe->specific_err = ASSUAN_General_Error;
	  g_error_free (error);
	  ret = -1;
	}
      else if (! password && ! error)
	/* User cancelled the operation.  */
	ret = -1;
      else
	{
	  pinentry_setbufferlen (pe, strlen (password) + 1);
	  if (pe->pin)
	    strcpy (pe->pin, password);

	  if (pe->repeat_passphrase)
	    pe->repeat_okay = 1;

	  ret = 1;
	}
    }
  else
    /* Message box mode.  */
    {
      GcrPromptReply reply;

      prompt = create_prompt (pe, 1);
      if (! prompt)
	/* Something went wrong.  */
	{
	  pe->canceled = 1;
	  return -1;
	}

      /* XXX: We don't support a third button!  */

      reply = gcr_prompt_confirm_run (prompt, NULL, &error);
      if (error)
	{
	  pe->specific_err = ASSUAN_General_Error;
	  ret = 0;
	}
      else if (reply == GCR_PROMPT_REPLY_CONTINUE
	       /* XXX: Hack since gcr doesn't yet support one button
		  message boxes treat cancel the same as okay.  */
	       || pe->one_button)
	/* Confirmation.  */
	ret = 1;
      else
	/* GCR_PROMPT_REPLY_CANCEL */
	{
	  pe->canceled = 1;
	  ret = 0;
	}
    }

  if (prompt)
    g_clear_object (&prompt);
  return ret;
}

pinentry_cmd_handler_t pinentry_cmd_handler = gnome3_cmd_handler;

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

  pinentry_parse_opts (argc, argv);

  if (pinentry_loop ())
    return 1;

  return 0;
}
