/* pinentrydialog.c - A W32 dialog for PIN entry.
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

#include <windows.h>
#include "resource.h"

#include "pinentrydialog.h"

BOOL CALLBACK
pinentry_dlg_proc (HWND dlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
    static struct w32_pinentry_s * ctx;

    switch (msg) {
    case WM_INITDIALOG:
	ctx = (struct w32_pinentry_s *)lparam;
	ctx->dlg = dlg;
	SetForegroundWindow (dlg);
	break;

    case WM_COMMAND:
	switch (LOWORD (wparam)) {
	case IDOK:
	    EndDialog (dlg, TRUE);
	    break;

	case IDCANCEL:
	    EndDialog (dlg, FALSE);
	    break;
	}
	break;
    }
    return FALSE;
}


struct w32_pinentry_s *
pinentry_new (HINSTANCE h)
{
    struct w32_pinentry_s *c;

    c = calloc (1, sizeof *c);
    if (!c)
	return NULL;
    DialogBoxParam (h, (LPCTSTR)IDD_PINENT, GetDesktopWindow (),
		    pinentry_dlg_proc, (LONG)c);
    return c;
}


int
pinentry_exec (struct w32_pinentry_s * c)
{
    ShowWindow (c->dlg, TRUE);
    return TRUE;
}


void
pinentry_set_description (struct w32_pinentry_s *ctx, const char * s)
{ 
    if (!s)
	SetDlgItemText (ctx->dlg, IDC_PINENT_DESC, "");
    else
	SetDlgItemText (ctx->dlg, IDC_PINENT_DESC, s);
    SetDlgItemText (ctx->dlg, IDC_PINENT_ERR, "");
}


const char *
pinentry_description (struct w32_pinentry_s *ctx)
{
    static char buf[256];
    GetDlgItemText (ctx->dlg, IDC_PINENT_DESC, buf, 256);
    return buf;
}


void
pinentry_set_error (struct w32_pinentry_s *ctx, const char * s)
{
    if (!s)
	SetDlgItemText (ctx->dlg, IDC_PINENT_ERR, "");
    else
	SetDlgItemText (ctx->dlg, IDC_PINENT_ERR, s);
}


const char *
pinentry_error (struct w32_pinentry_s * ctx)
{
    static char buf[256];
    GetDlgItemText (ctx->dlg, IDC_PINENT_ERR, buf, 256);
    return buf;
}


void
pinentry_set_text (struct w32_pinentry_s * ctx, const char * s)
{
    if (!s)
	SetDlgItemText (ctx->dlg, IDC_PINENT_TEXT, "");
    else
	SetDlgItemText (ctx->dlg, IDC_PINENT_TEXT, s);
}


const char *
pinentry_text (struct w32_pinentry_s * ctx)
{
    static char buf[256];
    GetDlgItemText (ctx->dlg, IDC_PINENT_TEXT, buf, 256);
    return buf;
}


void
pinentry_set_prompt (struct w32_pinentry_s * ctx, const char * s)
{
    if (!s)
	SetDlgItemText (ctx->dlg, IDC_PINENT_PROMPT, "");
    else
	SetDlgItemText (ctx->dlg, IDC_PINENT_PROMPT, s);
}



const char *
pinentry_prompt (struct w32_pinentry_s * ctx)
{
    static char buf[256];
    GetDlgItemText (ctx->dlg, IDC_PINENT_PROMPT, buf, 256);
    return buf;
}


void 
pinentry_set_ok_text (struct w32_pinentry_s * ctx, const char * s)
{
    if (!s)
	SetDlgItemText (ctx->dlg, IDOK, "OK");
    else
	SetDlgItemText (ctx->dlg, IDOK, s);
}

void 
pinentry_set_cancel_text (struct w32_pinentry_s * ctx, const char * s)
{
    if (!s)
	SetDlgItemText (ctx->dlg, IDCANCEL, "Cancel");
    else
	SetDlgItemText (ctx->dlg, IDCANCEL, s);
}
