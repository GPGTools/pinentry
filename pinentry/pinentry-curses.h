/* pinentry-curses.h - A secure curses dialog for PIN entry, library version
   Copyright (C) 2002 g10 Code GmbH
   
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
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA  */

#ifndef PINENTRY_CURSES_H
#define PINENTRY_CURSES_H

#include "pinentry.h"

#ifdef __cplusplus
extern "C" {
#endif

int curses_cmd_handler (pinentry_t pinentry);

#ifdef __cplusplus
}
#endif

#endif	/* PINENTRY_CURSES_H */
