/* main.c - Secure W32 dialog for PIN entry.
   Copyright (C) 2004, 2007 g10 Code GmbH
   
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

#ifndef LSFW_LOCK
# define LSFW_LOCK 1
# define LSFW_UNLOCK 2
#endif


/* This function pointer gets initialized in main.  */
static WINUSERAPI BOOL WINAPI (*lock_set_foreground_window)(UINT);


static int w32_cmd_handler (pinentry_t pe);
static void ok_button_clicked (HWND dlg, pinentry_t pe);


/* We use gloabl variables for the state, because there should never
   ever be a second instance.  */
static HWND dialog_handle;
static int confirm_mode;
static int passphrase_ok;
static int confirm_yes;

/* Connect this module to the pinnetry framework.  */
pinentry_cmd_handler_t pinentry_cmd_handler = w32_cmd_handler;





/* Convert a wchar to UTF8.  Caller needs to release the string.
   Returns NULL on error. */
static char *
wchar_to_utf8 (const wchar_t *string, size_t len, int secure)
{
  int n;
  char *result;

  /* Note, that CP_UTF8 is not defined in Windows versions earlier
     than NT.  */
  n = WideCharToMultiByte (CP_UTF8, 0, string, len, NULL, 0, NULL, NULL);
  if (n < 0)
    return NULL;

  result = secure? secmem_malloc (n+1) : malloc (n+1);
  if (!result)
    return NULL;
  n = WideCharToMultiByte (CP_UTF8, 0, string, len, result, n, NULL, NULL);
  if (n < 0)
    {
      if (secure)
        secmem_free (result);
      else
        free (result);
      return NULL;
    }
  return result;
}


/* Convert a UTF8 string to wchar.  Returns NULL on error. Caller
   needs to free the returned value.  */
wchar_t *
utf8_to_wchar (const char *string)
{
  int n;
  wchar_t *result;
  size_t len = strlen (string);

  n = MultiByteToWideChar (CP_UTF8, 0, string, len, NULL, 0);
  if (n < 0)
    return NULL;

  result = calloc ((n+1), sizeof *result);
  if (!result)
    return NULL;
  n = MultiByteToWideChar (CP_UTF8, 0, string, len, result, n);
  if (n < 0)
    {
      free (result);
      return NULL;
    }
  result[n] = 0;
  return result;
}


/* Center the window CHILDWND with the desktop as its parent
   window.  STYLE is passed as second arg to SetWindowPos.*/
static void
center_window (HWND childwnd, HWND style) 
{     
  HWND parwnd;
  RECT rchild, rparent;    
  HDC hdc;
  int wchild, hchild, wparent, hparent;
  int wscreen, hscreen, xnew, ynew;
  int flags = SWP_NOSIZE | SWP_NOZORDER;
  
  parwnd = GetDesktopWindow ();
  GetWindowRect (childwnd, &rchild);     
  wchild = rchild.right - rchild.left;     
  hchild = rchild.bottom - rchild.top;
  
  GetWindowRect (parwnd, &rparent);     
  wparent = rparent.right - rparent.left;     
  hparent = rparent.bottom - rparent.top;      
  
  hdc = GetDC (childwnd);     
  wscreen = GetDeviceCaps (hdc, HORZRES);     
  hscreen = GetDeviceCaps (hdc, VERTRES);     
  ReleaseDC (childwnd, hdc);      
  xnew = rparent.left + ((wparent - wchild) / 2);     
  if (xnew < 0)
    xnew = 0;
  else if ((xnew+wchild) > wscreen) 
    xnew = wscreen - wchild;
  ynew = rparent.top  + ((hparent - hchild) / 2);
  if (ynew < 0)
    ynew = 0;
  else if ((ynew+hchild) > hscreen)
    ynew = hscreen - hchild;
  if (style == HWND_TOPMOST || style == HWND_NOTOPMOST)
    flags = SWP_NOMOVE | SWP_NOSIZE;
  SetWindowPos (childwnd, style? style : NULL, xnew, ynew, 0, 0, flags);
}


/* Resize the button so that STRING fits into it.   */
static void
resize_button (HWND hwnd, const char *string)
{
  if (!hwnd)
    return;

  /* FIXME: Need to figure out how to convert dialog coorddnates to
     screen coordinates and how buttons should be placed.  */
/*   SetWindowPos (hbutton, NULL, */
/*                 10, 180,  */
/*                 strlen (string+2), 14, */
/*                 (SWP_NOZORDER)); */
}




/* Call SetDlgItemTextW with an UTF8 string.  */
static void
set_dlg_item_text (HWND dlg, int item, const char *string)
{
  if (!string || !*string)
    SetDlgItemText (dlg, item, "");
  else
    {
      wchar_t *wbuf;
      
      wbuf = utf8_to_wchar (string);
      if (!wbuf)
        SetDlgItemText (dlg, item, "[out of core]");
      else
        {
          SetDlgItemTextW (dlg, item, wbuf);
          free (wbuf);
        }
    }
}


/* Dialog processing loop.  */
static BOOL CALLBACK
dlg_proc (HWND dlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
  static pinentry_t pe;
  static int item;

  switch (msg)
    {
    case WM_INITDIALOG:
      dialog_handle = dlg;
      pe = (pinentry_t)lparam;
      if (!pe)
        abort ();
      set_dlg_item_text (dlg, IDC_PINENT_PROMPT, pe->prompt);
      set_dlg_item_text (dlg, IDC_PINENT_DESC, pe->description);
      set_dlg_item_text (dlg, IDC_PINENT_TEXT, "");
      if (pe->ok)
        {
          set_dlg_item_text (dlg, IDOK, pe->ok);
          resize_button (GetDlgItem (dlg, IDOK), pe->ok);
        }
      if (pe->cancel)
        {
          set_dlg_item_text (dlg, IDCANCEL, pe->cancel);
          resize_button (GetDlgItem (dlg, IDCANCEL), pe->cancel);
        }
      if (pe->error)
        set_dlg_item_text (dlg, IDC_PINENT_ERR, pe->error);

      center_window (dlg, HWND_TOP);

      if (confirm_mode)
        {
          EnableWindow (GetDlgItem (dlg, IDC_PINENT_TEXT), FALSE);
          SetWindowPos (GetDlgItem (dlg, IDC_PINENT_TEXT), NULL, 0, 0, 0, 0,
                        (SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_HIDEWINDOW));
          
          item = IDOK;
        }
      else
        item = IDC_PINENT_TEXT;

      if (GetDlgCtrlID ((HWND)wparam) != item) 
        SetFocus ( GetDlgItem (dlg, item)); 
      
      /* Fixme: There are two problems: A race condition between the
         two calls and more important that SetForegroundWindow will
         fail if a Menu is somewhere open.  */
      if (SetForegroundWindow (dlg) && lock_set_foreground_window)
        lock_set_foreground_window (LSFW_LOCK);
      break;

    case WM_COMMAND:
      switch (LOWORD (wparam))
	{
	case IDOK:
          if (confirm_mode)
            confirm_yes = 1;
          else
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
  wchar_t *w_buffer;
  size_t w_buffer_size = 255;
  unsigned int nchar;
  
  pe->locale_err = 1;
  w_buffer = secmem_malloc ((w_buffer_size + 1) * sizeof *w_buffer);
  if (!w_buffer)
    return;

  nchar = GetDlgItemTextW (dlg, IDC_PINENT_TEXT, w_buffer, w_buffer_size);
  s_utf8 = wchar_to_utf8 (w_buffer, nchar, 1);
  secmem_free (w_buffer);
  if (s_utf8)
    {
      passphrase_ok = 1;
      pinentry_setbufferlen (pe, strlen (s_utf8) + 1);
      if (pe->pin)
        strcpy (pe->pin, s_utf8);
      secmem_free (s_utf8);
      pe->locale_err = 0;
      pe->result = pe->pin? strlen (pe->pin) : 0;
    }
}


static int
w32_cmd_handler (pinentry_t pe)
{
  HWND lastwindow = GetForegroundWindow ();

  confirm_mode = !pe->pin;

  passphrase_ok = confirm_yes = 0;

  dialog_handle = NULL;
  DialogBoxParam (NULL, (LPCTSTR) IDD_PINENT,
                  GetDesktopWindow (), dlg_proc, (LPARAM)pe);
  if (dialog_handle)
    {
      ShowWindow (dialog_handle, SW_SHOWNORMAL);
      if (lock_set_foreground_window)
        lock_set_foreground_window (LSFW_UNLOCK);
      if (lastwindow)
        SetForegroundWindow (lastwindow);
    }
  else
    return -1;

  if (confirm_mode)
    return confirm_yes;
  else if (passphrase_ok && pe->pin)
    return strlen (pe->pin);
  else
    return -1;
}


int
main (int argc, char **argv)
{
  void *handle;

  pinentry_init (PGMNAME);

  /* Consumes all arguments.  */
  if (pinentry_parse_opts (argc, argv))
    {
      printf ("pinentry-w32 (pinentry) " VERSION "\n");
      exit (EXIT_SUCCESS);
    }

  /* We need to load a functuion because that one is only available
     since W2000 but not in older NTs.  */
  handle = LoadLibrary ("user32.dll");
  if (handle)
    {
      void *foo;
      foo = GetProcAddress (handle, "LockSetForegroundWindow");
      if (foo)
        lock_set_foreground_window = foo;
      else
        CloseHandle (handle);
    }

  if (pinentry_loop ())
    return 1;

  return 0;
}
