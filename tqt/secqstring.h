/* secntqstring.h - Secure version of TQString.
 * Copyright (C) 1992-2002 Trolltech AS.  All rights reserved.
 * Copyright (C) 2003 g10 Code GmbH
 *
 * The license of the original ntqstring.h file from which this file is
 * derived can be found below.  Modified by Marcus Brinkmann
 * <marcus@g10code.de>.  All modifications are licensed as follows, so
 * that the intersection of the two licenses is then the GNU General
 * Public License version 2.
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
 * SPDX-License-Identifier: GPL-2.0
 */

/****************************************************************************
** $Id$
**
** Definition of the SecTQString class, and related Unicode functions.
**
** Created : 920609
**
** Copyright (C) 1992-2002 Trolltech AS.  All rights reserved.
**
** This file is part of the tools module of the TQt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.TQPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid TQt Enterprise Edition or TQt Professional Edition
** licenses may use this file in accordance with the TQt Commercial License
** Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about TQt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for TQPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#ifndef SECTQSTRING_H
#define SECTQSTRING_H

extern "C"
{
#include "memory.h"
}

/* We need the original qchar and qstring for transparent conversion
   from TQChar to TQChar and TQString to SecTQString (but not the other
   way round).  */
#include <ntqstring.h>

#ifndef QT_H
#include "ntqcstring.h"
#endif // QT_H


/*****************************************************************************
  SecTQString class
 *****************************************************************************/

class SecTQString;
class SecTQCharRef;
template <class T> class TQDeepCopy;
#include <stdio.h>
// internal
struct Q_EXPORT SecTQStringData : public TQShared {
    SecTQStringData() :
        TQShared(), unicode(0), len(0), maxl(0) { ref(); }
    SecTQStringData(TQChar *u, uint l, uint m) :
        TQShared(), unicode(u), len(l), maxl(m) { }
    ~SecTQStringData() { if ( unicode ) ::secmem_free ((char*) unicode); }

    void deleteSelf();
    TQChar *unicode;
#ifdef Q_OS_MAC9
    uint len;
#else
    uint len : 30;
#endif
#ifdef Q_OS_MAC9
    uint maxl;
#else
    uint maxl : 30;
#endif
};


class Q_EXPORT SecTQString
{
public:
    SecTQString();                                  // make null string
    SecTQString( TQChar );                           // one-char string
    SecTQString( const SecTQString & );                 // impl-shared copy
    /* We need a way to convert a TQString to a SecTQString ("importing"
       it).  Having no conversion for the other way prevents
       accidential bugs where the secure string is copied to insecure
       memory.  */
    SecTQString( const TQString & ); // deep copy
    SecTQString( const TQChar* unicode, uint length ); // deep copy
    ~SecTQString();

    SecTQString    &operator=( const SecTQString & );   // impl-shared copy

    QT_STATIC_CONST SecTQString null;

    bool        isNull()        const;
    bool        isEmpty()       const;
    uint        length()        const;
    void        truncate( uint pos );

    SecTQString     left( uint len )  const;
    SecTQString     right( uint len ) const;
    SecTQString     mid( uint index, uint len=0xffffffff) const;


    SecTQString    &insert( uint index, const SecTQString & );
    SecTQString    &insert( uint index, const TQChar*, uint len );
    SecTQString    &remove( uint index, uint len );
    SecTQString    &replace( uint index, uint len, const SecTQString & );
    SecTQString    &replace( uint index, uint len, const TQChar*, uint clen );

    SecTQString    &operator+=( const SecTQString &str );

    TQChar at( uint i ) const
        { return i < d->len ? d->unicode[i] : TQChar::null; }
    TQChar operator[]( int i ) const { return at((uint)i); }
    SecTQCharRef at( uint i );
    SecTQCharRef operator[]( int i );

    TQChar constref(uint i) const
        { return at(i); }
    TQChar& ref(uint i)
        { // Optimized for easy-inlining by simple compilers.
            if ( d->count != 1 || i >= d->len )
                subat( i );
            return d->unicode[i];
        }

    const TQChar* unicode() const { return d->unicode; }

    uchar* utf8() const;

    void setLength( uint newLength );

    bool isRightToLeft() const;


private:
    SecTQString( int size, bool /* dummy */ );	// allocate size incl. \0

    void deref();
    void real_detach();
    void subat( uint );

    void grow( uint newLength );

    SecTQStringData *d;
    static SecTQStringData* shared_null;
    static SecTQStringData* makeSharedNull();

    friend class SecTQConstString;
    friend class TQTextStream;
    SecTQString( SecTQStringData* dd, bool /* dummy */ ) : d(dd) { }

    // needed for TQDeepCopy
    void detach();
    friend class TQDeepCopy<SecTQString>;
};

class Q_EXPORT SecTQCharRef {
    friend class SecTQString;
    SecTQString& s;
    uint p;
    SecTQCharRef(SecTQString* str, uint pos) : s(*str), p(pos) { }

public:
    // most TQChar operations repeated here

    // all this is not documented: We just say "like TQChar" and let it be.
#ifndef Q_QDOC
    ushort unicode() const { return s.constref(p).unicode(); }

    // An operator= for each TQChar cast constructors
    SecTQCharRef operator=(char c ) { s.ref(p)=c; return *this; }
    SecTQCharRef operator=(uchar c ) { s.ref(p)=c; return *this; }
    SecTQCharRef operator=(TQChar c ) { s.ref(p)=c; return *this; }
    SecTQCharRef operator=(const SecTQCharRef& c ) { s.ref(p)=c.unicode(); return *this; }
    SecTQCharRef operator=(ushort rc ) { s.ref(p)=rc; return *this; }
    SecTQCharRef operator=(short rc ) { s.ref(p)=rc; return *this; }
    SecTQCharRef operator=(uint rc ) { s.ref(p)=rc; return *this; }
    SecTQCharRef operator=(int rc ) { s.ref(p)=rc; return *this; }

    operator TQChar () const { return s.constref(p); }

    // each function...
    bool isNull() const { return unicode()==0; }
    bool isPrint() const { return s.constref(p).isPrint(); }
    bool isPunct() const { return s.constref(p).isPunct(); }
    bool isSpace() const { return s.constref(p).isSpace(); }
    bool isMark() const { return s.constref(p).isMark(); }
    bool isLetter() const { return s.constref(p).isLetter(); }
    bool isNumber() const { return s.constref(p).isNumber(); }
    bool isLetterOrNumber() { return s.constref(p).isLetterOrNumber(); }
    bool isDigit() const { return s.constref(p).isDigit(); }

    int digitValue() const { return s.constref(p).digitValue(); }
    TQChar lower() const { return s.constref(p).lower(); }
    TQChar upper() const { return s.constref(p).upper(); }

    TQChar::Category category() const { return s.constref(p).category(); }
    TQChar::Direction direction() const { return s.constref(p).direction(); }
    TQChar::Joining joining() const { return s.constref(p).joining(); }
    bool mirrored() const { return s.constref(p).mirrored(); }
    TQChar mirroredChar() const { return s.constref(p).mirroredChar(); }
    //    const SecTQString &decomposition() const { return s.constref(p).decomposition(); }
    TQChar::Decomposition decompositionTag() const { return s.constref(p).decompositionTag(); }
    unsigned char combiningClass() const { return s.constref(p).combiningClass(); }

    // Not the non-const ones of these.
    uchar cell() const { return s.constref(p).cell(); }
    uchar row() const { return s.constref(p).row(); }
#endif
};

inline SecTQCharRef SecTQString::at( uint i ) { return SecTQCharRef(this,i); }
inline SecTQCharRef SecTQString::operator[]( int i ) { return at((uint)i); }

class Q_EXPORT SecTQConstString : private SecTQString {
public:
    SecTQConstString( const TQChar* unicode, uint length );
    ~SecTQConstString();
    const SecTQString& string() const { return *this; }
};


/*****************************************************************************
  SecTQString inline functions
 *****************************************************************************/

// These two move code into makeSharedNull() and deletesData()
// to improve cache-coherence (and reduce code bloat), while
// keeping the common cases fast.
//
// No safe way to pre-init shared_null on ALL compilers/linkers.
inline SecTQString::SecTQString() :
    d(shared_null ? shared_null : makeSharedNull())
{
    d->ref();
}
//
inline SecTQString::~SecTQString()
{
    if ( d->deref() ) {
        if ( d != shared_null )
	    d->deleteSelf();
    }
}

// needed for TQDeepCopy
inline void SecTQString::detach()
{ real_detach(); }

inline bool SecTQString::isNull() const
{ return unicode() == 0; }

inline uint SecTQString::length() const
{ return d->len; }

inline bool SecTQString::isEmpty() const
{ return length() == 0; }

/*****************************************************************************
  SecTQString non-member operators
 *****************************************************************************/

Q_EXPORT inline const SecTQString operator+( const SecTQString &s1, const SecTQString &s2 )
{
    SecTQString tmp( s1 );
    tmp += s2;
    return tmp;
}

#endif // SECTQSTRING_H
