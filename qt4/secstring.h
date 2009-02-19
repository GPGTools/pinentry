/* 
   secstring.h - typedefs and conversion for secmem-backed std::strings.

   Copyright (C) 2008 Klarälvdalens Datakonsult AB (KDAB)

   Written by Marc Mutz <marc@kdab.com>.
   
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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef __PINENTRY_SECSTRING_H__
#define __PINENTRY_SECSTRING_H__

#include <secmem/secmem++.h>
#include <string>

#include <QChar>
#include <QMetaType>

typedef std::basic_string<  char, std::char_traits< char>, secmem::alloc< char> > secstring;
typedef std::basic_string< QChar, std::char_traits<QChar>, secmem::alloc<QChar> > secqstring;

secstring toUtf8( const secqstring & str );

Q_DECLARE_METATYPE( secqstring )
Q_DECLARE_METATYPE( secstring )

#endif // __PINENTRY_SECSTRING_H__
