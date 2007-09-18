/* secqlineedit.cpp - Secure version of QLineEdit.
   Copyright (C) 1992-2002 Trolltech AS.  All rights reserved.
   Copyright (C) 2003 g10 Code GmbH

   The license of the original qlineedit.cpp file from which this file
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


/* Undo/Redo is disabled, because it uses unsecure memory for the
   history buffer.  */
#define SECURE_NO_UNDO	1


/**********************************************************************
** $Id$
**
** Implementation of SecQLineEdit widget class
**
** Created : 941011
**
** Copyright (C) 1992-2002 Trolltech AS.  All rights reserved.
**
** This file is part of the widgets module of the Qt GUI Toolkit.
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

#include "secqlineedit.h"
#include "qpainter.h"
#include "qdrawutil.h"
#include "qfontmetrics.h"
#include "qpixmap.h"
#include "qclipboard.h"
#include "qapplication.h"
#include "qtimer.h"
#include "qpopupmenu.h"
#include "qstringlist.h"
#include "qguardedptr.h"
#include "qstyle.h"
#include "qwhatsthis.h"
#include "secqinternal_p.h"
#include "private/qtextlayout_p.h"
#include "qvaluevector.h"
#if defined(QT_ACCESSIBILITY_SUPPORT)
#include "qaccessible.h"
#endif

#ifndef QT_NO_ACCEL
#include "qkeysequence.h"
#define ACCEL_KEY(k) "\t" + QString(QKeySequence( Qt::CTRL | Qt::Key_ ## k ))
#else
#define ACCEL_KEY(k) "\t" + QString("Ctrl+" #k)
#endif

#define innerMargin 1

struct SecQLineEditPrivate : public Qt
{
    SecQLineEditPrivate( SecQLineEdit *q )
	: q(q), cursor(0), cursorTimer(0), tripleClickTimer(0), frame(1),
	  cursorVisible(0), separator(0), readOnly(0), modified(0),
	  direction(QChar::DirON), alignment(0),
	  echoMode(0), textDirty(0), selDirty(0),
	  ascent(0), maxLength(32767), menuId(0),
	  hscroll(0),
	  undoState(0), selstart(0), selend(0),
	  imstart(0), imend(0), imselstart(0), imselend(0)
	{}
    void init( const SecQString&);

    SecQLineEdit *q;
    SecQString text;
    int cursor;
    int cursorTimer;
    QPoint tripleClick;
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
	inline Command( CommandType type, int pos, QChar c )
	    :type(type),c(c),pos(pos){}
	uint type : 4;
	QChar c;
	int pos;
    };
    int undoState;
    QValueVector<Command> history;
#ifndef SECURE_NO_UNDO
    void addCommand( const Command& cmd );
#endif /* SECURE_NO_UNDO */

    void insert( const SecQString& s );
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
    inline bool isRightToLeft() const { return direction==QChar::DirON?text.isRightToLeft():(direction==QChar::DirR); }

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
    int pos = xToPos( x, QTextItem::OnCharacters );  return pos >= selstart && pos < selend; }

    // input methods
    int imstart, imend, imselstart, imselend;

    // complex text layout
    QTextLayout textLayout;
    void updateTextLayout();
    void moveCursor( int pos, bool mark = FALSE );
    void setText( const SecQString& txt );
    int xToPos( int x, QTextItem::CursorPosition = QTextItem::BetweenCharacters ) const;
    inline int visualAlignment() const { return alignment ? alignment : int( isRightToLeft() ? AlignRight : AlignLeft ); }
    QRect cursorRect() const;
    void updateMicroFocusHint();

};


/*!
    \class SecQLineEdit
    \brief The SecQLineEdit widget is a one-line text editor.

    \ingroup basic
    \mainclass

    A line edit allows the user to enter and edit a single line of
    plain text with a useful collection of editing functions,
    including undo and redo, cut and paste,

    By changing the echoMode() of a line edit, it can also be used as
    a "write-only" field, for inputs such as passwords.

    The length of the text can be constrained to maxLength().

    A related class is QTextEdit which allows multi-line, rich-text
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

    By default, SecQLineEdits have a frame as specified by the Windows
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

    \sa QTextEdit QLabel QComboBox
	\link guibooks.html#fowler GUI Design Handbook: Field, Entry\endlink
*/


/*!
    \fn void SecQLineEdit::textChanged( const SecQString& )

    This signal is emitted whenever the text changes. The argument is
    the new text.
*/

/*!
    \fn void SecQLineEdit::selectionChanged()

    This signal is emitted whenever the selection changes.

    \sa hasSelectedText(), selectedText()
*/

/*!
    \fn void SecQLineEdit::lostFocus()

    This signal is emitted when the line edit has lost focus.

    \sa hasFocus(), QWidget::focusInEvent(), QWidget::focusOutEvent()
*/



/*!
    Constructs a line edit with no text.

    The maximum text length is set to 32767 characters.

    The \a parent and \a name arguments are sent to the QWidget constructor.

    \sa setText(), setMaxLength()
*/

SecQLineEdit::SecQLineEdit( QWidget* parent, const char* name )
    : QFrame( parent, name, WNoAutoErase ), d(new SecQLineEditPrivate( this ))
{
    d->init( SecQString::null );
}

/*!
    Constructs a line edit containing the text \a contents.

    The cursor position is set to the end of the line and the maximum
    text length to 32767 characters.

    The \a parent and \a name arguments are sent to the QWidget
    constructor.

    \sa text(), setMaxLength()
*/

SecQLineEdit::SecQLineEdit( const SecQString& contents, QWidget* parent, const char* name )
    : QFrame( parent, name, WNoAutoErase ), d(new SecQLineEditPrivate( this ))
{
    d->init( contents );
}

/*!
    Destroys the line edit.
*/

SecQLineEdit::~SecQLineEdit()
{
    delete d;
}


/*!
    \property SecQLineEdit::text
    \brief the line edit's text

    Setting this property clears the selection, clears the undo/redo
    history, moves the cursor to the end of the line and resets the
    \c modified property to FALSE.

    The text is truncated to maxLength() length.

    \sa insert()
*/
SecQString SecQLineEdit::text() const
{
    return ( d->text.isNull() ? SecQString ("") : d->text );
}

void SecQLineEdit::setText( const SecQString& text)
{
    resetInputContext();
    d->setText( text );
    d->modified = FALSE;
    d->finishChange( -1, FALSE );
}


/*!
    \property SecQLineEdit::displayText
    \brief the displayed text

    If \c EchoMode is \c Normal this returns the same as text(); if
    \c EchoMode is \c Password it returns a string of asterisks
    text().length() characters long, e.g. "******"; if \c EchoMode is
    \c NoEcho returns an empty string, "".

    \sa setEchoMode() text() EchoMode
*/

QString SecQLineEdit::displayText() const
{
    if ( d->echoMode == NoEcho )
	return QString::fromLatin1("");

    QChar pwd_char = QChar (style().styleHint( QStyle::SH_LineEdit_PasswordCharacter, this));
    QString res;
    res.fill (pwd_char, d->text.length ());
    return res;
}


/*!
    \property SecQLineEdit::maxLength
    \brief the maximum permitted length of the text

    If the text is too long, it is truncated at the limit.

    If truncation occurs any selected text will be unselected, the
    cursor position is set to 0 and the first part of the string is
    shown.
*/

int SecQLineEdit::maxLength() const
{
    return d->maxLength;
}

void SecQLineEdit::setMaxLength( int maxLength )
{
    d->maxLength = maxLength;
    setText( d->text );
}



/*!
    \property SecQLineEdit::frame
    \brief whether the line edit draws itself with a frame

    If enabled (the default) the line edit draws itself inside a
    two-pixel frame, otherwise the line edit draws itself without any
    frame.
*/
bool SecQLineEdit::frame() const
{
    return frameShape() != NoFrame;
}


void SecQLineEdit::setFrame( bool enable )
{
    setFrameStyle( enable ? ( LineEditPanel | Sunken ) : NoFrame  );
}


/*!
    \enum SecQLineEdit::EchoMode

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
    \property SecQLineEdit::echoMode
    \brief the line edit's echo mode

    The initial setting is \c Normal, but SecQLineEdit also supports \c
    NoEcho and \c Password modes.

    The widget's display and the ability to copy the text is affected
    by this setting.

    \sa EchoMode displayText()
*/

SecQLineEdit::EchoMode SecQLineEdit::echoMode() const
{
    return (EchoMode) d->echoMode;
}

void SecQLineEdit::setEchoMode( EchoMode mode )
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

QSize SecQLineEdit::sizeHint() const
{
    constPolish();
    QFontMetrics fm( font() );
    int h = QMAX(fm.lineSpacing(), 14) + 2*innerMargin;
    int w = fm.width( 'x' ) * 17; // "some"
    int m = frameWidth() * 2;
    return (style().sizeFromContents(QStyle::CT_LineEdit, this,
				     QSize( w + m, h + m ).
				     expandedTo(QApplication::globalStrut())));
}


/*!
    Returns a minimum size for the line edit.

    The width returned is enough for at least one character.
*/

QSize SecQLineEdit::minimumSizeHint() const
{
    constPolish();
    QFontMetrics fm = fontMetrics();
    int h = fm.height() + QMAX( 2*innerMargin, fm.leading() );
    int w = fm.maxWidth();
    int m = frameWidth() * 2;
    return QSize( w + m, h + m );
}


/*!
    \property SecQLineEdit::cursorPosition
    \brief the current cursor position for this line edit

    Setting the cursor position causes a repaint when appropriate.
*/

int SecQLineEdit::cursorPosition() const
{
    return d->cursor;
}


void SecQLineEdit::setCursorPosition( int pos )
{
    if ( pos <= (int) d->text.length() )
	d->moveCursor( pos );
}


/*!
    \property SecQLineEdit::alignment
    \brief the alignment of the line edit

    Possible Values are \c Qt::AlignAuto, \c Qt::AlignLeft, \c
    Qt::AlignRight and \c Qt::AlignHCenter.

    Attempting to set the alignment to an illegal flag combination
    does nothing.

    \sa Qt::AlignmentFlags
*/

int SecQLineEdit::alignment() const
{
    return d->alignment;
}

void SecQLineEdit::setAlignment( int flag )
{
    d->alignment = flag & 0x7;
    update();
}


/*!
  \obsolete
  \fn void SecQLineEdit::cursorRight( bool, int )

  Use cursorForward() instead.

  \sa cursorForward()
*/

/*!
  \obsolete
  \fn void SecQLineEdit::cursorLeft( bool, int )
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

void SecQLineEdit::cursorForward( bool mark, int steps )
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
void SecQLineEdit::cursorBackward( bool mark, int steps )
{
    cursorForward( mark, -steps );
}

/*!
    Moves the cursor one word forward. If \a mark is TRUE, the word is
    also selected.

    \sa cursorWordBackward()
*/
void SecQLineEdit::cursorWordForward( bool mark )
{
    d->moveCursor( d->textLayout.nextCursorPosition(d->cursor, QTextLayout::SkipWords), mark );
}

/*!
    Moves the cursor one word backward. If \a mark is TRUE, the word
    is also selected.

    \sa cursorWordForward()
*/

void SecQLineEdit::cursorWordBackward( bool mark )
{
    d->moveCursor( d->textLayout.previousCursorPosition(d->cursor, QTextLayout::SkipWords), mark );
}


/*!
    If no text is selected, deletes the character to the left of the
    text cursor and moves the cursor one position to the left. If any
    text is selected, the cursor is moved to the beginning of the
    selected text and the selected text is deleted.

    \sa del()
*/
void SecQLineEdit::backspace()
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

void SecQLineEdit::del()
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

void SecQLineEdit::home( bool mark )
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

void SecQLineEdit::end( bool mark )
{
    d->moveCursor( d->text.length(), mark );
}


/*!
    \property SecQLineEdit::modified
    \brief whether the line edit's contents has been modified by the user

    The modified flag is never read by SecQLineEdit; it has a default value
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

bool SecQLineEdit::isModified() const
{
    return d->modified;
}

/*!
    Resets the modified flag to FALSE.

    \sa isModified()
*/
void SecQLineEdit::clearModified()
{
    d->modified = FALSE;
    d->history.clear();
    d->undoState = 0;
}

/*!
  \obsolete
  \property SecQLineEdit::edited
  \brief whether the line edit has been edited. Use modified instead.
*/
bool SecQLineEdit::edited() const { return d->modified; }
void SecQLineEdit::setEdited( bool on ) { d->modified = on; }

/*!
    \obsolete
    \property SecQLineEdit::hasMarkedText
    \brief whether part of the text has been selected by the user. Use hasSelectedText instead.
*/

/*!
    \property SecQLineEdit::hasSelectedText
    \brief whether there is any text selected

    hasSelectedText() returns TRUE if some or all of the text has been
    selected by the user; otherwise returns FALSE.

    \sa selectedText()
*/


bool SecQLineEdit::hasSelectedText() const
{
    return d->hasSelectedText();
}

/*!
  \obsolete
  \property SecQLineEdit::markedText
  \brief the text selected by the user. Use selectedText instead.
*/

/*!
    \property SecQLineEdit::selectedText
    \brief the selected text

    If there is no selected text this property's value is
    QString::null.

    \sa hasSelectedText()
*/

SecQString SecQLineEdit::selectedText() const
{
    if ( d->hasSelectedText() )
	return d->text.mid( d->selstart, d->selend - d->selstart );
    return SecQString::null;
}

/*!
    selectionStart() returns the index of the first selected character in the
    line edit or -1 if no text is selected.

    \sa selectedText()
*/

int SecQLineEdit::selectionStart() const
{
    return d->hasSelectedText() ? d->selstart : -1;
}


/*!
    Selects text from position \a start and for \a length characters.

    \sa deselect() selectAll()
*/

void SecQLineEdit::setSelection( int start, int length )
{
    if ( start < 0 || start > (int)d->text.length() || length < 0 ) {
	d->selstart = d->selend = 0;
    } else {
	d->selstart = start;
	d->selend = QMIN( start + length, (int)d->text.length() );
	d->cursor = d->selend;
    }
    update();
}


/*!
    \property SecQLineEdit::undoAvailable
    \brief whether undo is available
*/

bool SecQLineEdit::isUndoAvailable() const
{
    return d->isUndoAvailable();
}

/*!
    \property SecQLineEdit::redoAvailable
    \brief whether redo is available
*/

bool SecQLineEdit::isRedoAvailable() const
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

void SecQLineEdit::selectAll()
{
    d->selstart = d->selend = d->cursor = 0;
    d->moveCursor( d->text.length(), TRUE );
}

/*!
    Deselects any selected text.

    \sa setSelection() selectAll()
*/

void SecQLineEdit::deselect()
{
    d->deselect();
    d->finishChange();
}


/*!
    Deletes any selected text, inserts \a newText and sets it as the
    new contents of the line edit.
*/
void SecQLineEdit::insert( const SecQString &newText )
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
void SecQLineEdit::clear()
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
    SecQLineEdit::undoAvailable available\endlink. Deselects any current
    selection, and updates the selection start to the current cursor
    position.
*/
void SecQLineEdit::undo()
{
#ifndef SECURE_NO_UNDO
    resetInputContext();
    d->undo();
    d->finishChange( -1, FALSE );
#endif
}

/*!
    Redoes the last operation if redo is \link
    SecQLineEdit::redoAvailable available\endlink.
*/
void SecQLineEdit::redo()
{
#ifndef SECURE_NO_UNDO
    resetInputContext();
    d->redo();
    d->finishChange();
#endif
}


/*!
    \property SecQLineEdit::readOnly
    \brief whether the line edit is read only.

    In read-only mode, the user can still copy the text to the
    clipboard (if echoMode() is \c Normal), but cannot edit it.

    SecQLineEdit does not show a cursor in read-only mode.

    \sa setEnabled()
*/

bool SecQLineEdit::isReadOnly() const
{
    return d->readOnly;
}

void SecQLineEdit::setReadOnly( bool enable )
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

void SecQLineEdit::cut()
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

void SecQLineEdit::copy() const
{
    d->copy();
}

/*!
    Inserts the clipboard's text at the cursor position, deleting any
    selected text, providing the line edit is not \link
    SecQLineEdit::readOnly read-only\endlink.

    \sa copy() cut()
*/

void SecQLineEdit::paste()
{
    d->removeSelectedText();
    insert( QApplication::clipboard()->text( QClipboard::Clipboard ) );
}

void SecQLineEditPrivate::copy( bool clipboard ) const
{
#ifndef SECURE
    QString t = q->selectedText();
    if ( !t.isEmpty() && echoMode == SecQLineEdit::Normal ) {
	q->disconnect( QApplication::clipboard(), SIGNAL(selectionChanged()), q, 0);
	QApplication::clipboard()->setText( t, clipboard ? QClipboard::Clipboard : QClipboard::Selection );
	q->connect( QApplication::clipboard(), SIGNAL(selectionChanged()),
		 q, SLOT(clipboardChanged()) );
    }
#endif
}

#endif // !QT_NO_CLIPBOARD

/*!\reimp
*/

void SecQLineEdit::resizeEvent( QResizeEvent *e )
{
    QFrame::resizeEvent( e );
}

/*! \reimp
*/
bool SecQLineEdit::event( QEvent * e )
{
    if ( e->type() == QEvent::AccelOverride && !d->readOnly ) {
	QKeyEvent* ke = (QKeyEvent*) e;
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
    } else if ( e->type() == QEvent::Timer ) {
	// should be timerEvent, is here for binary compatibility
	int timerId = ((QTimerEvent*)e)->timerId();
	if ( timerId == d->cursorTimer ) {
	    if(!hasSelectedText() || style().styleHint( QStyle::SH_BlinkCursorWhenTextSelected ))
		d->setCursorVisible( !d->cursorVisible );
	} else if ( timerId == d->tripleClickTimer ) {
	    killTimer( d->tripleClickTimer );
	    d->tripleClickTimer = 0;
	}
    }
    return QWidget::event( e );
}

/*! \reimp
*/
void SecQLineEdit::mousePressEvent( QMouseEvent* e )
{
    if ( e->button() == RightButton )
	return;
    if ( d->tripleClickTimer && ( e->pos() - d->tripleClick ).manhattanLength() <
	 QApplication::startDragDistance() ) {
	selectAll();
	return;
    }
    bool mark = e->state() & ShiftButton;
    int cursor = d->xToPos( e->pos().x() );
    d->moveCursor( cursor, mark );
}

/*! \reimp
*/
void SecQLineEdit::mouseMoveEvent( QMouseEvent * e )
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
void SecQLineEdit::mouseReleaseEvent( QMouseEvent* e )
{
#ifndef QT_NO_CLIPBOARD
    if (QApplication::clipboard()->supportsSelection() ) {
	if ( e->button() == LeftButton ) {
	    d->copy( FALSE );
	} else if ( !d->readOnly && e->button() == MidButton ) {
	    d->deselect();
	    insert( QApplication::clipboard()->text( QClipboard::Selection ) );
	}
    }
#endif
}

/*! \reimp
*/
void SecQLineEdit::mouseDoubleClickEvent( QMouseEvent* e )
{
    if ( e->button() == Qt::LeftButton ) {
	deselect();
	d->cursor = d->xToPos( e->pos().x() );
	d->cursor = d->textLayout.previousCursorPosition( d->cursor, QTextLayout::SkipWords );
	// ## text layout should support end of words.
	int end = d->textLayout.nextCursorPosition( d->cursor, QTextLayout::SkipWords );
	while ( end > d->cursor && d->text[end-1].isSpace() )
	    --end;
	d->moveCursor( end, TRUE );
	d->tripleClickTimer = startTimer( QApplication::doubleClickInterval() );
	d->tripleClick = e->pos();
    }
}

/*!
    \fn void  SecQLineEdit::returnPressed()

    This signal is emitted when the Return or Enter key is pressed.
*/

/*!
    Converts key press event \a e into a line edit action.

    If Return or Enter is pressed the signal returnPressed() is
    emitted.

    The default key bindings are listed in the \link #desc detailed
    description.\endlink
*/

void SecQLineEdit::keyPressEvent( QKeyEvent * e )
{
    d->setCursorVisible( TRUE );
    if ( e->key() == Key_Enter || e->key() == Key_Return ) {
	emit returnPressed();
	e->ignore();
	return;
    }
    if ( !d->readOnly ) {
	QString t = e->text();
	if ( !t.isEmpty() && (!e->ascii() || e->ascii()>=32) &&
	     e->key() != Key_Delete &&
	     e->key() != Key_Backspace ) {
#ifdef Q_WS_X11
	    extern bool qt_hebrew_keyboard_hack;
	    if ( qt_hebrew_keyboard_hack ) {
		// the X11 keyboard layout is broken and does not reverse
		// braces correctly. This is a hack to get halfway correct
		// behaviour
		if ( d->isRightToLeft() ) {
		    QChar *c = (QChar *)t.unicode();
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
	d->direction = QChar::DirL;
    else if ( e->key() == Key_Direction_R )
	d->direction = QChar::DirR;

    if ( unknown )
	e->ignore();
}

/*! \reimp
 */
void SecQLineEdit::imStartEvent( QIMEvent *e )
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
void SecQLineEdit::imComposeEvent( QIMEvent *e )
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
void SecQLineEdit::imEndEvent( QIMEvent *e )
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

void SecQLineEdit::focusInEvent( QFocusEvent* e )
{
    if ( e->reason() == QFocusEvent::Tab ||
	 e->reason() == QFocusEvent::Backtab  ||
	 e->reason() == QFocusEvent::Shortcut )
	selectAll();
    if ( !d->cursorTimer ) {
	int cft = QApplication::cursorFlashTime();
	d->cursorTimer = cft ? startTimer( cft/2 ) : -1;
    }
    d->updateMicroFocusHint();
}

/*!\reimp
*/

void SecQLineEdit::focusOutEvent( QFocusEvent* e )
{
    if ( e->reason() != QFocusEvent::ActiveWindow &&
	 e->reason() != QFocusEvent::Popup )
	deselect();
    d->setCursorVisible( FALSE );
    if ( d->cursorTimer > 0 )
	killTimer( d->cursorTimer );
    d->cursorTimer = 0;
    emit lostFocus();
}

/*!\reimp
*/
void SecQLineEdit::drawContents( QPainter *p )
{
    const QColorGroup& cg = colorGroup();
    QRect cr = contentsRect();
    QFontMetrics fm = fontMetrics();
    QRect lineRect( cr.x() + innerMargin, cr.y() + (cr.height() - fm.height() + 1) / 2,
		    cr.width() - 2*innerMargin, fm.height() );
    QBrush bg = QBrush( paletteBackgroundColor() );
    if ( paletteBackgroundPixmap() )
	bg = QBrush( cg.background(), *paletteBackgroundPixmap() );
    else if ( !isEnabled() )
	bg = cg.brush( QColorGroup::Background );
    p->save();
    p->setClipRegion( QRegion(cr) - lineRect );
    p->fillRect( cr, bg );
    p->restore();
    SecQSharedDoubleBuffer buffer( p, lineRect.x(), lineRect.y(),
 				lineRect.width(), lineRect.height(),
 				hasFocus() ? SecQSharedDoubleBuffer::Force : 0 );
    p = buffer.painter();
    p->fillRect( lineRect, bg );

    // locate cursor position
    int cix = 0;
    QTextItem ci = d->textLayout.findItem( d->cursor );
    if ( ci.isValid() ) {
	if ( d->cursor != (int)d->text.length() && d->cursor == ci.from() + ci.length()
	     && ci.isRightToLeft() != d->isRightToLeft() )
	    ci = d->textLayout.findItem( d->cursor + 1 );
	cix = ci.x() + ci.cursorToX( d->cursor - ci.from() );
    }

    // horizontal scrolling
    int minLB = QMAX( 0, -fm.minLeftBearing() );
    int minRB = QMAX( 0, -fm.minRightBearing() );
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
    QPoint topLeft = lineRect.topLeft() - QPoint(d->hscroll, d->ascent-fm.ascent());

    // draw text, selections and cursors
    p->setPen( cg.text() );
    bool supressCursor = d->readOnly, hasRightToLeft = d->isRightToLeft();
    int textflags = 0;
    if ( font().underline() )
	textflags |= Qt::Underline;
    if ( font().strikeOut() )
	textflags |= Qt::StrikeOut;
    if ( font().overline() )
	textflags |= Qt::Overline;

    for ( int i = 0; i < d->textLayout.numItems(); i++ ) {
	QTextItem ti = d->textLayout.itemAt( i );
	hasRightToLeft |= ti.isRightToLeft();
	int tix = topLeft.x() + ti.x();
	int first = ti.from();
	int last = ti.from() + ti.length() - 1;

	// text and selection
	if ( d->selstart < d->selend && (last >= d->selstart && first < d->selend ) ) {
	    QRect highlight = QRect( QPoint( tix + ti.cursorToX( QMAX( d->selstart - first, 0 ) ),
					     lineRect.top() ),
				     QPoint( tix + ti.cursorToX( QMIN( d->selend - first, last - first + 1 ) ) - 1,
					     lineRect.bottom() ) ).normalize();
	    p->save();
  	    p->setClipRegion( QRegion( lineRect ) - highlight, QPainter::CoordPainter );
 	    p->drawTextItem( topLeft, ti, textflags );
 	    p->setClipRect( lineRect & highlight, QPainter::CoordPainter );
	    p->fillRect( highlight, cg.highlight() );
 	    p->setPen( cg.highlightedText() );
	    p->drawTextItem( topLeft, ti, textflags );
	    p->restore();
	} else {
	    p->drawTextItem( topLeft, ti, textflags );
	}

	// input method edit area
	if ( d->imstart < d->imend && (last >= d->imstart && first < d->imend ) ) {
	    QRect highlight = QRect( QPoint( tix + ti.cursorToX( QMAX( d->imstart - first, 0 ) ), lineRect.top() ),
			      QPoint( tix + ti.cursorToX( QMIN( d->imend - first, last - first + 1 ) )-1, lineRect.bottom() ) ).normalize();
	    p->save();
 	    p->setClipRect( lineRect & highlight, QPainter::CoordPainter );

	    int h1, s1, v1, h2, s2, v2;
	    cg.color( QColorGroup::Base ).hsv( &h1, &s1, &v1 );
	    cg.color( QColorGroup::Background ).hsv( &h2, &s2, &v2 );
	    QColor imCol;
	    imCol.setHsv( h1, s1, ( v1 + v2 ) / 2 );
	    p->fillRect( highlight, imCol );
	    p->drawTextItem( topLeft, ti, textflags );
	    p->restore();
	}

	// input method selection
	if ( d->imselstart < d->imselend && (last >= d->imselstart && first < d->imselend ) ) {
	    QRect highlight = QRect( QPoint( tix + ti.cursorToX( QMAX( d->imselstart - first, 0 ) ), lineRect.top() ),
			      QPoint( tix + ti.cursorToX( QMIN( d->imselend - first, last - first + 1 ) )-1, lineRect.bottom() ) ).normalize();
	    p->save();
	    p->setClipRect( lineRect & highlight, QPainter::CoordPainter );
	    p->fillRect( highlight, cg.text() );
	    p->setPen( paletteBackgroundColor() );
	    p->drawTextItem( topLeft, ti, textflags );
	    p->restore();
	}
    }

    // draw cursor
    if ( d->cursorVisible && !supressCursor ) {
	QPoint from( topLeft.x() + cix, lineRect.top() );
	QPoint to = from + QPoint( 0, lineRect.height() );
	p->drawLine( from, to );
	if ( hasRightToLeft ) {
	    to = from + QPoint( (ci.isRightToLeft()?-2:2), 2 );
	    p->drawLine( from, to );
	    from.ry() += 4;
	    p->drawLine( from, to );
	}
    }
    buffer.end();
}


enum { IdUndo, IdRedo, IdSep1, IdCut, IdCopy, IdPaste, IdClear, IdSep2, IdSelectAll };


/*! \reimp */
void SecQLineEdit::windowActivationChange( bool b )
{
    //### remove me with WHighlightSelection attribute
    if ( palette().active() != palette().inactive() )
	update();
    QWidget::windowActivationChange( b );
}

/*! \reimp */

void SecQLineEdit::setPalette( const QPalette & p )
{
    //### remove me with WHighlightSelection attribute
    QWidget::setPalette( p );
    update();
}

/*!
  \obsolete
  \fn void SecQLineEdit::repaintArea( int from, int to )
  Repaints all characters from \a from to \a to. If cursorPos is
  between from and to, ensures that cursorPos is visible.
*/

/*! \reimp
 */
void SecQLineEdit::setFont( const QFont & f )
{
    QWidget::setFont( f );
    d->updateTextLayout();
}


void SecQLineEdit::clipboardChanged()
{
}

void SecQLineEditPrivate::init( const SecQString& txt )
{
#ifndef QT_NO_CURSOR
    q->setCursor( readOnly ? arrowCursor : ibeamCursor );
#endif
    q->setFocusPolicy( QWidget::StrongFocus );
    q->setInputMethodEnabled( TRUE );
    //   Specifies that this widget can use more, but is able to survive on
    //   less, horizontal space; and is fixed vertically.
    q->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) );
    q->setBackgroundMode( PaletteBase );
    q->setKeyCompression( TRUE );
    q->setMouseTracking( TRUE );
    q->setAcceptDrops( TRUE );
    q->setFrame( TRUE );
    text = txt;
    updateTextLayout();
    cursor = text.length();
}

void SecQLineEditPrivate::updateTextLayout()
{
    // replace all non-printable characters with spaces (to avoid
    // drawing boxes when using fonts that don't have glyphs for such
    // characters)
    const QString &displayText = q->displayText();
    QString str(displayText.unicode(), displayText.length());
    QChar* uc = (QChar*)str.unicode();
    for (int i = 0; i < (int)str.length(); ++i) {
	if (! uc[i].isPrint())
	    uc[i] = QChar(0x0020);
    }
    textLayout.setText( str, q->font() );
    // ### want to do textLayout.setRightToLeft( text.isRightToLeft() );
    textLayout.beginLayout( QTextLayout::SingleLine );
    textLayout.beginLine( INT_MAX );
    while ( !textLayout.atEnd() )
	textLayout.addCurrentItem();
    ascent = 0;
    textLayout.endLine(0, 0, Qt::AlignLeft, &ascent);
}

int SecQLineEditPrivate::xToPos( int x, QTextItem::CursorPosition betweenOrOn ) const
{
    x-= q->contentsRect().x() - hscroll + innerMargin;
    for ( int i = 0; i < textLayout.numItems(); ++i ) {
	QTextItem ti = textLayout.itemAt( i );
	QRect tir = ti.rect();
	if ( x >= tir.left() && x <= tir.right() )
	    return ti.xToCursor( x - tir.x(), betweenOrOn ) + ti.from();
    }
    return x < 0 ? 0 : text.length();
}


QRect SecQLineEditPrivate::cursorRect() const
{
    QRect cr = q->contentsRect();
    int cix = cr.x() - hscroll + innerMargin;
    QTextItem ci = textLayout.findItem( cursor );
    if ( ci.isValid() ) {
	if ( cursor != (int)text.length() && cursor == ci.from() + ci.length()
	     && ci.isRightToLeft() != isRightToLeft() )
	    ci = textLayout.findItem( cursor + 1 );
	cix += ci.x() + ci.cursorToX( cursor - ci.from() );
    }
    int ch = q->fontMetrics().height();
    return QRect( cix-4, cr.y() + ( cr.height() -  ch + 1) / 2, 8, ch + 1 );
}

void SecQLineEditPrivate::updateMicroFocusHint()
{
    if ( q->hasFocus() ) {
	QRect r = cursorRect();
	q->setMicroFocusHint( r.x(), r.y(), r.width(), r.height() );
    }
}

void SecQLineEditPrivate::moveCursor( int pos, bool mark )
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
	selstart = QMIN( anchor, pos );
	selend = QMAX( anchor, pos );
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
	if( !q->style().styleHint( QStyle::SH_BlinkCursorWhenTextSelected ))
	    setCursorVisible( FALSE );
	emit q->selectionChanged();
    }
}

void SecQLineEditPrivate::finishChange( int validateFromState, bool setModified )
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
	QAccessible::updateAccessibility( q, 0, QAccessible::ValueChanged );
#endif
    }
    if ( selDirty ) {
	selDirty = FALSE;
	emit q->selectionChanged();
    }
    if ( lineDirty || !setModified )
	q->update();
}

void SecQLineEditPrivate::setText( const SecQString& txt )
{
    deselect();
    SecQString oldText = text;
    text = txt.isEmpty() ? SecQString ("") : txt.left( maxLength );
    history.clear();
    undoState = 0;
    cursor = text.length();
    textDirty = 1; // Err on safe side.
}


void SecQLineEditPrivate::setCursorVisible( bool visible )
{
    if ( (bool)cursorVisible == visible )
	return;
    if ( cursorTimer )
	cursorVisible = visible;
    QRect r = cursorRect();
    if ( !q->contentsRect().contains( r ) )
	q->update();
    else
	q->update( r );
}

#ifndef SECURE_NO_UNDO

void SecQLineEditPrivate::addCommand( const Command& cmd )
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

void SecQLineEditPrivate::insert( const SecQString& s )
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

void SecQLineEditPrivate::del( bool wasBackspace )
{
    if ( cursor < (int) text.length() ) {
#ifndef SECURE_NO_UNDO
	addCommand ( Command( (CommandType)(wasBackspace?Remove:Delete), cursor, text.at(cursor) ) );
#endif /* SECURE_NO_UNDO */
	text.remove( cursor, 1 );
	textDirty = TRUE;
    }
}

void SecQLineEditPrivate::removeSelectedText()
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
	    cursor -= QMIN( cursor, selend ) - selstart;
	deselect();
	textDirty = TRUE;
    }
}
