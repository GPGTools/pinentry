/* main.c - Secure W32 dialog for PIN entry.
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

#include "pinentry.h"
#include "memory.h"

#include "resource.h"


#define PGMNAME "pinentry-w32"


static int w32_cmd_handler (pinentry_t pe);
static void ok_button_clicked (HWND dlg, pinentry_t pe);


/* We use gloabl variables for the state, becuase there should never
   ever be a second instance.  */
static HWND dialog_handle;
static int passphrase_ok = 0;


/* Connect this module to the pinnetry framework.  */
pinentry_cmd_handler_t pinentry_cmd_handler = w32_cmd_handler;



/* Dialog processing loop.  */
static BOOL CALLBACK
dlg_proc (HWND dlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
  static pinentry_t pe;

  switch (msg)
    {
    case WM_INITDIALOG:
      pe = (pinentry_t)lparam;
      if (!pe)
        abort ();
      SetDlgItemText (dlg, IDC_PINENT_PROMPT, pe->prompt);
      SetDlgItemText (dlg, IDC_PINENT_DESC, pe->description);
      SetDlgItemText (dlg, IDC_PINENT_TEXT, "");
      if (pe->ok)
        SetDlgItemText (dlg, IDOK, pe->ok);
      if (pe->cancel)
        SetDlgItemText (dlg, IDCANCEL, pe->cancel);
      if (pe->error)
        SetDlgItemText (dlg, IDC_PINENT_ERR, pe->error);
      dialog_handle = dlg;
      SetForegroundWindow (dlg);
      break;

    case WM_COMMAND:
      switch (LOWORD (wparam))
	{
	case IDOK:
          ok_button_clicked (dlg, pe);
	  EndDialog (dlg, TRUE);
	  break;

	case IDCANCEL:
          pe->result = -1;
	  EndDialog (dlg, FALSE);
	  break;
	}
      break;
    }
  return FALSE;
}


/* The okay button has been clicked or the enter enter key in the text
   field.  */
static void
ok_button_clicked (HWND dlg, pinentry_t pe)
{
  char *s_utf8;
  char *s_buffer;
  size_t s_buffer_size = 256;
  
  pe->locale_err = 1;
  s_buffer = secmem_malloc (s_buffer_size + 1);
  if (!s_buffer)
    return;

  pe->result = GetDlgItemText (dlg, IDC_PINENT_TEXT, s_buffer, s_buffer_size);
/*   s_utf8 = pinentry_local_to_utf8 (pe->lc_ctype, s_buffer, 1); */
/*   secmem_free (s_buffer); */
  s_utf8 = s_buffer;   /* FIXME */
  if (s_utf8)
    {
      passphrase_ok = 1;
      pinentry_setbufferlen (pe, strlen (s_utf8) + 1);
      if (pe->pin)
        strcpy (pe->pin, s_utf8);
/*       secmem_free (s_utf8); */
      pe->locale_err = 0;
      pe->result = strlen (pe->pin);
    }
}


static int
w32_cmd_handler (pinentry_t pe)
{
  int want_pass = !!pe->pin;

  passphrase_ok = 0;

  if (want_pass)
    {
      DialogBoxParam (NULL, (LPCTSTR) IDD_PINENT,
                      GetDesktopWindow (), dlg_proc, (LPARAM)pe);
      ShowWindow (dialog_handle, SW_SHOWNORMAL);
      return pe->result;
    }
  else /* Confirmation mode.  */
    {
      int ret;

      if (pe->error)
        ret = MessageBox (NULL, pe->error, "Error", MB_YESNO | MB_ICONERROR);
      else
        ret = MessageBox (NULL, pe->description?pe->description:"",
                          "Information", MB_YESNO | MB_ICONINFORMATION);
      if (ret == IDYES)
        return 1;
      else
        return 0;
    }
}


int
main (int argc, char **argv)
{
  pinentry_init (PGMNAME);

  /* Consumes all arguments.  */
  if (pinentry_parse_opts (argc, argv))
    {
      printf ("pinentry-w32 (pinentry) " VERSION "\n");
      exit (EXIT_SUCCESS);
    }

  if (pinentry_loop ())
    return 1;

  return 0;
}
