/* secqstring.cpp - Secure version of QString.
   Copyright (C) 1992-2002 Trolltech AS.  All rights reserved.
   Copyright (C) 2003 g10 Code GmbH

   The license of the original qstring.cpp file from which this file
   is derived can be found below.  Modified by Marcus Brinkmann
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
** Implementation of the SecQString class and related Unicode functions
**
** Created : 920722
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

// Don't define it while compiling this module, or USERS of Qt will
// not be able to link.

#include "secqstring.h"

static uint computeNewMax( uint len )
{
    uint newMax = 4;
    while ( newMax < len )
	newMax *= 2;
    // try to save some memory
    if ( newMax >= 1024 * 1024 && len <= newMax - (newMax >> 2) )
	newMax -= newMax >> 2;
    return newMax;
}

// These macros are used for efficient allocation of QChar strings.
// IMPORTANT! If you change these, make sure you also change the
// "delete unicode" statement in ~SecQStringData() in SecQString.h correspondingly!

#define QT_ALLOC_SECQCHAR_VEC(N) (QChar*) ::secmem_malloc (sizeof(QChar) * (N))
#define QT_DELETE_SECQCHAR_VEC(P) ::secmem_free (P)


/*****************************************************************************
  SecQString member functions
 *****************************************************************************/

/*!
    \class SecQString SecQString.h
    \reentrant

    \brief The SecQString class provides an abstraction of Unicode text
    and the classic C '\0'-terminated char array.

    \ingroup tools
    \ingroup shared
    \ingroup text
    \mainclass

    SecQString uses \link shclass.html implicit sharing\endlink, which
    makes it very efficient and easy to use.

    In all of the SecQString methods that take \c {const char *}
    parameters, the \c {const char *} is interpreted as a classic
    C-style '\0'-terminated ASCII string. It is legal for the \c
    {const char *} parameter to be 0. If the \c {const char *} is not
    '\0'-terminated, the results are undefined. Functions that copy
    classic C strings into a SecQString will not copy the terminating
    '\0' character. The QChar array of the SecQString (as returned by
    unicode()) is generally not terminated by a '\0'. If you need to
    pass a SecQString to a function that requires a C '\0'-terminated
    string use latin1().

    \keyword SecQString::null
    A SecQString that has not been assigned to anything is \e null, i.e.
    both the length and data pointer is 0. A SecQString that references
    the empty string ("", a single '\0' char) is \e empty. Both null
    and empty SecQStrings are legal parameters to the methods. Assigning
    \c{(const char *) 0} to SecQString gives a null SecQString. For
    convenience, \c SecQString::null is a null SecQString. When sorting,
    empty strings come first, followed by non-empty strings, followed
    by null strings. We recommend using \c{if ( !str.isNull() )} to
    check for a non-null string rather than \c{if ( !str )}; see \l
    operator!() for an explanation.

    Note that if you find that you are mixing usage of \l QCString,
    SecQString, and \l QByteArray, this causes lots of unnecessary
    copying and might indicate that the true nature of the data you
    are dealing with is uncertain. If the data is '\0'-terminated 8-bit
    data, use \l QCString; if it is unterminated (i.e. contains '\0's)
    8-bit data, use \l QByteArray; if it is text, use SecQString.

    Lists of strings are handled by the SecQStringList class. You can
    split a string into a list of strings using SecQStringList::split(),
    and join a list of strings into a single string with an optional
    separator using SecQStringList::join(). You can obtain a list of
    strings from a string list that contain a particular substring or
    that match a particular \link qregexp.html regex\endlink using
    SecQStringList::grep().

    <b>Note for C programmers</b>

    Due to C++'s type system and the fact that SecQString is implicitly
    shared, SecQStrings may be treated like ints or other simple base
    types. For example:

    \code
    SecQString boolToString( bool b )
    {
	SecQString result;
	if ( b )
	    result = "True";
	else
	    result = "False";
	return result;
    }
    \endcode

    The variable, result, is an auto variable allocated on the stack.
    When return is called, because we're returning by value, The copy
    constructor is called and a copy of the string is returned. (No
    actual copying takes place thanks to the implicit sharing, see
    below.)

    Throughout Qt's source code you will encounter SecQString usages like
    this:
    \code
    SecQString func( const SecQString& input )
    {
	SecQString output = input;
	// process output
	return output;
    }
    \endcode

    The 'copying' of input to output is almost as fast as copying a
    pointer because behind the scenes copying is achieved by
    incrementing a reference count. SecQString (like all Qt's implicitly
    shared classes) operates on a copy-on-write basis, only copying if
    an instance is actually changed.

    If you wish to create a deep copy of a SecQString without losing any
    Unicode information then you should use QDeepCopy.

    \sa QChar QCString QByteArray SecQConstString
*/

Q_EXPORT SecQStringData *SecQString::shared_null = 0;
QT_STATIC_CONST_IMPL SecQString SecQString::null;
QT_STATIC_CONST_IMPL QChar QChar::null;
QT_STATIC_CONST_IMPL QChar QChar::replacement((ushort)0xfffd);
QT_STATIC_CONST_IMPL QChar QChar::byteOrderMark((ushort)0xfeff);
QT_STATIC_CONST_IMPL QChar QChar::byteOrderSwapped((ushort)0xfffe);
QT_STATIC_CONST_IMPL QChar QChar::nbsp((ushort)0x00a0);

SecQStringData* SecQString::makeSharedNull()
{
    SecQString::shared_null = new SecQStringData;
#if defined( Q_OS_MAC )
    SecQString *that = const_cast<SecQString *>(&SecQString::null);
    that->d = SecQString::shared_null;
#endif
    return SecQString::shared_null;
}

/*!
    \fn SecQString::SecQString()

    Constructs a null string, i.e. both the length and data pointer
    are 0.

    \sa isNull()
*/

/*!
    Constructs a string of length one, containing the character \a ch.
*/
SecQString::SecQString( QChar ch )
{
    d = new SecQStringData( QT_ALLOC_SECQCHAR_VEC( 1 ), 1, 1 );
    d->unicode[0] = ch;
}

/*!
    Constructs an implicitly shared copy of \a s. This is very fast
    since it only involves incrementing a reference count.
*/
SecQString::SecQString( const SecQString &s ) :
    d(s.d)
{
    d->ref();
}


SecQString::SecQString( int size, bool /*dummy*/ )
{
    if ( size ) {
	int l = size;
	QChar* uc = QT_ALLOC_SECQCHAR_VEC( l );
	d = new SecQStringData( uc, 0, l );
    } else {
	d = shared_null ? shared_null : (shared_null=new SecQStringData);
	d->ref();
    }
}


/* Deep copy of STR.  */
SecQString::SecQString( const QString &str )
{
    const QChar *unicode = str.unicode ();
    uint length = str.length ();

    if ( !unicode && !length ) {
	d = shared_null ? shared_null : makeSharedNull();
	d->ref();
    } else {
	QChar* uc = QT_ALLOC_SECQCHAR_VEC( length );
	if ( unicode )
	    memcpy(uc, unicode, length*sizeof(QChar));
	d = new SecQStringData(uc,unicode ? length : 0,length);
    }  
}


/*!
    Constructs a string that is a deep copy of the first \a length
    characters in the QChar array.

    If \a unicode and \a length are 0, then a null string is created.

    If only \a unicode is 0, the string is empty but has \a length
    characters of space preallocated: SecQString expands automatically
    anyway, but this may speed up some cases a little. We recommend
    using the plain constructor and setLength() for this purpose since
    it will result in more readable code.

    \sa isNull() setLength()
*/

SecQString::SecQString( const QChar* unicode, uint length )
{
    if ( !unicode && !length ) {
	d = shared_null ? shared_null : makeSharedNull();
	d->ref();
    } else {
	QChar* uc = QT_ALLOC_SECQCHAR_VEC( length );
	if ( unicode )
	    memcpy(uc, unicode, length*sizeof(QChar));
	d = new SecQStringData(uc,unicode ? length : 0,length);
    }
}

/*!
    \fn SecQString::~SecQString()

    Destroys the string and frees the string's data if this is the
    last reference to the string.
*/


/*!
    Deallocates any space reserved solely by this SecQString.

    If the string does not share its data with another SecQString
    instance, nothing happens; otherwise the function creates a new,
    unique copy of this string. This function is called whenever the
    string is modified.
*/

void SecQString::real_detach()
{
    setLength( length() );
}

void SecQString::deref()
{
    if ( d && d->deref() ) {
	if ( d != shared_null )
	    delete d;
	d = 0;
    }
}

void SecQStringData::deleteSelf()
{
    delete this;
}

/*!
    \fn SecQString& SecQString::operator=( QChar c )

    Sets the string to contain just the single character \a c.
*/


/*!
    \overload

    Assigns a shallow copy of \a s to this string and returns a
    reference to this string. This is very fast because the string
    isn't actually copied.
*/
SecQString &SecQString::operator=( const SecQString &s )
{
    s.d->ref();
    deref();
    d = s.d;
    return *this;
}


/*!
    \fn bool SecQString::isNull() const

    Returns TRUE if the string is null; otherwise returns FALSE. A
    null string is always empty.

    \code
	SecQString a;          // a.unicode() == 0, a.length() == 0
	a.isNull();         // TRUE, because a.unicode() == 0
	a.isEmpty();        // TRUE, because a.length() == 0
    \endcode

    \sa isEmpty(), length()
*/

/*!
    \fn bool SecQString::isEmpty() const

    Returns TRUE if the string is empty, i.e. if length() == 0;
    otherwise returns FALSE. Null strings are also empty.

    \code
	SecQString a( "" );
	a.isEmpty();        // TRUE
	a.isNull();         // FALSE

	SecQString b;
	b.isEmpty();        // TRUE
	b.isNull();         // TRUE
    \endcode

    \sa isNull(), length()
*/

/*!
    \fn uint SecQString::length() const

    Returns the length of the string.

    Null strings and empty strings have zero length.

    \sa isNull(), isEmpty()
*/

/*!
    If \a newLen is less than the length of the string, then the
    string is truncated at position \a newLen. Otherwise nothing
    happens.

    \code
	SecQString s = "truncate me";
	s.truncate( 5 );            // s == "trunc"
    \endcode

    \sa setLength()
*/

void SecQString::truncate( uint newLen )
{
    if ( newLen < d->len )
	setLength( newLen );
}

/*!
    Ensures that at least \a newLen characters are allocated to the
    string, and sets the length of the string to \a newLen. Any new
    space allocated contains arbitrary data.

    \sa reserve(), truncate()
*/
void SecQString::setLength( uint newLen )
{
    if ( d->count != 1 || newLen > d->maxl ||
	 ( newLen * 4 < d->maxl && d->maxl > 4 ) ) {
	// detach, grow or shrink
	uint newMax = computeNewMax( newLen );
	QChar* nd = QT_ALLOC_SECQCHAR_VEC( newMax );
	if ( nd ) {
	    uint len = QMIN( d->len, newLen );
	    memcpy( nd, d->unicode, sizeof(QChar) * len );
	    deref();
	    d = new SecQStringData( nd, newLen, newMax );
	}
    } else {
	d->len = newLen;
    }
}


/*!
    \internal

    Like setLength, but doesn't shrink the allocated memory.
*/
void SecQString::grow( uint newLen )
{
    if ( d->count != 1 || newLen > d->maxl ) {
	setLength( newLen );
    } else {
	d->len = newLen;
    }
}


/*!
    Returns a substring that contains the \a len leftmost characters
    of the string.

    The whole string is returned if \a len exceeds the length of the
    string.

    \code
	SecQString s = "Pineapple";
	SecQString t = s.left( 4 );    // t == "Pine"
    \endcode

    \sa right(), mid(), isEmpty()
*/

SecQString SecQString::left( uint len ) const
{
    if ( isEmpty() ) {
	return SecQString();
    } else if ( len == 0 ) {                    // ## just for 1.x compat:
	return SecQString ("");
    } else if ( len >= length() ) {
	return *this;
    } else {
	SecQString s( len, TRUE );
	memcpy( s.d->unicode, d->unicode, len * sizeof(QChar) );
	s.d->len = len;
	return s;
    }
}

/*!
    Returns a string that contains the \a len rightmost characters of
    the string.

    If \a len is greater than the length of the string then the whole
    string is returned.

    \code
	SecQString string( "Pineapple" );
	SecQString t = string.right( 5 );   // t == "apple"
    \endcode

    \sa left(), mid(), isEmpty()
*/

SecQString SecQString::right( uint len ) const
{
    if ( isEmpty() ) {
	return SecQString();
    } else if ( len == 0 ) {                    // ## just for 1.x compat:
	return SecQString ("");
    } else {
	uint l = length();
	if ( len >= l )
	    return *this;
	SecQString s( len, TRUE );
	memcpy( s.d->unicode, d->unicode+(l-len), len*sizeof(QChar) );
	s.d->len = len;
	return s;
    }
}

/*!
    Returns a string that contains the \a len characters of this
    string, starting at position \a index.

    Returns a null string if the string is empty or \a index is out of
    range. Returns the whole string from \a index if \a index + \a len
    exceeds the length of the string.

    \code
	SecQString s( "Five pineapples" );
	SecQString t = s.mid( 5, 4 );                  // t == "pine"
    \endcode

    \sa left(), right()
*/

SecQString SecQString::mid( uint index, uint len ) const
{
    uint slen = length();
    if ( isEmpty() || index >= slen ) {
	return SecQString();
    } else if ( len == 0 ) {                    // ## just for 1.x compat:
	return SecQString ("");
    } else {
	if ( len > slen-index )
	    len = slen - index;
	if ( index == 0 && len == slen )
	    return *this;
	register const QChar *p = unicode()+index;
	SecQString s( len, TRUE );
	memcpy( s.d->unicode, p, len * sizeof(QChar) );
	s.d->len = len;
	return s;
    }
}

/*!
    Inserts \a s into the string at position \a index.

    If \a index is beyond the end of the string, the string is
    extended with spaces to length \a index and \a s is then appended
    and returns a reference to the string.

    \code
	SecQString string( "I like fish" );
	str = string.insert( 2, "don't " );
	// str == "I don't like fish"
    \endcode

    \sa remove(), replace()
*/

SecQString &SecQString::insert( uint index, const SecQString &s )
{
    // the sub function takes care of &s == this case.
    return insert( index, s.unicode(), s.length() );
}

/*!
    \overload

    Inserts the first \a len characters in \a s into the string at
    position \a index and returns a reference to the string.
*/

SecQString &SecQString::insert( uint index, const QChar* s, uint len )
{
    if ( len == 0 )
	return *this;
    uint olen = length();
    int nlen = olen + len;

    if ( s >= d->unicode && (uint)(s - d->unicode) < d->maxl ) {
	// Part of me - take a copy.
	QChar *tmp = QT_ALLOC_SECQCHAR_VEC( len );
	memcpy(tmp,s,len*sizeof(QChar));
	insert(index,tmp,len);
	QT_DELETE_SECQCHAR_VEC( tmp );
	return *this;
    }

    if ( index >= olen ) {                      // insert after end of string
	grow( len + index );
	int n = index - olen;
	QChar* uc = d->unicode+olen;
	while (n--)
	    *uc++ = ' ';
	memcpy( d->unicode+index, s, sizeof(QChar)*len );
    } else {                                    // normal insert
	grow( nlen );
	memmove( d->unicode + index + len, unicode() + index,
		 sizeof(QChar) * (olen - index) );
	memcpy( d->unicode + index, s, sizeof(QChar) * len );
    }
    return *this;
}

/*!
    Removes \a len characters from the string starting at position \a
    index, and returns a reference to the string.

    If \a index is beyond the length of the string, nothing happens.
    If \a index is within the string, but \a index + \a len is beyond
    the end of the string, the string is truncated at position \a
    index.

    \code
	SecQString string( "Montreal" );
	string.remove( 1, 4 );      // string == "Meal"
    \endcode

    \sa insert(), replace()
*/

SecQString &SecQString::remove( uint index, uint len )
{
    uint olen = length();
    if ( index >= olen  ) {
	// range problems
    } else if ( index + len >= olen ) {  // index ok
	setLength( index );
    } else if ( len != 0 ) {
	real_detach();
	memmove( d->unicode+index, d->unicode+index+len,
		 sizeof(QChar)*(olen-index-len) );
	setLength( olen-len );
    }
    return *this;
}


/*!
    \overload

    Replaces \a len characters with \a slen characters of QChar data
    from \a s, starting at position \a index, and returns a reference
    to the string.

    \sa insert(), remove()
*/

SecQString &SecQString::replace( uint index, uint len, const QChar* s, uint slen )
{
    real_detach();
    if ( len == slen && index + len <= length() ) {
	// Optimized common case: replace without size change
	memcpy( d->unicode+index, s, len * sizeof(QChar) );
    } else if ( s >= d->unicode && (uint)(s - d->unicode) < d->maxl ) {
	// Part of me - take a copy.
	QChar *tmp = QT_ALLOC_SECQCHAR_VEC( slen );
	memcpy( tmp, s, slen * sizeof(QChar) );
	replace( index, len, tmp, slen );
	QT_DELETE_SECQCHAR_VEC( tmp );
    } else {
	remove( index, len );
	insert( index, s, slen );
    }
    return *this;
}


/*!
    Replaces \a len characters from the string with \a s, starting at
    position \a index, and returns a reference to the string.

    If \a index is beyond the length of the string, nothing is deleted
    and \a s is appended at the end of the string. If \a index is
    valid, but \a index + \a len is beyond the end of the string,
    the string is truncated at position \a index, then \a s is
    appended at the end.

    \code
	QString string( "Say yes!" );
	string = string.replace( 4, 3, "NO" );
	// string == "Say NO!"
    \endcode

    \sa insert(), remove()
*/

SecQString &SecQString::replace( uint index, uint len, const SecQString &s )
{
    return replace( index, len, s.unicode(), s.length() );
}


/*!
    Appends \a str to the string and returns a reference to the string.
*/
SecQString& SecQString::operator+=( const SecQString &str )
{
    uint len1 = length();
    uint len2 = str.length();
    if ( len2 ) {
	if ( isEmpty() ) {
	    operator=( str );
	} else {
	    grow( len1+len2 );
	    memcpy( d->unicode+len1, str.unicode(), sizeof(QChar)*len2 );
	}
    } else if ( isNull() && !str.isNull() ) {   // ## just for 1.x compat:
	*this = SecQString ("");
    }
    return *this;
}


/*!
    Returns the string encoded in UTF-8 format.

    See QTextCodec for more diverse coding/decoding of Unicode strings.

    \sa fromUtf8(), ascii(), latin1(), local8Bit()
*/
uchar *SecQString::utf8() const
{
    int l = length();
    int rlen = l*3+1;
    uchar* rstr = (uchar*) ::secmem_malloc (rlen);
    uchar* cursor = rstr;
    const QChar *ch = d->unicode;
    for (int i=0; i < l; i++) {
	uint u = ch->unicode();
 	if ( u < 0x80 ) {
 	    *cursor++ = (uchar)u;
 	} else {
 	    if ( u < 0x0800 ) {
		*cursor++ = 0xc0 | ((uchar) (u >> 6));
 	    } else {
		if (u >= 0xd800 && u < 0xdc00 && i < l-1) {
		    unsigned short low = ch[1].unicode();
		    if (low >= 0xdc00 && low < 0xe000) {
			++ch;
			++i;
			u = (u - 0xd800)*0x400 + (low - 0xdc00) + 0x10000;
		    }
		}
		if (u > 0xffff) {
		    // if people are working in utf8, but strings are encoded in eg. latin1, the resulting
		    // name might be invalid utf8. This and the corresponding code in fromUtf8 takes care
		    // we can handle this without loosing information. This can happen with latin filenames
		    // and a utf8 locale under Unix.
		    if (u > 0x10fe00 && u < 0x10ff00) {
			*cursor++ = (u - 0x10fe00);
			++ch;
			continue;
		    } else {
			*cursor++ = 0xf0 | ((uchar) (u >> 18));
			*cursor++ = 0x80 | ( ((uchar) (u >> 12)) & 0x3f);
		    }
		} else {
		    *cursor++ = 0xe0 | ((uchar) (u >> 12));
		}
 		*cursor++ = 0x80 | ( ((uchar) (u >> 6)) & 0x3f);
 	    }
 	    *cursor++ = 0x80 | ((uchar) (u&0x3f));
 	}
 	++ch;
    }
    /* FIXME: secmem_realloc doesn't release extra memory.  */
    *cursor = '\0';
    return rstr;
}


/*!
  \fn QChar SecQString::at( uint ) const

    Returns the character at index \a i, or 0 if \a i is beyond the
    length of the string.

    \code
	const SecQString string( "abcdefgh" );
	QChar ch = string.at( 4 );
	// ch == 'e'
    \endcode

    If the SecQString is not const (i.e. const SecQString) or const& (i.e.
    const SecQString &), then the non-const overload of at() will be used
    instead.
*/

/*!
    \fn QChar SecQString::constref(uint i) const

    Returns the QChar at index \a i by value.

    Equivalent to at(\a i).

    \sa ref()
*/

/*!
    \fn QChar& SecQString::ref(uint i)

    Returns the QChar at index \a i by reference, expanding the string
    with QChar::null if necessary. The resulting reference can be
    assigned to, or otherwise used immediately, but becomes invalid
    once furher modifications are made to the string.

    \code
	SecQString string("ABCDEF");
	QChar ch = string.ref( 3 );         // ch == 'D'
    \endcode

    \sa constref()
*/

/*!
    \fn QChar SecQString::operator[]( int ) const

    Returns the character at index \a i, or QChar::null if \a i is
    beyond the length of the string.

    If the SecQString is not const (i.e., const SecQString) or const\&
    (i.e., const SecQString\&), then the non-const overload of operator[]
    will be used instead.
*/

/*!
    \fn QCharRef SecQString::operator[]( int )

    \overload

    The function returns a reference to the character at index \a i.
    The resulting reference can then be assigned to, or used
    immediately, but it will become invalid once further modifications
    are made to the original string.

    If \a i is beyond the length of the string then the string is
    expanded with QChar::nulls, so that the QCharRef references a
    valid (null) character in the string.

    The QCharRef internal class can be used much like a constant
    QChar, but if you assign to it, you change the original string
    (which will detach itself because of SecQString's copy-on-write
    semantics). You will get compilation errors if you try to use the
    result as anything but a QChar.
*/

/*!
    \fn QCharRef SecQString::at( uint i )

    \overload

    The function returns a reference to the character at index \a i.
    The resulting reference can then be assigned to, or used
    immediately, but it will become invalid once further modifications
    are made to the original string.

    If \a i is beyond the length of the string then the string is
    expanded with QChar::null.
*/

/*
  Internal chunk of code to handle the
  uncommon cases of at() above.
*/
void SecQString::subat( uint i )
{
    uint olen = d->len;
    if ( i >= olen ) {
	setLength( i+1 );               // i is index; i+1 is needed length
	for ( uint j=olen; j<=i; j++ )
	    d->unicode[j] = QChar::null;
    } else {
	// Just be sure to detach
	real_detach();
    }
}


/*! \internal
 */
bool SecQString::isRightToLeft() const
{
    int len = length();
    QChar *p = d->unicode;
    while ( len-- ) {
	switch( (*p).direction () )
	{
	case QChar::DirL:
	case QChar::DirLRO:
	case QChar::DirLRE:
	    return FALSE;
	case QChar::DirR:
	case QChar::DirAL:
	case QChar::DirRLO:
	case QChar::DirRLE:
	    return TRUE;
	default:
	    break;
	}
	++p;
    }
    return FALSE;
}


/*!
    \fn const SecQString operator+( const SecQString &s1, const SecQString &s2 )

    \relates SecQString

    Returns a string which is the result of concatenating the string
    \a s1 and the string \a s2.

    Equivalent to \a {s1}.append(\a s2).
*/


/*! \fn void SecQString::detach()
  If the string does not share its data with another SecQString instance,
  nothing happens; otherwise the function creates a new, unique copy of
  this string. This function is called whenever the string is modified. The
  implicit sharing mechanism is implemented this way.
*/

