/* cppmemory.cpp - Memory management for a secure KDE dialog for PIN entry.
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

#include <new>
#include <stdlib.h>
extern "C"
{
#include "memory.h"
}

bool is_secure = false;

void *operator new (size_t s) throw (std::bad_alloc)
{
  if( s == 0 ) s = 1; // never allocate 0 bytes!

  while( true ) { // the standard requires us to keep trying
    void* p;
    if( is_secure ) p = ::secmem_malloc( s );
    else p = ::malloc( s );
    if(p) return p; // succes!
    
    std::new_handler h = std::set_new_handler(0);
    std::set_new_handler(h);
    if( h ) (*h)();
    else throw std::bad_alloc();
  }
}

void *operator new[] (size_t s) throw (std::bad_alloc)
{
  return operator new( s );
}

void operator delete (void* p) throw() 
{  
  if( !p ) return;
  if( ::m_is_secure(p) ) ::secmem_free( p );
  else ::free( p );
}

void operator delete[] (void* p) throw()
{
  operator delete( p );
}
