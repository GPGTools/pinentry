/* main.c - Secure W32 dialog for PIN entry.
 * Copyright (C) 2004, 2007 g10 Code GmbH
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#if WINVER < 0x0403
# define WINVER 0x0403  /* Required for SendInput.  */
#endif
#include <windows.h>

#include "pinentry.h"

#include "resource.h"
/* #include "msgcodes.h" */

#define PGMNAME "pinentry-w32"

#ifndef debugfp
#define debugfp stderr
#endif


static int w32_cmd_handler (pinentry_t pe);
static void ok_button_clicked (HWND dlg, pinentry_t pe);


/* We use global variables for the state, because there should never
   ever be a second instance.  */
static HWND dialog_handle;
static int confirm_mode;
static int passphrase_ok;
static int confirm_yes;

/* The file descriptors for the loop.  */
static int w32_infd;
static int w32_outfd;


/* Connect this module to the pinentry framework.  */
pinentry_cmd_handler_t pinentry_cmd_handler = w32_cmd_handler;



const char *
w32_strerror (int ec)
{
  static char strerr[256];

  if (ec == -1)
    ec = (int)GetLastError ();
  FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM, NULL, ec,
                 MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
                 strerr, sizeof strerr - 1, NULL);
  return strerr;
}



/* static HWND */
/* show_window_hierarchy (HWND parent, int level) */
/* { */
/*   HWND child; */

/*   child = GetWindow (parent, GW_CHILD); */
/*   while (child) */
/*     { */
/*       char buf[1024+1]; */
/*       char name[200]; */
/*       int nname; */
/*       char *pname; */

/*       memset (buf, 0, sizeof (buf)); */
/*       GetWindowText (child, buf, sizeof (buf)-1); */
/*       nname = GetClassName (child, name, sizeof (name)-1); */
/*       if (nname) */
/*         pname = name; */
/*       else */
/*         pname = NULL; */
/*       fprintf (debugfp, "### %*shwnd=%p (%s) `%s'\n", level*2, "", child, */
/*                pname? pname:"", buf); */
/*       show_window_hierarchy (child, level+1); */
/*       child = GetNextWindow (child, GW_HWNDNEXT);	 */
/*     } */

/*   return NULL; */
/* } */



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

  (void)string;

  /* FIXME: Need to figure out how to convert dialog coordinates to
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
    SetDlgItemTextW (dlg, item, L"");
  else
    {
      wchar_t *wbuf;

      wbuf = utf8_to_wchar (string);
      if (!wbuf)
        SetDlgItemTextW (dlg, item, L"[out of core]");
      else
        {
          SetDlgItemTextW (dlg, item, wbuf);
          free (wbuf);
        }
    }
}


/* Load our butmapped icon from the resource and display it.  */
static void
set_bitmap (HWND dlg, int item)
{
  HWND hwnd;
  HBITMAP bitmap;
  RECT rect;
  int resid;

  hwnd = GetDlgItem (dlg, item);
  if (!hwnd)
    return;

  rect.left = 0;
  rect.top = 0;
  rect.right = 32;
  rect.bottom = 32;
  if (!MapDialogRect (dlg, &rect))
    {
      fprintf (stderr, "MapDialogRect failed: %s\n",  w32_strerror (-1));
      return;
    }
  /* fprintf (stderr, "MapDialogRect: %d/%d\n", rect.right, rect.bottom); */

  switch (rect.right)
    {
    case 32: resid = IDB_ICON_32; break;
    case 48: resid = IDB_ICON_48; break;
    case 64: resid = IDB_ICON_64; break;
    case 96: resid = IDB_ICON_96; break;
    default: resid = IDB_ICON_128;break;
    }

  bitmap = LoadImage (GetModuleHandle (NULL),
                      MAKEINTRESOURCE (resid),
                      IMAGE_BITMAP,
                      rect.right, rect.bottom,
                      (LR_SHARED | LR_LOADTRANSPARENT | LR_LOADMAP3DCOLORS));
  if (!bitmap)
    {
      fprintf (stderr, "LoadImage failed: %s\n",  w32_strerror (-1));
      return;
    }
  SendMessage(hwnd, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)bitmap);
}


/* Dialog processing loop.  */
static INT_PTR CALLBACK
dlg_proc (HWND dlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
  static pinentry_t pe;
  /* static int item; */


/*   { */
/*     int idx; */

/*     for (idx=0; msgcodes[idx].string; idx++) */
/*       if (msg == msgcodes[idx].msg) */
/*         break; */
/*     if (msgcodes[idx].string) */
/*       fprintf (debugfp, "received %s\n", msgcodes[idx].string); */
/*     else */
/*       fprintf (debugfp, "received WM_%u\n", msg); */
/*   } */

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
      set_bitmap (dlg, IDC_PINENT_ICON);
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

      if (confirm_mode)
        {
          EnableWindow (GetDlgItem (dlg, IDC_PINENT_TEXT), FALSE);
          SetWindowPos (GetDlgItem (dlg, IDC_PINENT_TEXT), NULL, 0, 0, 0, 0,
                        (SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_HIDEWINDOW));

          /* item = IDOK; */
        }
      /* else */
      /*   item = IDC_PINENT_TEXT; */

      center_window (dlg, HWND_TOP);

      ShowWindow (dlg, SW_SHOW);

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

     case WM_KEYDOWN:
       if (wparam == VK_RETURN)
         {
           if (confirm_mode)
             confirm_yes = 1;
           else
             ok_button_clicked (dlg, pe);
           EndDialog (dlg, TRUE);
         }
       break;

    case WM_CTLCOLORSTATIC:
      if ((HWND)lparam == GetDlgItem (dlg, IDC_PINENT_ERR))
        {
          /* Display the error prompt in red.  */
          SetTextColor ((HDC)wparam, RGB (255, 0, 0));
          SetBkMode ((HDC)wparam, TRANSPARENT);
          return (BOOL)GetStockObject (NULL_BRUSH);
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
  confirm_mode = !pe->pin;

  passphrase_ok = confirm_yes = 0;

  dialog_handle = NULL;
  DialogBoxParam (GetModuleHandle (NULL), MAKEINTRESOURCE (IDD_PINENT),
                  GetDesktopWindow (), dlg_proc, (LPARAM)pe);
  if (!dialog_handle)
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
  w32_infd = STDIN_FILENO;
  w32_outfd = STDOUT_FILENO;

  pinentry_init (PGMNAME);

  pinentry_parse_opts (argc, argv);

/*   debugfp = fopen ("pinentry.log", "w"); */
/*   if (!debugfp) */
/*     debugfp = stderr; */


  if (pinentry_loop2 (w32_infd, w32_outfd))
    return 1;

  return 0;
}
