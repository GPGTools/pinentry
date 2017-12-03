/* secqlineedit.cpp - Secure version of TQLineEdit.
 * Copyright (C) 1992-2002 Trolltech AS.  All rights reserved.
 * Copyright (C) 2003 g10 Code GmbH
 *
 * The license of the original qlineedit.cpp file from which this file
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


/* Undo/Redo is disabled, because it uses unsecure memory for the
   history buffer.  */
#define SECURE_NO_UNDO	1


/**********************************************************************
** $Id$
**
** Implementation of SecTQLineEdit widget class
**
** Created : 941011
**
** Copyright (C) 1992-2002 Trolltech AS.  All rights reserved.
**
** This file is part of the widgets module of the TQt GUI Toolkit.
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

#include "secqlineedit.h"
#include "ntqpainter.h"
#include "ntqdrawutil.h"
#include "ntqfontmetrics.h"
#include "ntqpixmap.h"
#include "ntqclipboard.h"
#include "ntqapplication.h"
#include "ntqtimer.h"
#include "ntqpopupmenu.h"
#include "ntqstringlist.h"
#include "ntqguardedptr.h"
#include "ntqstyle.h"
#include "ntqwhatsthis.h"
#include "secqinternal_p.h"
#include "private/qtextlayout_p.h"
#include "ntqvaluevector.h"
#if defined(QT_ACCESSIBILITY_SUPPORT)
#include "ntqaccessible.h"
#endif

#ifndef QT_NO_ACCEL
#include "ntqkeysequence.h"
#define ACCEL_KEY(k) "\t" + TQString(TQKeySequence( TQt::CTRL | TQt::Key_ ## k ))
#else
#define ACCEL_KEY(k) "\t" + TQString("Ctrl+" #k)
#endif

#define innerMargin 1

struct SecTQLineEditPrivate : public TQt
{
    SecTQLineEditPrivate( SecTQLineEdit *q )
	: q(q), cursor(0), cursorTimer(0), tripleClickTimer(0), frame(1),
	  cursorVisible(0), separator(0), readOnly(0), modified(0),
	  direction(TQChar::DirON), alignment(0),
	  echoMode(0), textDirty(0), selDirty(0),
	  ascent(0), maxLength(32767), menuId(0),
	  hscroll(0),
	  undoState(0), selstart(0), selend(0),
	  imstart(0), imend(0), imselstart(0), imselend(0)
	{}
    void init( const SecTQString&);

    SecTQLineEdit *q;
    SecTQString text;
    int cursor;
    int cursorTimer;
    TQPoint tripleClick;
    int tripleClickTimer;
    uint frame : 1;
    uint cursorVisible : 1;
    uint separator : 1;
    uint readOnly : 1;
    uint modified : 1;
    uint direction : 5;
    uint alignment : 3;
    uint echoMode : 2;
    uint textDirty : 1;
    uint selDirty : 1;
    int ascent;
    int maxLength;
    int menuId;
    int hscroll;

    void finishChange( int validateFromState = -1, bool setModified = TRUE );

    void setCursorVisible( bool visible );


    // undo/redo handling
    enum CommandType { Separator, Insert, Remove, Delete, RemoveSelection, DeleteSelection };
    struct Command {
	inline Command(){}
	inline Command( CommandType type, int pos, TQChar c )
	    :type(type),c(c),pos(pos){}
	uint type : 4;
	TQChar c;
	int pos;
    };
    int undoState;
    TQValueVector<Command> history;
#ifndef SECURE_NO_UNDO
    void addCommand( const Command& cmd );
#endif /* SECURE_NO_UNDO */

    void insert( const SecTQString& s );
    void del( bool wasBackspace = FALSE );
    void remove( int pos );

    inline void separate() { separator = TRUE; }
#ifndef SECURE_NO_UNDO
    inline void undo( int until = -1 ) {
	if ( !isUndoAvailable() )
	    return;
	deselect();
	while ( undoState && undoState > until ) {
	    Command& cmd = history[--undoState];
	    switch ( cmd.type ) {
	    case Insert:
		text.remove( cmd.pos, 1);
		cursor = cmd.pos;
		break;
	    case Remove:
	    case RemoveSelection:
		text.insert( cmd.pos, cmd.c );
		cursor = cmd.pos + 1;
		break;
	    case Delete:
	    case DeleteSelection:
		text.insert( cmd.pos, cmd.c );
		cursor = cmd.pos;
		break;
	    case Separator:
		continue;
	    }
	    if ( until < 0 && undoState ) {
		Command& next = history[undoState-1];
		if ( next.type != cmd.type && next.type < RemoveSelection
		     && !( cmd.type >= RemoveSelection && next.type != Separator ) )
		    break;
	    }
	}
	modified = ( undoState != 0 );
	textDirty = TRUE;
    }
    inline void redo() {
	if ( !isRedoAvailable() )
	    return;
	deselect();
	while ( undoState < (int)history.size() ) {
	    Command& cmd = history[undoState++];
	    switch ( cmd.type ) {
	    case Insert:
		text.insert( cmd.pos, cmd.c );
		cursor = cmd.pos + 1;
		break;
	    case Remove:
	    case Delete:
	    case RemoveSelection:
	    case DeleteSelection:
		text.remove( cmd.pos, 1 );
		cursor = cmd.pos;
		break;
	    case Separator:
		continue;
	    }
	    if ( undoState < (int)history.size() ) {
		Command& next = history[undoState];
		if ( next.type != cmd.type && cmd.type < RemoveSelection
		     && !( next.type >= RemoveSelection && cmd.type != Separator ) )
		    break;
	    }
	}
	textDirty = TRUE;
    }
#endif  /* SECURE_NO_UNDO */
    inline bool isUndoAvailable() const { return !readOnly && undoState; }
    inline bool isRedoAvailable() const { return !readOnly && undoState < (int)history.size(); }


    // bidi
    inline bool isRightToLeft() const { return direction==TQChar::DirON?text.isRightToLeft():(direction==TQChar::DirR); }

    // selection
    int selstart, selend;
    inline bool allSelected() const { return !text.isEmpty() && selstart == 0 && selend == (int)text.length(); }
    inline bool hasSelectedText() const { return !text.isEmpty() && selend > selstart; }
    inline void deselect() { selDirty |= (selend > selstart); selstart = selend = 0; }
    void removeSelectedText();
#ifndef QT_NO_CLIPBOARD
    void copy( bool clipboard = TRUE ) const;
#endif
    inline bool inSelection( int x ) const
    { if ( selstart >= selend ) return FALSE;
    int pos = xToPos( x, TQTextItem::OnCharacters );  return pos >= selstart && pos < selend; }

    // input methods
    int imstart, imend, imselstart, imselend;

    // complex text layout
    TQTextLayout textLayout;
    void updateTextLayout();
    void moveCursor( int pos, bool mark = FALSE );
    void setText( const SecTQString& txt );
    int xToPos( int x, TQTextItem::CursorPosition = TQTextItem::BetweenCharacters ) const;
    inline int visualAlignment() const { return alignment ? alignment : int( isRightToLeft() ? AlignRight : AlignLeft ); }
    TQRect cursorRect() const;
    void updateMicroFocusHint();

};


/*!
    \class SecTQLineEdit
    \brief The SecTQLineEdit widget is a one-line text editor.

    \ingroup basic
    \mainclass

    A line edit allows the user to enter and edit a single line of
    plain text with a useful collection of editing functions,
    including undo and redo, cut and paste,

    By changing the echoMode() of a line edit, it can also be used as
    a "write-only" field, for inputs such as passwords.

    The length of the text can be constrained to maxLength().

    A related class is TQTextEdit which allows multi-line, rich-text
    editing.

    You can change the text with setText() or insert(). The text is
    retrieved with text(); the displayed text (which may be different,
    see \l{EchoMode}) is retrieved with displayText(). Text can be
    selected with setSelection() or selectAll(), and the selection can
    be cut(), copy()ied and paste()d. The text can be aligned with
    setAlignment().

    When the text changes the textChanged() signal is emitted; when
    the Return or Enter key is pressed the returnPressed() signal is
    emitted.

    When the text changes the textModified() signal is emitted in all
    cases.

    By default, SecTQLineEdits have a frame as specified by the Windows
    and Motif style guides; you can turn it off by calling
    setFrame(FALSE).

    The default key bindings are described below.
    \target desc
    \table
    \header \i Keypress \i Action
    \row \i Left Arrow \i Moves the cursor one character to the left.
    \row \i Shift+Left Arrow \i Moves and selects text one character to the left.
    \row \i Right Arrow \i Moves the cursor one character to the right.
    \row \i Shift+Right Arrow \i Moves and selects text one character to the right.
    \row \i Home \i Moves the cursor to the beginning of the line.
    \row \i End \i Moves the cursor to the end of the line.
    \row \i Backspace \i Deletes the character to the left of the cursor.
    \row \i Ctrl+Backspace \i Deletes the word to the left of the cursor.
    \row \i Delete \i Deletes the character to the right of the cursor.
    \row \i Ctrl+Delete \i Deletes the word to the right of the cursor.
    \row \i Ctrl+A \i Moves the cursor to the beginning of the line.
    \row \i Ctrl+B \i Moves the cursor one character to the left.
    \row \i Ctrl+C \i Copies the selected text to the clipboard.
		      (Windows also supports Ctrl+Insert for this operation.)
    \row \i Ctrl+D \i Deletes the character to the right of the cursor.
    \row \i Ctrl+E \i Moves the cursor to the end of the line.
    \row \i Ctrl+F \i Moves the cursor one character to the right.
    \row \i Ctrl+H \i Deletes the character to the left of the cursor.
    \row \i Ctrl+K \i Deletes to the end of the line.
    \row \i Ctrl+V \i Pastes the clipboard text into line edit.
		      (Windows also supports Shift+Insert for this operation.)
    \row \i Ctrl+X \i Deletes the selected text and copies it to the clipboard.
		      (Windows also supports Shift+Delete for this operation.)
    \row \i Ctrl+Z \i Undoes the last operation.
    \row \i Ctrl+Y \i Redoes the last undone operation.
    \endtable

    Any other key sequence that represents a valid character, will
    cause the character to be inserted into the line edit.

    <img src=qlined-m.png> <img src=qlined-w.png>

    \sa TQTextEdit TQLabel TQComboBox
	\link guibooks.html#fowler GUI Design Handbook: Field, Entry\endlink
*/


/*!
    \fn void SecTQLineEdit::textChanged( const SecTQString& )

    This signal is emitted whenever the text changes. The argument is
    the new text.
*/

/*!
    \fn void SecTQLineEdit::selectionChanged()

    This signal is emitted whenever the selection changes.

    \sa hasSelectedText(), selectedText()
*/

/*!
    \fn void SecTQLineEdit::lostFocus()

    This signal is emitted when the line edit has lost focus.

    \sa hasFocus(), TQWidget::focusInEvent(), TQWidget::focusOutEvent()
*/



/*!
    Constructs a line edit with no text.

    The maximum text length is set to 32767 characters.

    The \a parent and \a name arguments are sent to the TQWidget constructor.

    \sa setText(), setMaxLength()
*/

SecTQLineEdit::SecTQLineEdit( TQWidget* parent, const char* name )
    : TQFrame( parent, name, WNoAutoErase ), d(new SecTQLineEditPrivate( this ))
{
    d->init( SecTQString::null );
}

/*!
    Constructs a line edit containing the text \a contents.

    The cursor position is set to the end of the line and the maximum
    text length to 32767 characters.

    The \a parent and \a name arguments are sent to the TQWidget
    constructor.

    \sa text(), setMaxLength()
*/

SecTQLineEdit::SecTQLineEdit( const SecTQString& contents, TQWidget* parent, const char* name )
    : TQFrame( parent, name, WNoAutoErase ), d(new SecTQLineEditPrivate( this ))
{
    d->init( contents );
}

/*!
    Destroys the line edit.
*/

SecTQLineEdit::~SecTQLineEdit()
{
    delete d;
}


/*!
    \property SecTQLineEdit::text
    \brief the line edit's text

    Setting this property clears the selection, clears the undo/redo
    history, moves the cursor to the end of the line and resets the
    \c modified property to FALSE.

    The text is truncated to maxLength() length.

    \sa insert()
*/
SecTQString SecTQLineEdit::text() const
{
    return ( d->text.isNull() ? SecTQString ("") : d->text );
}

void SecTQLineEdit::setText( const SecTQString& text)
{
    resetInputContext();
    d->setText( text );
    d->modified = FALSE;
    d->finishChange( -1, FALSE );
}


/*!
    \property SecTQLineEdit::displayText
    \brief the displayed text

    If \c EchoMode is \c Normal this returns the same as text(); if
    \c EchoMode is \c Password it returns a string of asterisks
    text().length() characters long, e.g. "******"; if \c EchoMode is
    \c NoEcho returns an empty string, "".

    \sa setEchoMode() text() EchoMode
*/

TQString SecTQLineEdit::displayText() const
{
    if ( d->echoMode == NoEcho )
	return TQString::fromLatin1("");

    TQChar pwd_char = TQChar (style().styleHint( TQStyle::SH_LineEdit_PasswordCharacter, this));
    TQString res;
    res.fill (pwd_char, d->text.length ());
    return res;
}


/*!
    \property SecTQLineEdit::maxLength
    \brief the maximum permitted length of the text

    If the text is too long, it is truncated at the limit.

    If truncation occurs any selected text will be unselected, the
    cursor position is set to 0 and the first part of the string is
    shown.
*/

int SecTQLineEdit::maxLength() const
{
    return d->maxLength;
}

void SecTQLineEdit::setMaxLength( int maxLength )
{
    d->maxLength = maxLength;
    setText( d->text );
}



/*!
    \property SecTQLineEdit::frame
    \brief whether the line edit draws itself with a frame

    If enabled (the default) the line edit draws itself inside a
    two-pixel frame, otherwise the line edit draws itself without any
    frame.
*/
bool SecTQLineEdit::frame() const
{
    return frameShape() != NoFrame;
}


void SecTQLineEdit::setFrame( bool enable )
{
    setFrameStyle( enable ? ( LineEditPanel | Sunken ) : NoFrame  );
}


/*!
    \enum SecTQLineEdit::EchoMode

    This enum type describes how a line edit should display its
    contents.

    \value Normal   Display characters as they are entered. This is the
		    default.
    \value NoEcho   Do not display anything. This may be appropriate
		    for passwords where even the length of the
		    password should be kept secret.
    \value Password  Display asterisks instead of the characters
		    actually entered.

    \sa setEchoMode() echoMode()
*/


/*!
    \property SecTQLineEdit::echoMode
    \brief the line edit's echo mode

    The initial setting is \c Normal, but SecTQLineEdit also supports \c
    NoEcho and \c Password modes.

    The widget's display and the ability to copy the text is affected
    by this setting.

    \sa EchoMode displayText()
*/

SecTQLineEdit::EchoMode SecTQLineEdit::echoMode() const
{
    return (EchoMode) d->echoMode;
}

void SecTQLineEdit::setEchoMode( EchoMode mode )
{
    d->echoMode = mode;
    d->updateTextLayout();
    update();
}



/*!
    Returns a recommended size for the widget.

    The width returned, in pixels, is usually enough for about 15 to
    20 characters.
*/

TQSize SecTQLineEdit::sizeHint() const
{
    constPolish();
    TQFontMetrics fm( font() );
    int h = TQMAX(fm.lineSpacing(), 14) + 2*innerMargin;
    int w = fm.width( 'x' ) * 17; // "some"
    int m = frameWidth() * 2;
    return (style().sizeFromContents(TQStyle::CT_LineEdit, this,
				     TQSize( w + m, h + m ).
				     expandedTo(TQApplication::globalStrut())));
}


/*!
    Returns a minimum size for the line edit.

    The width returned is enough for at least one character.
*/

TQSize SecTQLineEdit::minimumSizeHint() const
{
    constPolish();
    TQFontMetrics fm = fontMetrics();
    int h = fm.height() + TQMAX( 2*innerMargin, fm.leading() );
    int w = fm.maxWidth();
    int m = frameWidth() * 2;
    return TQSize( w + m, h + m );
}


/*!
    \property SecTQLineEdit::cursorPosition
    \brief the current cursor position for this line edit

    Setting the cursor position causes a repaint when appropriate.
*/

int SecTQLineEdit::cursorPosition() const
{
    return d->cursor;
}


void SecTQLineEdit::setCursorPosition( int pos )
{
    if ( pos <= (int) d->text.length() )
	d->moveCursor( pos );
}


/*!
    \property SecTQLineEdit::alignment
    \brief the alignment of the line edit

    Possible Values are \c TQt::AlignAuto, \c TQt::AlignLeft, \c
    TQt::AlignRight and \c TQt::AlignHCenter.

    Attempting to set the alignment to an illegal flag combination
    does nothing.

    \sa TQt::AlignmentFlags
*/

int SecTQLineEdit::alignment() const
{
    return d->alignment;
}

void SecTQLineEdit::setAlignment( int flag )
{
    d->alignment = flag & 0x7;
    update();
}


/*!
  \obsolete
  \fn void SecTQLineEdit::cursorRight( bool, int )

  Use cursorForward() instead.

  \sa cursorForward()
*/

/*!
  \obsolete
  \fn void SecTQLineEdit::cursorLeft( bool, int )
  For compatibilty with older applications only. Use cursorBackward()
  instead.
  \sa cursorBackward()
*/

/*!
    Moves the cursor forward \a steps characters. If \a mark is TRUE
    each character moved over is added to the selection; if \a mark is
    FALSE the selection is cleared.

    \sa cursorBackward()
*/

void SecTQLineEdit::cursorForward( bool mark, int steps )
{
    int cursor = d->cursor;
    if ( steps > 0 ) {
	while( steps-- )
	    cursor = d->textLayout.nextCursorPosition( cursor );
    } else if ( steps < 0 ) {
	while ( steps++ )
	    cursor = d->textLayout.previousCursorPosition( cursor );
    }
    d->moveCursor( cursor, mark );
}


/*!
    Moves the cursor back \a steps characters. If \a mark is TRUE each
    character moved over is added to the selection; if \a mark is
    FALSE the selection is cleared.

    \sa cursorForward()
*/
void SecTQLineEdit::cursorBackward( bool mark, int steps )
{
    cursorForward( mark, -steps );
}

/*!
    Moves the cursor one word forward. If \a mark is TRUE, the word is
    also selected.

    \sa cursorWordBackward()
*/
void SecTQLineEdit::cursorWordForward( bool mark )
{
    d->moveCursor( d->textLayout.nextCursorPosition(d->cursor, TQTextLayout::SkipWords), mark );
}

/*!
    Moves the cursor one word backward. If \a mark is TRUE, the word
    is also selected.

    \sa cursorWordForward()
*/

void SecTQLineEdit::cursorWordBackward( bool mark )
{
    d->moveCursor( d->textLayout.previousCursorPosition(d->cursor, TQTextLayout::SkipWords), mark );
}


/*!
    If no text is selected, deletes the character to the left of the
    text cursor and moves the cursor one position to the left. If any
    text is selected, the cursor is moved to the beginning of the
    selected text and the selected text is deleted.

    \sa del()
*/
void SecTQLineEdit::backspace()
{
    int priorState = d->undoState;
    if ( d->hasSelectedText() ) {
	d->removeSelectedText();
    } else if ( d->cursor ) {
	    --d->cursor;
	    d->del( TRUE );
    }
    d->finishChange( priorState );
}

/*!
    If no text is selected, deletes the character to the right of the
    text cursor. If any text is selected, the cursor is moved to the
    beginning of the selected text and the selected text is deleted.

    \sa backspace()
*/

void SecTQLineEdit::del()
{
    int priorState = d->undoState;
    if ( d->hasSelectedText() ) {
	d->removeSelectedText();
    } else {
	int n = d->textLayout.nextCursorPosition( d->cursor ) - d->cursor;
	while ( n-- )
	    d->del();
    }
    d->finishChange( priorState );
}

/*!
    Moves the text cursor to the beginning of the line unless it is
    already there. If \a mark is TRUE, text is selected towards the
    first position; otherwise, any selected text is unselected if the
    cursor is moved.

    \sa end()
*/

void SecTQLineEdit::home( bool mark )
{
    d->moveCursor( 0, mark );
}

/*!
    Moves the text cursor to the end of the line unless it is already
    there. If \a mark is TRUE, text is selected towards the last
    position; otherwise, any selected text is unselected if the cursor
    is moved.

    \sa home()
*/

void SecTQLineEdit::end( bool mark )
{
    d->moveCursor( d->text.length(), mark );
}


/*!
    \property SecTQLineEdit::modified
    \brief whether the line edit's contents has been modified by the user

    The modified flag is never read by SecTQLineEdit; it has a default value
    of FALSE and is changed to TRUE whenever the user changes the line
    edit's contents.

    This is useful for things that need to provide a default value but
    do not start out knowing what the default should be (perhaps it
    depends on other fields on the form). Start the line edit without
    the best default, and when the default is known, if modified()
    returns FALSE (the user hasn't entered any text), insert the
    default value.

    Calling clearModified() or setText() resets the modified flag to
    FALSE.
*/

bool SecTQLineEdit::isModified() const
{
    return d->modified;
}

/*!
    Resets the modified flag to FALSE.

    \sa isModified()
*/
void SecTQLineEdit::clearModified()
{
    d->modified = FALSE;
    d->history.clear();
    d->undoState = 0;
}

/*!
  \obsolete
  \property SecTQLineEdit::edited
  \brief whether the line edit has been edited. Use modified instead.
*/
bool SecTQLineEdit::edited() const { return d->modified; }
void SecTQLineEdit::setEdited( bool on ) { d->modified = on; }

/*!
    \obsolete
    \property SecTQLineEdit::hasMarkedText
    \brief whether part of the text has been selected by the user. Use hasSelectedText instead.
*/

/*!
    \property SecTQLineEdit::hasSelectedText
    \brief whether there is any text selected

    hasSelectedText() returns TRUE if some or all of the text has been
    selected by the user; otherwise returns FALSE.

    \sa selectedText()
*/


bool SecTQLineEdit::hasSelectedText() const
{
    return d->hasSelectedText();
}

/*!
  \obsolete
  \property SecTQLineEdit::markedText
  \brief the text selected by the user. Use selectedText instead.
*/

/*!
    \property SecTQLineEdit::selectedText
    \brief the selected text

    If there is no selected text this property's value is
    TQString::null.

    \sa hasSelectedText()
*/

SecTQString SecTQLineEdit::selectedText() const
{
    if ( d->hasSelectedText() )
	return d->text.mid( d->selstart, d->selend - d->selstart );
    return SecTQString::null;
}

/*!
    selectionStart() returns the index of the first selected character in the
    line edit or -1 if no text is selected.

    \sa selectedText()
*/

int SecTQLineEdit::selectionStart() const
{
    return d->hasSelectedText() ? d->selstart : -1;
}


/*!
    Selects text from position \a start and for \a length characters.

    \sa deselect() selectAll()
*/

void SecTQLineEdit::setSelection( int start, int length )
{
    if ( start < 0 || start > (int)d->text.length() || length < 0 ) {
	d->selstart = d->selend = 0;
    } else {
	d->selstart = start;
	d->selend = TQMIN( start + length, (int)d->text.length() );
	d->cursor = d->selend;
    }
    update();
}


/*!
    \property SecTQLineEdit::undoAvailable
    \brief whether undo is available
*/

bool SecTQLineEdit::isUndoAvailable() const
{
    return d->isUndoAvailable();
}

/*!
    \property SecTQLineEdit::redoAvailable
    \brief whether redo is available
*/

bool SecTQLineEdit::isRedoAvailable() const
{
    return d->isRedoAvailable();
}

/*!
    Selects all the text (i.e. highlights it) and moves the cursor to
    the end. This is useful when a default value has been inserted
    because if the user types before clicking on the widget, the
    selected text will be deleted.

    \sa setSelection() deselect()
*/

void SecTQLineEdit::selectAll()
{
    d->selstart = d->selend = d->cursor = 0;
    d->moveCursor( d->text.length(), TRUE );
}

/*!
    Deselects any selected text.

    \sa setSelection() selectAll()
*/

void SecTQLineEdit::deselect()
{
    d->deselect();
    d->finishChange();
}


/*!
    Deletes any selected text, inserts \a newText and sets it as the
    new contents of the line edit.
*/
void SecTQLineEdit::insert( const SecTQString &newText )
{
//     q->resetInputContext(); //#### FIX ME IN QT
    int priorState = d->undoState;
    d->removeSelectedText();
    d->insert( newText );
    d->finishChange( priorState );
}

/*!
    Clears the contents of the line edit.
*/
void SecTQLineEdit::clear()
{
    int priorState = d->undoState;
    resetInputContext();
    d->selstart = 0;
    d->selend = d->text.length();
    d->removeSelectedText();
    d->separate();
    d->finishChange( priorState );
}

/*!
    Undoes the last operation if undo is \link
    SecTQLineEdit::undoAvailable available\endlink. Deselects any current
    selection, and updates the selection start to the current cursor
    position.
*/
void SecTQLineEdit::undo()
{
#ifndef SECURE_NO_UNDO
    resetInputContext();
    d->undo();
    d->finishChange( -1, FALSE );
#endif
}

/*!
    Redoes the last operation if redo is \link
    SecTQLineEdit::redoAvailable available\endlink.
*/
void SecTQLineEdit::redo()
{
#ifndef SECURE_NO_UNDO
    resetInputContext();
    d->redo();
    d->finishChange();
#endif
}


/*!
    \property SecTQLineEdit::readOnly
    \brief whether the line edit is read only.

    In read-only mode, the user can still copy the text to the
    clipboard (if echoMode() is \c Normal), but cannot edit it.

    SecTQLineEdit does not show a cursor in read-only mode.

    \sa setEnabled()
*/

bool SecTQLineEdit::isReadOnly() const
{
    return d->readOnly;
}

void SecTQLineEdit::setReadOnly( bool enable )
{
    d->readOnly = enable;
#ifndef QT_NO_CURSOR
    setCursor( enable ? arrowCursor : ibeamCursor );
#endif
    update();
}


#ifndef QT_NO_CLIPBOARD
/*!
    Copies the selected text to the clipboard and deletes it, if there
    is any, and if echoMode() is \c Normal.

    \sa copy() paste() setValidator()
*/

void SecTQLineEdit::cut()
{
    if ( hasSelectedText() ) {
	copy();
	del();
    }
}


/*!
    Copies the selected text to the clipboard, if there is any, and if
    echoMode() is \c Normal.

    \sa cut() paste()
*/

void SecTQLineEdit::copy() const
{
    d->copy();
}

/*!
    Inserts the clipboard's text at the cursor position, deleting any
    selected text, providing the line edit is not \link
    SecTQLineEdit::readOnly read-only\endlink.

    \sa copy() cut()
*/

void SecTQLineEdit::paste()
{
    d->removeSelectedText();
    insert( TQApplication::clipboard()->text( TQClipboard::Clipboard ) );
}

void SecTQLineEditPrivate::copy( bool clipboard ) const
{
#ifndef SECURE
    TQString t = q->selectedText();
    if ( !t.isEmpty() && echoMode == SecTQLineEdit::Normal ) {
	q->disconnect( TQApplication::clipboard(), SIGNAL(selectionChanged()), q, 0);
	TQApplication::clipboard()->setText( t, clipboard ? TQClipboard::Clipboard : TQClipboard::Selection );
	q->connect( TQApplication::clipboard(), SIGNAL(selectionChanged()),
		 q, SLOT(clipboardChanged()) );
    }
#endif
}

#endif // !QT_NO_CLIPBOARD

/*!\reimp
*/

void SecTQLineEdit::resizeEvent( TQResizeEvent *e )
{
    TQFrame::resizeEvent( e );
}

/*! \reimp
*/
bool SecTQLineEdit::event( TQEvent * e )
{
    if ( e->type() == TQEvent::AccelOverride && !d->readOnly ) {
	TQKeyEvent* ke = (TQKeyEvent*) e;
	if ( ke->state() == NoButton || ke->state() == ShiftButton
	     || ke->state() == Keypad ) {
	    if ( ke->key() < Key_Escape ) {
		ke->accept();
	    } else if ( ke->state() == NoButton
			|| ke->state() == ShiftButton ) {
		switch ( ke->key() ) {
		case Key_Delete:
		case Key_Home:
		case Key_End:
		case Key_Backspace:
		case Key_Left:
		case Key_Right:
		    ke->accept();
		default:
		    break;
		}
	    }
	} else if ( ke->state() & ControlButton ) {
	    switch ( ke->key() ) {
// Those are too frequently used for application functionality
/*	    case Key_A:
	    case Key_B:
	    case Key_D:
	    case Key_E:
	    case Key_F:
	    case Key_H:
	    case Key_K:
*/
	    case Key_C:
	    case Key_V:
	    case Key_X:
	    case Key_Y:
	    case Key_Z:
	    case Key_Left:
	    case Key_Right:
#if defined (Q_WS_WIN)
	    case Key_Insert:
	    case Key_Delete:
#endif
		ke->accept();
	    default:
		break;
	    }
	}
    } else if ( e->type() == TQEvent::Timer ) {
	// should be timerEvent, is here for binary compatibility
	int timerId = ((TQTimerEvent*)e)->timerId();
	if ( timerId == d->cursorTimer ) {
	    if(!hasSelectedText() || style().styleHint( TQStyle::SH_BlinkCursorWhenTextSelected ))
		d->setCursorVisible( !d->cursorVisible );
	} else if ( timerId == d->tripleClickTimer ) {
	    killTimer( d->tripleClickTimer );
	    d->tripleClickTimer = 0;
	}
    }
    return TQWidget::event( e );
}

/*! \reimp
*/
void SecTQLineEdit::mousePressEvent( TQMouseEvent* e )
{
    if ( e->button() == RightButton )
	return;
    if ( d->tripleClickTimer && ( e->pos() - d->tripleClick ).manhattanLength() <
	 TQApplication::startDragDistance() ) {
	selectAll();
	return;
    }
    bool mark = e->state() & ShiftButton;
    int cursor = d->xToPos( e->pos().x() );
    d->moveCursor( cursor, mark );
}

/*! \reimp
*/
void SecTQLineEdit::mouseMoveEvent( TQMouseEvent * e )
{

#ifndef QT_NO_CURSOR
    if ( ( e->state() & MouseButtonMask ) == 0 ) {
	if ( !d->readOnly )
	  setCursor( ( d->inSelection( e->pos().x() ) ? arrowCursor : ibeamCursor ) );
    }
#endif

    if ( e->state() & LeftButton ) {
      d->moveCursor( d->xToPos( e->pos().x() ), TRUE );
    }
}

/*! \reimp
*/
void SecTQLineEdit::mouseReleaseEvent( TQMouseEvent* e )
{
#ifndef QT_NO_CLIPBOARD
    if (TQApplication::clipboard()->supportsSelection() ) {
	if ( e->button() == LeftButton ) {
	    d->copy( FALSE );
	} else if ( !d->readOnly && e->button() == MidButton ) {
	    d->deselect();
	    insert( TQApplication::clipboard()->text( TQClipboard::Selection ) );
	}
    }
#endif
}

/*! \reimp
*/
void SecTQLineEdit::mouseDoubleClickEvent( TQMouseEvent* e )
{
    if ( e->button() == TQt::LeftButton ) {
	deselect();
	d->cursor = d->xToPos( e->pos().x() );
	d->cursor = d->textLayout.previousCursorPosition( d->cursor, TQTextLayout::SkipWords );
	// ## text layout should support end of words.
	int end = d->textLayout.nextCursorPosition( d->cursor, TQTextLayout::SkipWords );
	while ( end > d->cursor && d->text[end-1].isSpace() )
	    --end;
	d->moveCursor( end, TRUE );
	d->tripleClickTimer = startTimer( TQApplication::doubleClickInterval() );
	d->tripleClick = e->pos();
    }
}

/*!
    \fn void  SecTQLineEdit::returnPressed()

    This signal is emitted when the Return or Enter key is pressed.
*/

/*!
    Converts key press event \a e into a line edit action.

    If Return or Enter is pressed the signal returnPressed() is
    emitted.

    The default key bindings are listed in the \link #desc detailed
    description.\endlink
*/

void SecTQLineEdit::keyPressEvent( TQKeyEvent * e )
{
    d->setCursorVisible( TRUE );
    if ( e->key() == Key_Enter || e->key() == Key_Return ) {
	emit returnPressed();
	e->ignore();
	return;
    }
    if ( !d->readOnly ) {
	TQString t = e->text();
	if ( !t.isEmpty() && (!e->ascii() || e->ascii()>=32) &&
	     e->key() != Key_Delete &&
	     e->key() != Key_Backspace ) {
#ifdef Q_WS_X11
	    extern bool tqt_hebrew_keyboard_hack;
	    if ( tqt_hebrew_keyboard_hack ) {
		// the X11 keyboard layout is broken and does not reverse
		// braces correctly. This is a hack to get halfway correct
		// behaviour
		if ( d->isRightToLeft() ) {
		    TQChar *c = (TQChar *)t.unicode();
		    int l = t.length();
		    while( l-- ) {
			if ( c->mirrored() )
			    *c = c->mirroredChar();
			c++;
		    }
		}
	    }
#endif
	    insert( t );
	    return;
	}
    }
    bool unknown = FALSE;
    if ( e->state() & ControlButton ) {
	switch ( e->key() ) {
	case Key_A:
#if defined(Q_WS_X11)
	    home( e->state() & ShiftButton );
#else
	    selectAll();
#endif
	    break;
	case Key_B:
	    cursorForward( e->state() & ShiftButton, -1 );
	    break;
#ifndef QT_NO_CLIPBOARD
	case Key_C:
	    copy();
	    break;
#endif
	case Key_D:
	    if ( !d->readOnly ) {
		del();
	    }
	    break;
	case Key_E:
	    end( e->state() & ShiftButton );
	    break;
	case Key_F:
	    cursorForward( e->state() & ShiftButton, 1 );
	    break;
	case Key_H:
	    if ( !d->readOnly ) {
		backspace();
	    }
	    break;
	case Key_K:
	    if ( !d->readOnly ) {
		int priorState = d->undoState;
		d->deselect();
		while ( d->cursor < (int) d->text.length() )
		    d->del();
		d->finishChange( priorState );
	    }
	    break;
#if defined(Q_WS_X11)
        case Key_U:
	    if ( !d->readOnly )
		clear();
	    break;
#endif
#ifndef QT_NO_CLIPBOARD
	case Key_V:
	    if ( !d->readOnly )
		paste();
	    break;
	case Key_X:
	    if ( !d->readOnly && d->hasSelectedText() && echoMode() == Normal ) {
		copy();
		del();
	    }
	    break;
#if defined (Q_WS_WIN)
	case Key_Insert:
	    copy();
	    break;
#endif
#endif
	case Key_Delete:
	    if ( !d->readOnly ) {
		cursorWordForward( TRUE );
		del();
	    }
	    break;
	case Key_Backspace:
	    if ( !d->readOnly ) {
		cursorWordBackward( TRUE );
		del();
	    }
	    break;
	case Key_Right:
	case Key_Left:
	    if ( d->isRightToLeft() == (e->key() == Key_Right) ) {
	        if ( echoMode() == Normal )
		    cursorWordBackward( e->state() & ShiftButton );
		else
		    home( e->state() & ShiftButton );
	    } else {
		if ( echoMode() == Normal )
		    cursorWordForward( e->state() & ShiftButton );
		else
		    end( e->state() & ShiftButton );
	    }
	    break;
	case Key_Z:
	    if ( !d->readOnly ) {
		if(e->state() & ShiftButton)
		    redo();
		else
		    undo();
	    }
	    break;
	case Key_Y:
	    if ( !d->readOnly )
		redo();
	    break;
	default:
	    unknown = TRUE;
	}
    } else { // ### check for *no* modifier
	switch ( e->key() ) {
	case Key_Shift:
	    // ### TODO
	    break;
	case Key_Left:
	case Key_Right: {
	    int step =  (d->isRightToLeft() == (e->key() == Key_Right)) ? -1 : 1;
	    cursorForward( e->state() & ShiftButton, step );
	}
	break;
	case Key_Backspace:
	    if ( !d->readOnly ) {
		backspace();
	    }
	    break;
	case Key_Home:
#ifdef Q_WS_MACX
	case Key_Up:
#endif
	    home( e->state() & ShiftButton );
	    break;
	case Key_End:
#ifdef Q_WS_MACX
	case Key_Down:
#endif
	    end( e->state() & ShiftButton );
	    break;
	case Key_Delete:
	    if ( !d->readOnly ) {
#if defined (Q_WS_WIN)
		if ( e->state() & ShiftButton ) {
		    cut();
		    break;
		}
#endif
		del();
	    }
	    break;
#if defined (Q_WS_WIN)
	case Key_Insert:
	    if ( !d->readOnly && e->state() & ShiftButton )
		paste();
	    else
		unknown = TRUE;
	    break;
#endif
	case Key_F14: // Undo key on Sun keyboards
	    if ( !d->readOnly )
		undo();
	    break;
#ifndef QT_NO_CLIPBOARD
	case Key_F16: // Copy key on Sun keyboards
	    copy();
	    break;
	case Key_F18: // Paste key on Sun keyboards
	    if ( !d->readOnly )
		paste();
	    break;
	case Key_F20: // Cut key on Sun keyboards
	    if ( !d->readOnly && hasSelectedText() && echoMode() == Normal ) {
		copy();
		del();
	    }
	    break;
#endif
	default:
	    unknown = TRUE;
	}
    }
    if ( e->key() == Key_Direction_L )
	d->direction = TQChar::DirL;
    else if ( e->key() == Key_Direction_R )
	d->direction = TQChar::DirR;

    if ( unknown )
	e->ignore();
}

/*! \reimp
 */
void SecTQLineEdit::imStartEvent( TQIMEvent *e )
{
    if ( d->readOnly ) {
	e->ignore();
	return;
    }
    d->removeSelectedText();
    d->updateMicroFocusHint();
    d->imstart = d->imend = d->imselstart = d->imselend = d->cursor;
}

/*! \reimp
 */
void SecTQLineEdit::imComposeEvent( TQIMEvent *e )
{
    if ( d->readOnly ) {
	e->ignore();
    } else {
	d->text.replace( d->imstart, d->imend - d->imstart, e->text() );
	d->imend = d->imstart + e->text().length();
	d->imselstart = d->imstart + e->cursorPos();
	d->imselend = d->imselstart + e->selectionLength();
	d->cursor = e->selectionLength() ? d->imend : d->imselend;
	d->updateTextLayout();
	update();
    }
}

/*! \reimp
 */
void SecTQLineEdit::imEndEvent( TQIMEvent *e )
{
    if ( d->readOnly ) {
	e->ignore();
    } else {
	d->text.remove( d->imstart, d->imend - d->imstart );
	d->cursor = d->imselstart = d->imselend = d->imend = d->imstart;
	d->textDirty = TRUE;
	insert( e->text() );
    }
}

/*!\reimp
*/

void SecTQLineEdit::focusInEvent( TQFocusEvent* e )
{
    if ( e->reason() == TQFocusEvent::Tab ||
	 e->reason() == TQFocusEvent::Backtab  ||
	 e->reason() == TQFocusEvent::Shortcut )
	selectAll();
    if ( !d->cursorTimer ) {
	int cft = TQApplication::cursorFlashTime();
	d->cursorTimer = cft ? startTimer( cft/2 ) : -1;
    }
    d->updateMicroFocusHint();
}

/*!\reimp
*/

void SecTQLineEdit::focusOutEvent( TQFocusEvent* e )
{
    if ( e->reason() != TQFocusEvent::ActiveWindow &&
	 e->reason() != TQFocusEvent::Popup )
	deselect();
    d->setCursorVisible( FALSE );
    if ( d->cursorTimer > 0 )
	killTimer( d->cursorTimer );
    d->cursorTimer = 0;
    emit lostFocus();
}

/*!\reimp
*/
void SecTQLineEdit::drawContents( TQPainter *p )
{
    const TQColorGroup& cg = colorGroup();
    TQRect cr = contentsRect();
    TQFontMetrics fm = fontMetrics();
    TQRect lineRect( cr.x() + innerMargin, cr.y() + (cr.height() - fm.height() + 1) / 2,
		    cr.width() - 2*innerMargin, fm.height() );
    TQBrush bg = TQBrush( paletteBackgroundColor() );
    if ( paletteBackgroundPixmap() )
	bg = TQBrush( cg.background(), *paletteBackgroundPixmap() );
    else if ( !isEnabled() )
	bg = cg.brush( TQColorGroup::Background );
    p->save();
    p->setClipRegion( TQRegion(cr) - lineRect );
    p->fillRect( cr, bg );
    p->restore();
    SecTQSharedDoubleBuffer buffer( p, lineRect.x(), lineRect.y(),
				lineRect.width(), lineRect.height(),
				hasFocus() ? SecTQSharedDoubleBuffer::Force : 0 );
    p = buffer.painter();
    p->fillRect( lineRect, bg );

    // locate cursor position
    int cix = 0;
    TQTextItem ci = d->textLayout.findItem( d->cursor );
    if ( ci.isValid() ) {
	if ( d->cursor != (int)d->text.length() && d->cursor == ci.from() + ci.length()
	     && ci.isRightToLeft() != d->isRightToLeft() )
	    ci = d->textLayout.findItem( d->cursor + 1 );
	cix = ci.x() + ci.cursorToX( d->cursor - ci.from() );
    }

    // horizontal scrolling
    int minLB = TQMAX( 0, -fm.minLeftBearing() );
    int minRB = TQMAX( 0, -fm.minRightBearing() );
    int widthUsed = d->textLayout.widthUsed() + 1 + minRB;
    if ( (minLB + widthUsed) <=  lineRect.width() ) {
	switch ( d->visualAlignment() ) {
	case AlignRight:
	    d->hscroll = widthUsed - lineRect.width();
	    break;
	case AlignHCenter:
	    d->hscroll = ( widthUsed - lineRect.width() ) / 2;
	    break;
	default:
	    d->hscroll = 0;
	    break;
	}
	d->hscroll -= minLB;
    } else if ( cix - d->hscroll >= lineRect.width() ) {
	d->hscroll = cix - lineRect.width() + 1;
    } else if ( cix - d->hscroll < 0 ) {
	d->hscroll = cix;
    } else if ( widthUsed - d->hscroll < lineRect.width() ) {
	d->hscroll = widthUsed - lineRect.width() + 1;
    }
    // the y offset is there to keep the baseline constant in case we have script changes in the text.
    TQPoint topLeft = lineRect.topLeft() - TQPoint(d->hscroll, d->ascent-fm.ascent());

    // draw text, selections and cursors
    p->setPen( cg.text() );
    bool supressCursor = d->readOnly, hasRightToLeft = d->isRightToLeft();
    int textflags = 0;
    if ( font().underline() )
	textflags |= TQt::Underline;
    if ( font().strikeOut() )
	textflags |= TQt::StrikeOut;
    if ( font().overline() )
	textflags |= TQt::Overline;

    for ( int i = 0; i < d->textLayout.numItems(); i++ ) {
	TQTextItem ti = d->textLayout.itemAt( i );
	hasRightToLeft |= ti.isRightToLeft();
	int tix = topLeft.x() + ti.x();
	int first = ti.from();
	int last = ti.from() + ti.length() - 1;

	// text and selection
	if ( d->selstart < d->selend && (last >= d->selstart && first < d->selend ) ) {
	    TQRect highlight = TQRect( TQPoint( tix + ti.cursorToX( TQMAX( d->selstart - first, 0 ) ),
					     lineRect.top() ),
				     TQPoint( tix + ti.cursorToX( TQMIN( d->selend - first, last - first + 1 ) ) - 1,
					     lineRect.bottom() ) ).normalize();
	    p->save();
	    p->setClipRegion( TQRegion( lineRect ) - highlight, TQPainter::CoordPainter );
	    p->drawTextItem( topLeft, ti, textflags );
	    p->setClipRect( lineRect & highlight, TQPainter::CoordPainter );
	    p->fillRect( highlight, cg.highlight() );
	    p->setPen( cg.highlightedText() );
	    p->drawTextItem( topLeft, ti, textflags );
	    p->restore();
	} else {
	    p->drawTextItem( topLeft, ti, textflags );
	}

	// input method edit area
	if ( d->imstart < d->imend && (last >= d->imstart && first < d->imend ) ) {
	    TQRect highlight = TQRect( TQPoint( tix + ti.cursorToX( TQMAX( d->imstart - first, 0 ) ), lineRect.top() ),
			      TQPoint( tix + ti.cursorToX( TQMIN( d->imend - first, last - first + 1 ) )-1, lineRect.bottom() ) ).normalize();
	    p->save();
	    p->setClipRect( lineRect & highlight, TQPainter::CoordPainter );

	    int h1, s1, v1, h2, s2, v2;
	    cg.color( TQColorGroup::Base ).hsv( &h1, &s1, &v1 );
	    cg.color( TQColorGroup::Background ).hsv( &h2, &s2, &v2 );
	    TQColor imCol;
	    imCol.setHsv( h1, s1, ( v1 + v2 ) / 2 );
	    p->fillRect( highlight, imCol );
	    p->drawTextItem( topLeft, ti, textflags );
	    p->restore();
	}

	// input method selection
	if ( d->imselstart < d->imselend && (last >= d->imselstart && first < d->imselend ) ) {
	    TQRect highlight = TQRect( TQPoint( tix + ti.cursorToX( TQMAX( d->imselstart - first, 0 ) ), lineRect.top() ),
			      TQPoint( tix + ti.cursorToX( TQMIN( d->imselend - first, last - first + 1 ) )-1, lineRect.bottom() ) ).normalize();
	    p->save();
	    p->setClipRect( lineRect & highlight, TQPainter::CoordPainter );
	    p->fillRect( highlight, cg.text() );
	    p->setPen( paletteBackgroundColor() );
	    p->drawTextItem( topLeft, ti, textflags );
	    p->restore();
	}
    }

    // draw cursor
    if ( d->cursorVisible && !supressCursor ) {
	TQPoint from( topLeft.x() + cix, lineRect.top() );
	TQPoint to = from + TQPoint( 0, lineRect.height() );
	p->drawLine( from, to );
	if ( hasRightToLeft ) {
	    to = from + TQPoint( (ci.isRightToLeft()?-2:2), 2 );
	    p->drawLine( from, to );
	    from.ry() += 4;
	    p->drawLine( from, to );
	}
    }
    buffer.end();
}


enum { IdUndo, IdRedo, IdSep1, IdCut, IdCopy, IdPaste, IdClear, IdSep2, IdSelectAll };


/*! \reimp */
void SecTQLineEdit::windowActivationChange( bool b )
{
    //### remove me with WHighlightSelection attribute
    if ( palette().active() != palette().inactive() )
	update();
    TQWidget::windowActivationChange( b );
}

/*! \reimp */

void SecTQLineEdit::setPalette( const TQPalette & p )
{
    //### remove me with WHighlightSelection attribute
    TQWidget::setPalette( p );
    update();
}

/*!
  \obsolete
  \fn void SecTQLineEdit::repaintArea( int from, int to )
  Repaints all characters from \a from to \a to. If cursorPos is
  between from and to, ensures that cursorPos is visible.
*/

/*! \reimp
 */
void SecTQLineEdit::setFont( const TQFont & f )
{
    TQWidget::setFont( f );
    d->updateTextLayout();
}


void SecTQLineEdit::clipboardChanged()
{
}

void SecTQLineEditPrivate::init( const SecTQString& txt )
{
#ifndef QT_NO_CURSOR
    q->setCursor( readOnly ? arrowCursor : ibeamCursor );
#endif
    q->setFocusPolicy( TQWidget::StrongFocus );
    q->setInputMethodEnabled( TRUE );
    //   Specifies that this widget can use more, but is able to survive on
    //   less, horizontal space; and is fixed vertically.
    q->setSizePolicy( TQSizePolicy( TQSizePolicy::Expanding, TQSizePolicy::Fixed ) );
    q->setBackgroundMode( PaletteBase );
    q->setKeyCompression( TRUE );
    q->setMouseTracking( TRUE );
    q->setAcceptDrops( TRUE );
    q->setFrame( TRUE );
    text = txt;
    updateTextLayout();
    cursor = text.length();
}

void SecTQLineEditPrivate::updateTextLayout()
{
    // replace all non-printable characters with spaces (to avoid
    // drawing boxes when using fonts that don't have glyphs for such
    // characters)
    const TQString &displayText = q->displayText();
    TQString str(displayText.unicode(), displayText.length());
    TQChar* uc = (TQChar*)str.unicode();
    for (int i = 0; i < (int)str.length(); ++i) {
	if (! uc[i].isPrint())
	    uc[i] = TQChar(0x0020);
    }
    textLayout.setText( str, q->font() );
    // ### want to do textLayout.setRightToLeft( text.isRightToLeft() );
    textLayout.beginLayout( TQTextLayout::SingleLine );
    textLayout.beginLine( INT_MAX );
    while ( !textLayout.atEnd() )
	textLayout.addCurrentItem();
    ascent = 0;
    textLayout.endLine(0, 0, TQt::AlignLeft, &ascent);
}

int SecTQLineEditPrivate::xToPos( int x, TQTextItem::CursorPosition betweenOrOn ) const
{
    x-= q->contentsRect().x() - hscroll + innerMargin;
    for ( int i = 0; i < textLayout.numItems(); ++i ) {
	TQTextItem ti = textLayout.itemAt( i );
	TQRect tir = ti.rect();
	if ( x >= tir.left() && x <= tir.right() )
	    return ti.xToCursor( x - tir.x(), betweenOrOn ) + ti.from();
    }
    return x < 0 ? 0 : text.length();
}


TQRect SecTQLineEditPrivate::cursorRect() const
{
    TQRect cr = q->contentsRect();
    int cix = cr.x() - hscroll + innerMargin;
    TQTextItem ci = textLayout.findItem( cursor );
    if ( ci.isValid() ) {
	if ( cursor != (int)text.length() && cursor == ci.from() + ci.length()
	     && ci.isRightToLeft() != isRightToLeft() )
	    ci = textLayout.findItem( cursor + 1 );
	cix += ci.x() + ci.cursorToX( cursor - ci.from() );
    }
    int ch = q->fontMetrics().height();
    return TQRect( cix-4, cr.y() + ( cr.height() -  ch + 1) / 2, 8, ch + 1 );
}

void SecTQLineEditPrivate::updateMicroFocusHint()
{
    if ( q->hasFocus() ) {
	TQRect r = cursorRect();
	q->setMicroFocusHint( r.x(), r.y(), r.width(), r.height() );
    }
}

void SecTQLineEditPrivate::moveCursor( int pos, bool mark )
{
    if ( pos != cursor )
	separate();
    bool fullUpdate = mark || hasSelectedText();
    if ( mark ) {
	int anchor;
	if ( selend > selstart && cursor == selstart )
	    anchor = selend;
	else if ( selend > selstart && cursor == selend )
	    anchor = selstart;
	else
	    anchor = cursor;
	selstart = TQMIN( anchor, pos );
	selend = TQMAX( anchor, pos );
    } else {
	selstart = selend = 0;
    }
    if ( fullUpdate ) {
	cursor = pos;
	q->update();
    } else {
	setCursorVisible( FALSE );
	cursor = pos;
	setCursorVisible( TRUE );
    }
    updateMicroFocusHint();
    if ( mark ) {
	if( !q->style().styleHint( TQStyle::SH_BlinkCursorWhenTextSelected ))
	    setCursorVisible( FALSE );
	emit q->selectionChanged();
    }
}

void SecTQLineEditPrivate::finishChange( int validateFromState, bool setModified )
{
    bool lineDirty = selDirty;
    if ( textDirty ) {
	if ( validateFromState >= 0 ) {
#ifndef SECURE_NO_UNDO
	    undo( validateFromState );
#endif /* SECURE_NO_UNDO */
	    history.resize( undoState );
	    textDirty = setModified = FALSE;
	}
	updateTextLayout();
	updateMicroFocusHint();
	lineDirty |= textDirty;
	if ( setModified )
	    modified = TRUE;
	if ( textDirty ) {
	    textDirty = FALSE;
	    emit q->textChanged( text );
	}
        emit q->textModified( text );
#if defined(QT_ACCESSIBILITY_SUPPORT)
	TQAccessible::updateAccessibility( q, 0, TQAccessible::ValueChanged );
#endif
    }
    if ( selDirty ) {
	selDirty = FALSE;
	emit q->selectionChanged();
    }
    if ( lineDirty || !setModified )
	q->update();
}

void SecTQLineEditPrivate::setText( const SecTQString& txt )
{
    deselect();
    SecTQString oldText = text;
    text = txt.isEmpty() ? SecTQString ("") : txt.left( maxLength );
    history.clear();
    undoState = 0;
    cursor = text.length();
    textDirty = 1; // Err on safe side.
}


void SecTQLineEditPrivate::setCursorVisible( bool visible )
{
    if ( (bool)cursorVisible == visible )
	return;
    if ( cursorTimer )
	cursorVisible = visible;
    TQRect r = cursorRect();
    if ( !q->contentsRect().contains( r ) )
	q->update();
    else
	q->update( r );
}

#ifndef SECURE_NO_UNDO

void SecTQLineEditPrivate::addCommand( const Command& cmd )
{
    if ( separator && undoState && history[undoState-1].type != Separator ) {
	history.resize( undoState + 2 );
	history[undoState++] = Command( Separator, 0, 0 );
    } else {
	history.resize( undoState + 1);
    }
    separator = FALSE;
    history[ undoState++ ] = cmd;
}
#endif /* SECURE_NO_UNDO */

void SecTQLineEditPrivate::insert( const SecTQString& s )
{
  int remaining = maxLength - text.length();
  text.insert( cursor, s.left(remaining) );
  for ( int i = 0; i < (int) s.left(remaining).length(); ++i )
    {
#ifndef SECURE_NO_UNDO
      addCommand( Command( Insert, cursor, s.at(i) ) );
#endif /* SECURE_NO_UNDO */
      cursor++;
    }
  textDirty = TRUE;
}

void SecTQLineEditPrivate::del( bool wasBackspace )
{
    if ( cursor < (int) text.length() ) {
#ifndef SECURE_NO_UNDO
	addCommand ( Command( (CommandType)(wasBackspace?Remove:Delete), cursor, text.at(cursor) ) );
#endif /* SECURE_NO_UNDO */
	text.remove( cursor, 1 );
	textDirty = TRUE;
    }
}

void SecTQLineEditPrivate::removeSelectedText()
{
    if ( selstart < selend && selend <= (int) text.length() ) {
	separate();
#ifndef SECURE_NO_UNDO
	int i ;
	if ( selstart <= cursor && cursor < selend ) {
	    // cursor is within the selection. Split up the commands
	    // to be able to restore the correct cursor position
	    for ( i = cursor; i >= selstart; --i )
		addCommand ( Command( DeleteSelection, i, text.at(i) ) );
	    for ( i = selend - 1; i > cursor; --i )
		addCommand ( Command( DeleteSelection, i - cursor + selstart - 1, text.at(i) ) );
	} else {
	    for ( i = selend-1; i >= selstart; --i )
		addCommand ( Command( RemoveSelection, i, text.at(i) ) );
	}
#endif /* SECURE_NO_UNDO */
	text.remove( selstart, selend - selstart );
	if ( cursor > selstart )
	    cursor -= TQMIN( cursor, selend ) - selstart;
	deselect();
	textDirty = TRUE;
    }
}

#include "secqlineedit.moc"
