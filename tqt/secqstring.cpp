/* secqstring.cpp - Secure version of TQString.
 * Copyright (C) 1992-2002 Trolltech AS.  All rights reserved.
 * Copyright (C) 2003 g10 Code GmbH
 *
 * The license of the original qstring.cpp file from which this file
 * is derived can be found below.  Modified by Marcus Brinkmann
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
** Implementation of the SecTQString class and related Unicode functions
**
** Created : 920722
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

// Don't define it while compiling this module, or USERS of TQt will
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

// These macros are used for efficient allocation of TQChar strings.
// IMPORTANT! If you change these, make sure you also change the
// "delete unicode" statement in ~SecTQStringData() in SecTQString.h correspondingly!

#define QT_ALLOC_SECTQCHAR_VEC(N) (TQChar*) ::secmem_malloc (sizeof(TQChar) * (N))
#define QT_DELETE_SECTQCHAR_VEC(P) ::secmem_free (P)


/*****************************************************************************
  SecTQString member functions
 *****************************************************************************/

/*!
    \class SecTQString SecTQString.h
    \reentrant

    \brief The SecTQString class provides an abstraction of Unicode text
    and the classic C '\0'-terminated char array.

    \ingroup tools
    \ingroup shared
    \ingroup text
    \mainclass

    SecTQString uses \link shclass.html implicit sharing\endlink, which
    makes it very efficient and easy to use.

    In all of the SecTQString methods that take \c {const char *}
    parameters, the \c {const char *} is interpreted as a classic
    C-style '\0'-terminated ASCII string. It is legal for the \c
    {const char *} parameter to be 0. If the \c {const char *} is not
    '\0'-terminated, the results are undefined. Functions that copy
    classic C strings into a SecTQString will not copy the terminating
    '\0' character. The TQChar array of the SecTQString (as returned by
    unicode()) is generally not terminated by a '\0'. If you need to
    pass a SecTQString to a function that requires a C '\0'-terminated
    string use latin1().

    \keyword SecTQString::null
    A SecTQString that has not been assigned to anything is \e null, i.e.
    both the length and data pointer is 0. A SecTQString that references
    the empty string ("", a single '\0' char) is \e empty. Both null
    and empty SecTQStrings are legal parameters to the methods. Assigning
    \c{(const char *) 0} to SecTQString gives a null SecTQString. For
    convenience, \c SecTQString::null is a null SecTQString. When sorting,
    empty strings come first, followed by non-empty strings, followed
    by null strings. We recommend using \c{if ( !str.isNull() )} to
    check for a non-null string rather than \c{if ( !str )}; see \l
    operator!() for an explanation.

    Note that if you find that you are mixing usage of \l TQCString,
    SecTQString, and \l TQByteArray, this causes lots of unnecessary
    copying and might indicate that the true nature of the data you
    are dealing with is uncertain. If the data is '\0'-terminated 8-bit
    data, use \l TQCString; if it is unterminated (i.e. contains '\0's)
    8-bit data, use \l TQByteArray; if it is text, use SecTQString.

    Lists of strings are handled by the SecTQStringList class. You can
    split a string into a list of strings using SecTQStringList::split(),
    and join a list of strings into a single string with an optional
    separator using SecTQStringList::join(). You can obtain a list of
    strings from a string list that contain a particular substring or
    that match a particular \link ntqregexp.html regex\endlink using
    SecTQStringList::grep().

    <b>Note for C programmers</b>

    Due to C++'s type system and the fact that SecTQString is implicitly
    shared, SecTQStrings may be treated like ints or other simple base
    types. For example:

    \code
    SecTQString boolToString( bool b )
    {
	SecTQString result;
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

    Throughout TQt's source code you will encounter SecTQString usages like
    this:
    \code
    SecTQString func( const SecTQString& input )
    {
	SecTQString output = input;
	// process output
	return output;
    }
    \endcode

    The 'copying' of input to output is almost as fast as copying a
    pointer because behind the scenes copying is achieved by
    incrementing a reference count. SecTQString (like all TQt's implicitly
    shared classes) operates on a copy-on-write basis, only copying if
    an instance is actually changed.

    If you wish to create a deep copy of a SecTQString without losing any
    Unicode information then you should use TQDeepCopy.

    \sa TQChar TQCString TQByteArray SecTQConstString
*/

Q_EXPORT SecTQStringData *SecTQString::shared_null = 0;
QT_STATIC_CONST_IMPL SecTQString SecTQString::null;
QT_STATIC_CONST_IMPL TQChar TQChar::null;
QT_STATIC_CONST_IMPL TQChar TQChar::replacement((ushort)0xfffd);
QT_STATIC_CONST_IMPL TQChar TQChar::byteOrderMark((ushort)0xfeff);
QT_STATIC_CONST_IMPL TQChar TQChar::byteOrderSwapped((ushort)0xfffe);
QT_STATIC_CONST_IMPL TQChar TQChar::nbsp((ushort)0x00a0);

SecTQStringData* SecTQString::makeSharedNull()
{
    SecTQString::shared_null = new SecTQStringData;
#if defined( Q_OS_MAC )
    SecTQString *that = const_cast<SecTQString *>(&SecTQString::null);
    that->d = SecTQString::shared_null;
#endif
    return SecTQString::shared_null;
}

/*!
    \fn SecTQString::SecTQString()

    Constructs a null string, i.e. both the length and data pointer
    are 0.

    \sa isNull()
*/

/*!
    Constructs a string of length one, containing the character \a ch.
*/
SecTQString::SecTQString( TQChar ch )
{
    d = new SecTQStringData( QT_ALLOC_SECTQCHAR_VEC( 1 ), 1, 1 );
    d->unicode[0] = ch;
}

/*!
    Constructs an implicitly shared copy of \a s. This is very fast
    since it only involves incrementing a reference count.
*/
SecTQString::SecTQString( const SecTQString &s ) :
    d(s.d)
{
    d->ref();
}


SecTQString::SecTQString( int size, bool /*dummy*/ )
{
    if ( size ) {
	int l = size;
	TQChar* uc = QT_ALLOC_SECTQCHAR_VEC( l );
	d = new SecTQStringData( uc, 0, l );
    } else {
	d = shared_null ? shared_null : (shared_null=new SecTQStringData);
	d->ref();
    }
}


/* Deep copy of STR.  */
SecTQString::SecTQString( const TQString &str )
{
    const TQChar *unicode = str.unicode ();
    uint length = str.length ();

    if ( !unicode && !length ) {
	d = shared_null ? shared_null : makeSharedNull();
	d->ref();
    } else {
	TQChar* uc = QT_ALLOC_SECTQCHAR_VEC( length );
	if ( unicode )
	    memcpy(uc, unicode, length*sizeof(TQChar));
	d = new SecTQStringData(uc,unicode ? length : 0,length);
    }
}


/*!
    Constructs a string that is a deep copy of the first \a length
    characters in the TQChar array.

    If \a unicode and \a length are 0, then a null string is created.

    If only \a unicode is 0, the string is empty but has \a length
    characters of space preallocated: SecTQString expands automatically
    anyway, but this may speed up some cases a little. We recommend
    using the plain constructor and setLength() for this purpose since
    it will result in more readable code.

    \sa isNull() setLength()
*/

SecTQString::SecTQString( const TQChar* unicode, uint length )
{
    if ( !unicode && !length ) {
	d = shared_null ? shared_null : makeSharedNull();
	d->ref();
    } else {
	TQChar* uc = QT_ALLOC_SECTQCHAR_VEC( length );
	if ( unicode )
	    memcpy(uc, unicode, length*sizeof(TQChar));
	d = new SecTQStringData(uc,unicode ? length : 0,length);
    }
}

/*!
    \fn SecTQString::~SecTQString()

    Destroys the string and frees the string's data if this is the
    last reference to the string.
*/


/*!
    Deallocates any space reserved solely by this SecTQString.

    If the string does not share its data with another SecTQString
    instance, nothing happens; otherwise the function creates a new,
    unique copy of this string. This function is called whenever the
    string is modified.
*/

void SecTQString::real_detach()
{
    setLength( length() );
}

void SecTQString::deref()
{
    if ( d && d->deref() ) {
	if ( d != shared_null )
	    delete d;
	d = 0;
    }
}

void SecTQStringData::deleteSelf()
{
    delete this;
}

/*!
    \fn SecTQString& SecTQString::operator=( TQChar c )

    Sets the string to contain just the single character \a c.
*/


/*!
    \overload

    Assigns a shallow copy of \a s to this string and returns a
    reference to this string. This is very fast because the string
    isn't actually copied.
*/
SecTQString &SecTQString::operator=( const SecTQString &s )
{
    s.d->ref();
    deref();
    d = s.d;
    return *this;
}


/*!
    \fn bool SecTQString::isNull() const

    Returns TRUE if the string is null; otherwise returns FALSE. A
    null string is always empty.

    \code
	SecTQString a;          // a.unicode() == 0, a.length() == 0
	a.isNull();         // TRUE, because a.unicode() == 0
	a.isEmpty();        // TRUE, because a.length() == 0
    \endcode

    \sa isEmpty(), length()
*/

/*!
    \fn bool SecTQString::isEmpty() const

    Returns TRUE if the string is empty, i.e. if length() == 0;
    otherwise returns FALSE. Null strings are also empty.

    \code
	SecTQString a( "" );
	a.isEmpty();        // TRUE
	a.isNull();         // FALSE

	SecTQString b;
	b.isEmpty();        // TRUE
	b.isNull();         // TRUE
    \endcode

    \sa isNull(), length()
*/

/*!
    \fn uint SecTQString::length() const

    Returns the length of the string.

    Null strings and empty strings have zero length.

    \sa isNull(), isEmpty()
*/

/*!
    If \a newLen is less than the length of the string, then the
    string is truncated at position \a newLen. Otherwise nothing
    happens.

    \code
	SecTQString s = "truncate me";
	s.truncate( 5 );            // s == "trunc"
    \endcode

    \sa setLength()
*/

void SecTQString::truncate( uint newLen )
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
void SecTQString::setLength( uint newLen )
{
    if ( d->count != 1 || newLen > d->maxl ||
	 ( newLen * 4 < d->maxl && d->maxl > 4 ) ) {
	// detach, grow or shrink
	uint newMax = computeNewMax( newLen );
	TQChar* nd = QT_ALLOC_SECTQCHAR_VEC( newMax );
	if ( nd ) {
	    uint len = TQMIN( d->len, newLen );
	    memcpy( nd, d->unicode, sizeof(TQChar) * len );
	    deref();
	    d = new SecTQStringData( nd, newLen, newMax );
	}
    } else {
	d->len = newLen;
    }
}


/*!
    \internal

    Like setLength, but doesn't shrink the allocated memory.
*/
void SecTQString::grow( uint newLen )
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
	SecTQString s = "Pineapple";
	SecTQString t = s.left( 4 );    // t == "Pine"
    \endcode

    \sa right(), mid(), isEmpty()
*/

SecTQString SecTQString::left( uint len ) const
{
    if ( isEmpty() ) {
	return SecTQString();
    } else if ( len == 0 ) {                    // ## just for 1.x compat:
	return SecTQString ("");
    } else if ( len >= length() ) {
	return *this;
    } else {
	SecTQString s( len, TRUE );
	memcpy( s.d->unicode, d->unicode, len * sizeof(TQChar) );
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
	SecTQString string( "Pineapple" );
	SecTQString t = string.right( 5 );   // t == "apple"
    \endcode

    \sa left(), mid(), isEmpty()
*/

SecTQString SecTQString::right( uint len ) const
{
    if ( isEmpty() ) {
	return SecTQString();
    } else if ( len == 0 ) {                    // ## just for 1.x compat:
	return SecTQString ("");
    } else {
	uint l = length();
	if ( len >= l )
	    return *this;
	SecTQString s( len, TRUE );
	memcpy( s.d->unicode, d->unicode+(l-len), len*sizeof(TQChar) );
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
	SecTQString s( "Five pineapples" );
	SecTQString t = s.mid( 5, 4 );                  // t == "pine"
    \endcode

    \sa left(), right()
*/

SecTQString SecTQString::mid( uint index, uint len ) const
{
    uint slen = length();
    if ( isEmpty() || index >= slen ) {
	return SecTQString();
    } else if ( len == 0 ) {                    // ## just for 1.x compat:
	return SecTQString ("");
    } else {
	if ( len > slen-index )
	    len = slen - index;
	if ( index == 0 && len == slen )
	    return *this;
	register const TQChar *p = unicode()+index;
	SecTQString s( len, TRUE );
	memcpy( s.d->unicode, p, len * sizeof(TQChar) );
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
	SecTQString string( "I like fish" );
	str = string.insert( 2, "don't " );
	// str == "I don't like fish"
    \endcode

    \sa remove(), replace()
*/

SecTQString &SecTQString::insert( uint index, const SecTQString &s )
{
    // the sub function takes care of &s == this case.
    return insert( index, s.unicode(), s.length() );
}

/*!
    \overload

    Inserts the first \a len characters in \a s into the string at
    position \a index and returns a reference to the string.
*/

SecTQString &SecTQString::insert( uint index, const TQChar* s, uint len )
{
    if ( len == 0 )
	return *this;
    uint olen = length();
    int nlen = olen + len;

    if ( s >= d->unicode && (uint)(s - d->unicode) < d->maxl ) {
	// Part of me - take a copy.
	TQChar *tmp = QT_ALLOC_SECTQCHAR_VEC( len );
	memcpy(tmp,s,len*sizeof(TQChar));
	insert(index,tmp,len);
	QT_DELETE_SECTQCHAR_VEC( tmp );
	return *this;
    }

    if ( index >= olen ) {                      // insert after end of string
	grow( len + index );
	int n = index - olen;
	TQChar* uc = d->unicode+olen;
	while (n--)
	    *uc++ = ' ';
	memcpy( d->unicode+index, s, sizeof(TQChar)*len );
    } else {                                    // normal insert
	grow( nlen );
	memmove( d->unicode + index + len, unicode() + index,
		 sizeof(TQChar) * (olen - index) );
	memcpy( d->unicode + index, s, sizeof(TQChar) * len );
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
	SecTQString string( "Montreal" );
	string.remove( 1, 4 );      // string == "Meal"
    \endcode

    \sa insert(), replace()
*/

SecTQString &SecTQString::remove( uint index, uint len )
{
    uint olen = length();
    if ( index >= olen  ) {
	// range problems
    } else if ( index + len >= olen ) {  // index ok
	setLength( index );
    } else if ( len != 0 ) {
	real_detach();
	memmove( d->unicode+index, d->unicode+index+len,
		 sizeof(TQChar)*(olen-index-len) );
	setLength( olen-len );
    }
    return *this;
}


/*!
    \overload

    Replaces \a len characters with \a slen characters of TQChar data
    from \a s, starting at position \a index, and returns a reference
    to the string.

    \sa insert(), remove()
*/

SecTQString &SecTQString::replace( uint index, uint len, const TQChar* s, uint slen )
{
    real_detach();
    if ( len == slen && index + len <= length() ) {
	// Optimized common case: replace without size change
	memcpy( d->unicode+index, s, len * sizeof(TQChar) );
    } else if ( s >= d->unicode && (uint)(s - d->unicode) < d->maxl ) {
	// Part of me - take a copy.
	TQChar *tmp = QT_ALLOC_SECTQCHAR_VEC( slen );
	memcpy( tmp, s, slen * sizeof(TQChar) );
	replace( index, len, tmp, slen );
	QT_DELETE_SECTQCHAR_VEC( tmp );
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
	TQString string( "Say yes!" );
	string = string.replace( 4, 3, "NO" );
	// string == "Say NO!"
    \endcode

    \sa insert(), remove()
*/

SecTQString &SecTQString::replace( uint index, uint len, const SecTQString &s )
{
    return replace( index, len, s.unicode(), s.length() );
}


/*!
    Appends \a str to the string and returns a reference to the string.
*/
SecTQString& SecTQString::operator+=( const SecTQString &str )
{
    uint len1 = length();
    uint len2 = str.length();
    if ( len2 ) {
	if ( isEmpty() ) {
	    operator=( str );
	} else {
	    grow( len1+len2 );
	    memcpy( d->unicode+len1, str.unicode(), sizeof(TQChar)*len2 );
	}
    } else if ( isNull() && !str.isNull() ) {   // ## just for 1.x compat:
	*this = SecTQString ("");
    }
    return *this;
}


/*!
    Returns the string encoded in UTF-8 format.

    See TQTextCodec for more diverse coding/decoding of Unicode strings.

    \sa fromUtf8(), ascii(), latin1(), local8Bit()
*/
uchar *SecTQString::utf8() const
{
    int l = length();
    int rlen = l*3+1;
    uchar* rstr = (uchar*) ::secmem_malloc (rlen);
    uchar* cursor = rstr;
    const TQChar *ch = d->unicode;
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
  \fn TQChar SecTQString::at( uint ) const

    Returns the character at index \a i, or 0 if \a i is beyond the
    length of the string.

    \code
	const SecTQString string( "abcdefgh" );
	TQChar ch = string.at( 4 );
	// ch == 'e'
    \endcode

    If the SecTQString is not const (i.e. const SecTQString) or const& (i.e.
    const SecTQString &), then the non-const overload of at() will be used
    instead.
*/

/*!
    \fn TQChar SecTQString::constref(uint i) const

    Returns the TQChar at index \a i by value.

    Equivalent to at(\a i).

    \sa ref()
*/

/*!
    \fn TQChar& SecTQString::ref(uint i)

    Returns the TQChar at index \a i by reference, expanding the string
    with TQChar::null if necessary. The resulting reference can be
    assigned to, or otherwise used immediately, but becomes invalid
    once furher modifications are made to the string.

    \code
	SecTQString string("ABCDEF");
	TQChar ch = string.ref( 3 );         // ch == 'D'
    \endcode

    \sa constref()
*/

/*!
    \fn TQChar SecTQString::operator[]( int ) const

    Returns the character at index \a i, or TQChar::null if \a i is
    beyond the length of the string.

    If the SecTQString is not const (i.e., const SecTQString) or const\&
    (i.e., const SecTQString\&), then the non-const overload of operator[]
    will be used instead.
*/

/*!
    \fn TQCharRef SecTQString::operator[]( int )

    \overload

    The function returns a reference to the character at index \a i.
    The resulting reference can then be assigned to, or used
    immediately, but it will become invalid once further modifications
    are made to the original string.

    If \a i is beyond the length of the string then the string is
    expanded with TQChar::nulls, so that the TQCharRef references a
    valid (null) character in the string.

    The TQCharRef internal class can be used much like a constant
    TQChar, but if you assign to it, you change the original string
    (which will detach itself because of SecTQString's copy-on-write
    semantics). You will get compilation errors if you try to use the
    result as anything but a TQChar.
*/

/*!
    \fn TQCharRef SecTQString::at( uint i )

    \overload

    The function returns a reference to the character at index \a i.
    The resulting reference can then be assigned to, or used
    immediately, but it will become invalid once further modifications
    are made to the original string.

    If \a i is beyond the length of the string then the string is
    expanded with TQChar::null.
*/

/*
  Internal chunk of code to handle the
  uncommon cases of at() above.
*/
void SecTQString::subat( uint i )
{
    uint olen = d->len;
    if ( i >= olen ) {
	setLength( i+1 );               // i is index; i+1 is needed length
	for ( uint j=olen; j<=i; j++ )
	    d->unicode[j] = TQChar::null;
    } else {
	// Just be sure to detach
	real_detach();
    }
}


/*! \internal
 */
bool SecTQString::isRightToLeft() const
{
    int len = length();
    TQChar *p = d->unicode;
    while ( len-- ) {
	switch( (*p).direction () )
	{
	case TQChar::DirL:
	case TQChar::DirLRO:
	case TQChar::DirLRE:
	    return FALSE;
	case TQChar::DirR:
	case TQChar::DirAL:
	case TQChar::DirRLO:
	case TQChar::DirRLE:
	    return TRUE;
	default:
	    break;
	}
	++p;
    }
    return FALSE;
}


/*!
    \fn const SecTQString operator+( const SecTQString &s1, const SecTQString &s2 )

    \relates SecTQString

    Returns a string which is the result of concatenating the string
    \a s1 and the string \a s2.

    Equivalent to \a {s1}.append(\a s2).
*/


/*! \fn void SecTQString::detach()
  If the string does not share its data with another SecTQString instance,
  nothing happens; otherwise the function creates a new, unique copy of
  this string. This function is called whenever the string is modified. The
  implicit sharing mechanism is implemented this way.
*/
