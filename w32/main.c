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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <windows.h>

#include "pinentrydialog.h"
#include "pinentrycontroller.h"


int WINAPI
WinMain (HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmdLine, int nShowCmd)
{
    struct pinentry_controller_s ctl;

    pinentry_ctrl_exec (&ctl);
    pinentry_controller_free (&ctl);
    return TRUE;
}
