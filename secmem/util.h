/* Quintuple Agent utilities
 * Copyright (C) 1999 Robert Bihlmeyer <robbe@orcus.priv.at>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _UTIL_H
#define _UTIL_H

#include <sys/types.h>

#ifndef HAVE_BYTE_TYPEDEF
# undef byte	   
# ifdef __riscos__
    /* Norcroft treats char == unsigned char but char* != unsigned char*  */
    typedef char byte;
# else 
    typedef unsigned char byte;
# endif 
# define HAVE_BYTE_TYPEDEF
#endif

#ifndef HAVE_ULONG_TYPEDEF
# undef ulong	   
  typedef unsigned long ulong;
# define HAVE_ULONG_TYPEDEF
#endif


ssize_t xwrite(int, const void *, size_t); /* write until finished */
int debugmsg(const char *, ...); /* output a debug message if debugging==on */
void wipe(void *, size_t);	/* wipe a block of memory */
void lower_privs();		/* lower privileges */
void raise_privs();		/* raise privileges again */
void drop_privs();		/* finally drop privileges */


#define xtoi_1(p)   (*(p) <= '9'? (*(p)- '0'): \
                     *(p) <= 'F'? (*(p)-'A'+10):(*(p)-'a'+10))
#define xtoi_2(p)   ((xtoi_1(p) * 16) + xtoi_1((p)+1))


#endif
