/* controller.c - A W32 dialog for PIN entry.
   Copyright (C) 2004 g10 Code GmbH
   
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.
 
   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
 
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA  */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>

#include <windows.h>

#include "controller.h"
#include "dialog.h"


#define xtoi_1(p)   (*(p) <= '9'? (*(p)- '0'): \
                     *(p) <= 'F'? (*(p)-'A'+10):(*(p)-'a'+10))
#define xtoi_2(p)   ((xtoi_1(p) * 16) + xtoi_1((p)+1))

static void
strcpy_escaped (char *d, const char *s)
{
  while (*s)
    {
      if (*s == '%' && s[1] && s[2])
	{
	  s++;
	  *d++ = xtoi_2 (s);
	  s += 2;
	}
      else
	*d++ = *s++;
    }
  *d = 0;
}


int
pinentry_controller_new (struct pinentry_controller_s **ctx)
{
  struct pinentry_controller_s *c;
  int rc;
  int fds[2];

  c = calloc (1, sizeof *c);
  if (!c)
    exit (1);
  fds[0] = 0;
  fds[1] = 1;

  assuan_set_malloc_hooks (malloc, realloc, free);
  rc = assuan_init_pipe_server (&c->_ctx, fds);
  if (rc)
    exit (-1);
  rc = pinentry_ctl_registcmds (c);
  assuan_set_pointer (c->_ctx, c);
  *ctx = c;
  return 0;
}



void
pinentry_controller_free (struct pinentry_controller_s *c)
{
  if (c->_desc)
    free (c->_desc);
  if (c->_error)
    free (c->_error);
  if (c->_desc)
    free (c->_desc);
  if (c->_prompt)
    free (c->_prompt);
  if (c->_ok)
    free (c->_ok);
  if (c->_cancel)
    free (c->_cancel);
  assuan_deinit_server (c->_ctx);
}


void
pinentry_ctrl_exec (struct pinentry_controller_s *c)
{
  while (1)
    {
      assuan_error_t rc = assuan_accept (c->_ctx);
      if (rc == -1)
	{
	  fprintf (stderr, "Assuan terminated");
	  break;
	}
      else if (rc)
	{
	  fprintf (stderr, "Assuan accept problem: %s", assuan_strerror (rc));
	  break;
	}
      rc = assuan_process (c->_ctx);
      if (rc)
	{
	  fprintf (stderr, "Assuan processing failed: %s",
		   assuan_strerror (rc));
	  continue;
	}
    }
}


int
pinentry_ctl_desc (ASSUAN_CONTEXT ctx, char *line)
{
  struct pinentry_controller_s *c;
  char *newl;

  c = (struct pinentry_controller_s *) assuan_get_pointer (ctx);
  fprintf (stderr, "ctl_desc %s\n", line);

  newl = malloc (strlen (line) + 1);
  strcpy_escaped (newl, line);

  c->_desc = strdup (newl);	/*UTF8 */
  c->_error = NULL;

  free (newl);
  return 0;
}


int
pinentry_ctl_prompt (ASSUAN_CONTEXT ctx, char *line)
{
  struct pinentry_controller_s *c;
  char *newl;

  c = (struct pinentry_controller_s *) assuan_get_pointer (ctx);

  fprintf (stderr, "ctl_prompt %s\n", line);

  newl = malloc (strlen (line) + 1);
  strcpy_escaped (newl, line);

  c->_prompt = strdup (newl);	/*UTF8 */
  c->_error = NULL;

  free (newl);
  return 0;
}


int
pinentry_ctl_error (ASSUAN_CONTEXT ctx, char *line)
{
  struct pinentry_controller_s *c;
  char *newl;

  c = (struct pinentry_controller_s *) assuan_get_pointer (ctx);

  fprintf (stderr, "error %s", line);

  newl = malloc (strlen (line) + 1);
  strcpy_escaped (newl, line);

  c->_error = strdup (newl);	/* UTF8 */

  free (newl);
  return 0;
}

int
pinentry_ctl_ok (ASSUAN_CONTEXT ctx, char *line)
{
  struct pinentry_controller_s *c;
  char *newl;

  c = (struct pinentry_controller_s *) assuan_get_pointer (ctx);

  fprintf (stderr, "OK %s\n", line);

  newl = malloc (strlen (line) + 1);
  strcpy_escaped (newl, line);

  c->_ok = strdup (line);

  free (newl);
  return 0;
}

int
pinentry_ctl_cancel (ASSUAN_CONTEXT ctx, char *line)
{
  struct pinentry_controller_s *c;
  char *newl;

  c = (struct pinentry_controller_s *) assuan_get_pointer (ctx);

  fprintf (stderr, "cancel %s\n", line);

  newl = malloc (strlen (line) + 1);
  strcpy_escaped (newl, line);

  c->_cancel = strdup (line);

  free (newl);
  return 0;
}


int
_pinentry_ctl_getpin (struct pinentry_controller_s *c, char *line)
{
  int ret;
  FILE *fp;

  if (c->_pinentry == NULL)
    c->_pinentry = pinentry_new (NULL);

  pinentry_set_prompt (c->_pinentry, c->_prompt);
  pinentry_set_description (c->_pinentry, c->_desc);
  pinentry_set_text (c->_pinentry, NULL);
  if (c->_ok)
    pinentry_set_ok_text (c->_pinentry, c->_ok);
  if (c->_cancel)
    pinentry_set_cancel_text (c->_pinentry, c->_cancel);
  if (c->_error)
    pinentry_set_error (c->_pinentry, c->_error);

  ret = pinentry_exec (c->_pinentry);
  fp = NULL; /* We can't use that: assuan_get_data_fp (c->_ctx); */
  if (ret)
    {
      char *p;
      p = (char *) pinentry_text (c->_pinentry);
      fputs (p /*UTF8 */ , fp);
      return 0;
    }
  else
    {
      assuan_set_error (c->_ctx, ASSUAN_Canceled, "Dialog cancelled by user");
      return ASSUAN_Canceled;
    }
}


int
pinentry_ctl_getpin (ASSUAN_CONTEXT ctx, char *line)
{
  struct pinentry_controller_s *c;

  c = (struct pinentry_controller_s *) assuan_get_pointer (ctx);

  fprintf (stderr, "getpin %s\n", line);
  return _pinentry_ctl_getpin (c, line);
}


int
_pinentry_ctl_confirm (struct pinentry_controller_s *c, char *line)
{
  int ret;
  FILE *fp;

  if (c->_error)
    ret = MessageBox (NULL, c->_error, "Error", MB_YESNO | MB_ICONERROR);
  else
    ret =
      MessageBox (NULL, c->_desc, "Information",
		  MB_YESNO | MB_ICONINFORMATION);
  fp = NULL; /* We can't use that: assuan_get_data_fp (c->_ctx); */
  if (ret == IDYES)
    {
      //fputs( "YES", fp );    
      return ASSUAN_No_Error;
    }
  else
    {
      //fputs( "NO", fp );
      return ASSUAN_Not_Confirmed;
    }
}


int
pinentry_ctl_confirm (ASSUAN_CONTEXT ctx, char *line)
{
  struct pinentry_controller_s *c;

  c = (struct pinentry_controller_s *) assuan_get_pointer (ctx);

  fprintf (stderr, "confirm %s\n", line);

  return _pinentry_ctl_confirm (c, line);
}



int
pinentry_ctl_opthandler (ASSUAN_CONTEXT ctx, const char *key,
			 const char *value)
{
  struct pinentry_controller_s *c;

  c = (struct pinentry_controller_s *) assuan_get_pointer (ctx);

  /* FIXME: For now we simply ignore all options.  This module should
     be converted to make use of the pinentry framework. */
  return 0;
}

int
pinentry_ctl_registcmds (struct pinentry_controller_s *c)
{
  static struct
  {
    const char *name;
    int (*handler) (ASSUAN_CONTEXT, char *line);
  } table[] =
  {
    {
    "SETDESC", pinentry_ctl_desc},
    {
    "SETPROMPT", pinentry_ctl_prompt},
    {
    "SETERROR", pinentry_ctl_error},
    {
    "SETOK", pinentry_ctl_ok},
    {"SETCANCEL", pinentry_ctl_cancel},
    {
    "GETPIN", pinentry_ctl_getpin},
    {
    "CONFIRM", pinentry_ctl_confirm},
    {
    0, 0}
  };
  int i, rc;

  for (i = 0; table[i].name; i++)
    {
      rc = assuan_register_command (c->_ctx, 0, table[i].name, table[i].handler);
      if (rc)
	return rc;
    }
  assuan_register_option_handler (c->_ctx, pinentry_ctl_opthandler);
  return 0;
}
