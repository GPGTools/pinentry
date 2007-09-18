/* secqlineedit.h - Secure version of QLineEdit.
   Copyright (C) 1992-2002 Trolltech AS.  All rights reserved.
   Copyright (C) 2003 g10 Code GmbH

   The license of the original qlineedit.h file from which this file
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

#include "secqstring.h"

/* This disables some insecure code.  */
#define SECURE	1

/**********************************************************************
** $Id$
**
** Definition of SecQLineEdit widget class
**
** Created : 941011
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
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

#ifndef SECQLINEEDIT_H
#define SECQLINEEDIT_H

struct SecQLineEditPrivate;

class QPopupMenu;

#ifndef QT_H
#include "qframe.h"
#include "qstring.h"
#endif // QT_H

class QTextParagraph;
class QTextCursor;

class Q_EXPORT SecQLineEdit : public QFrame
{
    Q_OBJECT
    Q_ENUMS( EchoMode )
      //    Q_PROPERTY( SecQString text READ text WRITE setText )
    Q_PROPERTY( int maxLength READ maxLength WRITE setMaxLength )
    Q_PROPERTY( bool frame READ frame WRITE setFrame )
    Q_PROPERTY( EchoMode echoMode READ echoMode WRITE setEchoMode )
    Q_PROPERTY( QString displayText READ displayText )
    Q_PROPERTY( int cursorPosition READ cursorPosition WRITE setCursorPosition )
    Q_PROPERTY( Alignment alignment READ alignment WRITE setAlignment )
    Q_PROPERTY( bool edited READ edited WRITE setEdited DESIGNABLE false )
    Q_PROPERTY( bool modified READ isModified )
    Q_PROPERTY( bool hasMarkedText READ hasMarkedText DESIGNABLE false )
    Q_PROPERTY( bool hasSelectedText READ hasSelectedText )
      //    Q_PROPERTY( SecQString markedText READ markedText DESIGNABLE false )
      //    Q_PROPERTY( SecQString selectedText READ selectedText )
    Q_PROPERTY( bool readOnly READ isReadOnly WRITE setReadOnly )
    Q_PROPERTY( bool undoAvailable READ isUndoAvailable )
    Q_PROPERTY( bool redoAvailable READ isRedoAvailable )

public:
    SecQLineEdit( QWidget* parent, const char* name=0 );
    SecQLineEdit( const SecQString &, QWidget* parent, const char* name=0 );
    SecQLineEdit( const SecQString &, const QString &, QWidget* parent, const char* name=0 );
    ~SecQLineEdit();

    SecQString text() const;

    QString displayText() const;

    int maxLength() const;

    bool frame() const;

    enum EchoMode { Normal, NoEcho, Password };
    EchoMode echoMode() const;

    bool isReadOnly() const;

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

    int cursorPosition() const;
    bool validateAndSet( const SecQString &, int, int, int ); // obsolete

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
    SecQString selectedText() const;
    int selectionStart() const;

    bool isUndoAvailable() const;
    bool isRedoAvailable() const;

#ifndef QT_NO_COMPAT
    bool hasMarkedText() const { return hasSelectedText(); }
    SecQString markedText() const { return selectedText(); }
#endif

public slots:
    virtual void setText( const SecQString &);
    virtual void selectAll();
    virtual void deselect();
    virtual void insert( const SecQString &);
    virtual void clear();
    virtual void undo();
    virtual void redo();
    virtual void setMaxLength( int );
    virtual void setFrame( bool );
    virtual void setEchoMode( EchoMode );
    virtual void setReadOnly( bool );
    virtual void setFont( const QFont & );
    virtual void setPalette( const QPalette & );
    virtual void setSelection( int, int );
    virtual void setCursorPosition( int );
    virtual void setAlignment( int flag );
#ifndef QT_NO_CLIPBOARD
    virtual void cut();
    virtual void copy() const;
    virtual void paste();
#endif

signals:
    void textChanged( const SecQString &);
    void textModified( const SecQString &);
    void returnPressed();
    void lostFocus();
    void selectionChanged();

protected:
    bool event( QEvent * );
    void mousePressEvent( QMouseEvent * );
    void mouseMoveEvent( QMouseEvent * );
    void mouseReleaseEvent( QMouseEvent * );
    void mouseDoubleClickEvent( QMouseEvent * );
    void keyPressEvent( QKeyEvent * );
    void imStartEvent( QIMEvent * );
    void imComposeEvent( QIMEvent * );
    void imEndEvent( QIMEvent * );
    void focusInEvent( QFocusEvent * );
    void focusOutEvent( QFocusEvent * );
    void resizeEvent( QResizeEvent * );
    void drawContents( QPainter * );
    void windowActivationChange( bool );
#ifndef QT_NO_COMPAT
    void repaintArea( int, int ) { update(); }
#endif

private slots:
    void clipboardChanged();

public:
    QChar passwordChar() const; // obsolete internal

private:
    friend struct SecQLineEditPrivate;
    SecQLineEditPrivate * d;

private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    SecQLineEdit( const SecQLineEdit & );
    SecQLineEdit &operator=( const SecQLineEdit & );
#endif
};

#endif // SECQLINEEDIT_H
