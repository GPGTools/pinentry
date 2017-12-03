/* secntqlineedit.h - Secure version of TQLineEdit.
 * Copyright (C) 1992-2002 Trolltech AS.  All rights reserved.
 * Copyright (C) 2003 g10 Code GmbH
 *
 * The license of the original ntqlineedit.h file from which this file
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

#include "secqstring.h"

/* This disables some insecure code.  */
#define SECURE	1

/**********************************************************************
** $Id$
**
** Definition of SecTQLineEdit widget class
**
** Created : 941011
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
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

#ifndef SECTQLINEEDIT_H
#define SECTQLINEEDIT_H

struct SecTQLineEditPrivate;

class TQPopupMenu;

#ifndef QT_H
#include "ntqframe.h"
#include "ntqstring.h"
#endif // QT_H

class TQTextParagraph;
class TQTextCursor;

class Q_EXPORT SecTQLineEdit : public TQFrame
{
    TQ_OBJECT
    TQ_ENUMS( EchoMode )
      //    TQ_PROPERTY( SecTQString text READ text WRITE setText )
    TQ_PROPERTY( int maxLength READ maxLength WRITE setMaxLength )
    TQ_PROPERTY( bool frame READ frame WRITE setFrame )
    TQ_PROPERTY( EchoMode echoMode READ echoMode WRITE setEchoMode )
    TQ_PROPERTY( TQString displayText READ displayText )
    TQ_PROPERTY( int cursorPosition READ cursorPosition WRITE setCursorPosition )
    TQ_PROPERTY( Alignment alignment READ alignment WRITE setAlignment )
    TQ_PROPERTY( bool edited READ edited WRITE setEdited DESIGNABLE false )
    TQ_PROPERTY( bool modified READ isModified )
    TQ_PROPERTY( bool hasSelectedText READ hasSelectedText )
      //    TQ_PROPERTY( SecTQString markedText READ markedText DESIGNABLE false )
      //    TQ_PROPERTY( SecTQString selectedText READ selectedText )
    TQ_PROPERTY( bool readOnly READ isReadOnly WRITE setReadOnly )
    TQ_PROPERTY( bool undoAvailable READ isUndoAvailable )
    TQ_PROPERTY( bool redoAvailable READ isRedoAvailable )

public:
    SecTQLineEdit( TQWidget* parent, const char* name=0 );
    SecTQLineEdit( const SecTQString &, TQWidget* parent, const char* name=0 );
    SecTQLineEdit( const SecTQString &, const TQString &, TQWidget* parent, const char* name=0 );
    ~SecTQLineEdit();

    SecTQString text() const;

    TQString displayText() const;

    int maxLength() const;

    bool frame() const;

    enum EchoMode { Normal, NoEcho, Password };
    EchoMode echoMode() const;

    bool isReadOnly() const;

    TQSize sizeHint() const;
    TQSize minimumSizeHint() const;

    int cursorPosition() const;
    bool validateAndSet( const SecTQString &, int, int, int ); // obsolete

    int alignment() const;

#ifndef QT_NO_COMPAT
    void cursorLeft( bool mark, int steps = 1 ) { cursorForward( mark, -steps ); }
    void cursorRight( bool mark, int steps = 1 ) { cursorForward( mark, steps ); }
#endif
    void cursorForward( bool mark, int steps = 1 );
    void cursorBackward( bool mark, int steps = 1 );
    void cursorWordForward( bool mark );
    void cursorWordBackward( bool mark );
    void backspace();
    void del();
    void home( bool mark );
    void end( bool mark );

    bool isModified() const;
    void clearModified();

    bool edited() const; // obsolete, use isModified()
    void setEdited( bool ); // obsolete, use clearModified()

    bool hasSelectedText() const;
    SecTQString selectedText() const;
    int selectionStart() const;

    bool isUndoAvailable() const;
    bool isRedoAvailable() const;

#ifndef QT_NO_COMPAT
    bool hasMarkedText() const { return hasSelectedText(); }
    SecTQString markedText() const { return selectedText(); }
#endif

public slots:
    virtual void setText( const SecTQString &);
    virtual void selectAll();
    virtual void deselect();
    virtual void insert( const SecTQString &);
    virtual void clear();
    virtual void undo();
    virtual void redo();
    virtual void setMaxLength( int );
    virtual void setFrame( bool );
    virtual void setEchoMode( EchoMode );
    virtual void setReadOnly( bool );
    virtual void setFont( const TQFont & );
    virtual void setPalette( const TQPalette & );
    virtual void setSelection( int, int );
    virtual void setCursorPosition( int );
    virtual void setAlignment( int flag );
#ifndef QT_NO_CLIPBOARD
    virtual void cut();
    virtual void copy() const;
    virtual void paste();
#endif

signals:
    void textChanged( const SecTQString &);
    void textModified( const SecTQString &);
    void returnPressed();
    void lostFocus();
    void selectionChanged();

protected:
    bool event( TQEvent * );
    void mousePressEvent( TQMouseEvent * );
    void mouseMoveEvent( TQMouseEvent * );
    void mouseReleaseEvent( TQMouseEvent * );
    void mouseDoubleClickEvent( TQMouseEvent * );
    void keyPressEvent( TQKeyEvent * );
    void imStartEvent( TQIMEvent * );
    void imComposeEvent( TQIMEvent * );
    void imEndEvent( TQIMEvent * );
    void focusInEvent( TQFocusEvent * );
    void focusOutEvent( TQFocusEvent * );
    void resizeEvent( TQResizeEvent * );
    void drawContents( TQPainter * );
    void windowActivationChange( bool );
#ifndef QT_NO_COMPAT
    void repaintArea( int, int ) { update(); }
#endif

private slots:
    void clipboardChanged();

public:
    TQChar passwordChar() const; // obsolete internal

private:
    friend struct SecTQLineEditPrivate;
    SecTQLineEditPrivate * d;

private:	// Disabled copy constructor and operator=
#if defined(TQ_DISABLE_COPY)
    SecTQLineEdit( const SecTQLineEdit & );
    SecTQLineEdit &operator=( const SecTQLineEdit & );
#endif
};

#endif // SECTQLINEEDIT_H
