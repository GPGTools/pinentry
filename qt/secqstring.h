/* secqstring.h - Secure version of QString.
   Copyright (C) 1992-2002 Trolltech AS.  All rights reserved.
   Copyright (C) 2003 g10 Code GmbH

   The license of the original qstring.h file from which this file is
   derived can be found below.  Modified by Marcus Brinkmann
   <marcus@g10code.de>.  All modifications are licensed as follows, so
   that the intersection of the two licenses is then the GNU General
   Public License version 2.

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

/****************************************************************************
** $Id$
**
** Definition of the SecQString class, and related Unicode functions.
**
** Created : 920609
**
** Copyright (C) 1992-2002 Trolltech AS.  All rights reserved.
**
** This file is part of the tools module of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses may use this file in accordance with the Qt Commercial License
** Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#ifndef SECQSTRING_H
#define SECQSTRING_H

extern "C"
{
#include "memory.h"
}

/* We need the original qchar and qstring for transparent conversion
   from QChar to QChar and QString to SecQString (but not the other
   way round).  */
#include <qstring.h>

#ifndef QT_H
#include "qcstring.h"
#endif // QT_H


/*****************************************************************************
  SecQString class
 *****************************************************************************/

class SecQString;
class SecQCharRef;
template <class T> class QDeepCopy;
#include <stdio.h>
// internal
struct Q_EXPORT SecQStringData : public QShared {
    SecQStringData() :
        QShared(), unicode(0), len(0), maxl(0) { ref(); }
    SecQStringData(QChar *u, uint l, uint m) :
        QShared(), unicode(u), len(l), maxl(m) { }
    ~SecQStringData() { if ( unicode ) ::secmem_free ((char*) unicode); }

    void deleteSelf();
    QChar *unicode;
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


class Q_EXPORT SecQString
{
public:
    SecQString();                                  // make null string
    SecQString( QChar );                           // one-char string
    SecQString( const SecQString & );                 // impl-shared copy
    /* We need a way to convert a QString to a SecQString ("importing"
       it).  Having no conversion for the other way prevents
       accidential bugs where the secure string is copied to insecure
       memory.  */
    SecQString( const QString & ); // deep copy
    SecQString( const QChar* unicode, uint length ); // deep copy
    ~SecQString();

    SecQString    &operator=( const SecQString & );   // impl-shared copy

    QT_STATIC_CONST SecQString null;

    bool        isNull()        const;
    bool        isEmpty()       const;
    uint        length()        const;
    void        truncate( uint pos );

    SecQString     left( uint len )  const;
    SecQString     right( uint len ) const;
    SecQString     mid( uint index, uint len=0xffffffff) const;


    SecQString    &insert( uint index, const SecQString & );
    SecQString    &insert( uint index, const QChar*, uint len );
    SecQString    &remove( uint index, uint len );
    SecQString    &replace( uint index, uint len, const SecQString & );
    SecQString    &replace( uint index, uint len, const QChar*, uint clen );

    SecQString    &operator+=( const SecQString &str );

    QChar at( uint i ) const
        { return i < d->len ? d->unicode[i] : QChar::null; }
    QChar operator[]( int i ) const { return at((uint)i); }
    SecQCharRef at( uint i );
    SecQCharRef operator[]( int i );

    QChar constref(uint i) const
        { return at(i); }
    QChar& ref(uint i)
        { // Optimized for easy-inlining by simple compilers.
            if ( d->count != 1 || i >= d->len )
                subat( i );
            return d->unicode[i];
        }

    const QChar* unicode() const { return d->unicode; }

    uchar* utf8() const;

    void setLength( uint newLength );

    bool isRightToLeft() const;


private:
    SecQString( int size, bool /* dummy */ );	// allocate size incl. \0

    void deref();
    void real_detach();
    void subat( uint );

    void grow( uint newLength );

    SecQStringData *d;
    static SecQStringData* shared_null;
    static SecQStringData* makeSharedNull();

    friend class SecQConstString;
    friend class QTextStream;
    SecQString( SecQStringData* dd, bool /* dummy */ ) : d(dd) { }

    // needed for QDeepCopy
    void detach();
    friend class QDeepCopy<SecQString>;
};

class Q_EXPORT SecQCharRef {
    friend class SecQString;
    SecQString& s;
    uint p;
    SecQCharRef(SecQString* str, uint pos) : s(*str), p(pos) { }

public:
    // most QChar operations repeated here

    // all this is not documented: We just say "like QChar" and let it be.
#ifndef Q_QDOC
    ushort unicode() const { return s.constref(p).unicode(); }

    // An operator= for each QChar cast constructors
    SecQCharRef operator=(char c ) { s.ref(p)=c; return *this; }
    SecQCharRef operator=(uchar c ) { s.ref(p)=c; return *this; }
    SecQCharRef operator=(QChar c ) { s.ref(p)=c; return *this; }
    SecQCharRef operator=(const SecQCharRef& c ) { s.ref(p)=c.unicode(); return *this; }
    SecQCharRef operator=(ushort rc ) { s.ref(p)=rc; return *this; }
    SecQCharRef operator=(short rc ) { s.ref(p)=rc; return *this; }
    SecQCharRef operator=(uint rc ) { s.ref(p)=rc; return *this; }
    SecQCharRef operator=(int rc ) { s.ref(p)=rc; return *this; }

    operator QChar () const { return s.constref(p); }

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
    QChar lower() const { return s.constref(p).lower(); }
    QChar upper() const { return s.constref(p).upper(); }

    QChar::Category category() const { return s.constref(p).category(); }
    QChar::Direction direction() const { return s.constref(p).direction(); }
    QChar::Joining joining() const { return s.constref(p).joining(); }
    bool mirrored() const { return s.constref(p).mirrored(); }
    QChar mirroredChar() const { return s.constref(p).mirroredChar(); }
    //    const SecQString &decomposition() const { return s.constref(p).decomposition(); }
    QChar::Decomposition decompositionTag() const { return s.constref(p).decompositionTag(); }
    unsigned char combiningClass() const { return s.constref(p).combiningClass(); }

    // Not the non-const ones of these.
    uchar cell() const { return s.constref(p).cell(); }
    uchar row() const { return s.constref(p).row(); }
#endif
};

inline SecQCharRef SecQString::at( uint i ) { return SecQCharRef(this,i); }
inline SecQCharRef SecQString::operator[]( int i ) { return at((uint)i); }

class Q_EXPORT SecQConstString : private SecQString {
public:
    SecQConstString( const QChar* unicode, uint length );
    ~SecQConstString();
    const SecQString& string() const { return *this; }
};


/*****************************************************************************
  SecQString inline functions
 *****************************************************************************/

// These two move code into makeSharedNull() and deletesData()
// to improve cache-coherence (and reduce code bloat), while
// keeping the common cases fast.
//
// No safe way to pre-init shared_null on ALL compilers/linkers.
inline SecQString::SecQString() :
    d(shared_null ? shared_null : makeSharedNull())
{
    d->ref();
}
//
inline SecQString::~SecQString()
{
    if ( d->deref() ) {
        if ( d != shared_null )
	    d->deleteSelf();
    }
}

// needed for QDeepCopy
inline void SecQString::detach()
{ real_detach(); }

inline bool SecQString::isNull() const
{ return unicode() == 0; }

inline uint SecQString::length() const
{ return d->len; }

inline bool SecQString::isEmpty() const
{ return length() == 0; }

/*****************************************************************************
  SecQString non-member operators
 *****************************************************************************/

Q_EXPORT inline const SecQString operator+( const SecQString &s1, const SecQString &s2 )
{
    SecQString tmp( s1 );
    tmp += s2;
    return tmp;
}

#endif // SECQSTRING_H
