/* dialog.h - A W32 dialog for PIN entry.
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

#ifndef W32_DIALOG_H
#define W32_DIALOG_H

struct w32_pinentry_s
{
  HWND dlg;
};


BOOL CALLBACK pinentry_dlg_proc (HWND dlg, UINT msg, WPARAM wparam,
				 LPARAM lparam);
struct w32_pinentry_s *pinentry_new (HINSTANCE h);
int pinentry_exec (struct w32_pinentry_s *c);
void pinentry_set_description (struct w32_pinentry_s *ctx, const char *s);
const char *pinentry_description (struct w32_pinentry_s *ctx);
void pinentry_set_error (struct w32_pinentry_s *ctx, const char *s);
const char *pinentry_error (struct w32_pinentry_s *ctx);
void pinentry_set_text (struct w32_pinentry_s *ctx, const char *s);
const char *pinentry_text (struct w32_pinentry_s *ctx);
void pinentry_set_prompt (struct w32_pinentry_s *ctx, const char *s);
const char *pinentry_prompt (struct w32_pinentry_s *ctx);
void pinentry_set_ok_text (struct w32_pinentry_s *ctx, const char *s);
void pinentry_set_cancel_text (struct w32_pinentry_s *ctx, const char *s);

#endif /*W32_DIALOG_H*/
