/* pinentrycontroller.h - A secure KDE dialog for PIN entry.
   Copyright (C) 2002 Klarälvdalens Datakonsult AB
   Written by Steffen Hansen <steffen@klaralvdalens-datakonsult.se>.
   
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

#ifndef __PINENTRYCONTROLLER_H__
#define __PINENTRYCONTROLLER_H__


#include <assuan.h>


struct pinentry_controller_s {
    ASSUAN_CONTEXT _ctx;
    char * _desc;
    char * _error;
    char * _prompt;
    char * _ok;
    char * _cancel;
    struct w32_pinentry_s *_pinentry;
};


int  pinentry_ctl_registcmds (struct pinentry_controller_s * c);
void pinentry_controller_free (struct pinentry_controller_s * c);
void pinentry_ctrl_exec (struct pinentry_controller_s * c);

#endif // __PINENTRYCONTROLLER_H__
