#include <new>
#include <stdlib.h>
#include "memory.h"

bool is_secure = false;

void *operator new (size_t s) throw (std::bad_alloc)
{
  if( s == 0 ) s = 1; // never allocate 0 bytes!

  while( true ) { // the standard requires us to keep trying
    void* p;
    if( is_secure ) p = ::secmem_malloc( s );
    else p = ::malloc( s );
    if(p) return p; // succes!
    
    new_handler h = set_new_handler(0);
    set_new_handler(h);
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
