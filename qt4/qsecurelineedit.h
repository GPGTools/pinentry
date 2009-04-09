/* 
   qsecurelineedit.h - QLineEdit that uses secmem

   Copyright (C) 2008 Klarälvdalens Datakonsult AB (KDAB)

   Written by Marc Mutz <marc@kdab.com>.
   
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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

// the following code has been ported from QString to secqstring.
// The licence of the original code:

/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** Licensees holding a valid Qt License Agreement may use this file in
** accordance with the rights, responsibilities and obligations
** contained therein.  Please consult your licensing agreement or
** contact sales@trolltech.com if any conditions of this licensing
** agreement are not clear to you.
**
** Further information about Qt licensing is available at:
** http://www.trolltech.com/products/qt/licensing.html or by
** contacting info@trolltech.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef QSECURELINEEDIT_H
#define QSECURELINEEDIT_H

#include <QtGui/qframe.h>
#include <QtCore/qstring.h>

#include "secstring.h"

// for moc, since qt4_automoc doesn't appear to hand over defines when
// running moc. They should't be visible when #including other Qt
// headers, since they #ifdef out virtual functions (->BIC).
#ifndef QT_NO_VALIDATOR
# define QT_NO_VALIDATOR
#endif
#ifndef QT_NO_COMPLETER
# define QT_NO_COMPLETER
#endif
#ifndef QT_NO_CLIPBOARD
# define QT_NO_CLIPBOARD
#endif
#ifndef QT_NO_CONTEXTMENU
# define QT_NO_CONTEXTMENU
#endif
#ifndef QT_NO_DRAGANDDROP
# define QT_NO_DRAGANDDROP
#endif
#ifndef QT_NO_STYLE_STYLESHEET
# define QT_NO_STYLE_STYLESHEET
#endif

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_LINEEDIT

class QValidator;
class QMenu;
class QSecureLineEditPrivate;
class QCompleter;
class QStyleOptionFrame;
class QAbstractSpinBox;
class QDateTimeEdit;

class QSecureLineEdit : public QWidget
{
    Q_OBJECT

    Q_ENUMS(EchoMode)
    Q_PROPERTY(QString inputMask READ inputMask WRITE setInputMask)
    Q_PROPERTY(secqstring text READ text WRITE setText NOTIFY textChanged USER true)
    Q_PROPERTY(int maxLength READ maxLength WRITE setMaxLength)
    Q_PROPERTY(bool frame READ hasFrame WRITE setFrame)
    Q_PROPERTY(EchoMode echoMode READ echoMode /*WRITE setEchoMode*/)
    Q_PROPERTY(secqstring displayText READ displayText)
    Q_PROPERTY(int cursorPosition READ cursorPosition WRITE setCursorPosition)
    Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment)
    Q_PROPERTY(bool modified READ isModified WRITE setModified DESIGNABLE false)
    Q_PROPERTY(bool hasSelectedText READ hasSelectedText)
    Q_PROPERTY(secqstring selectedText READ selectedText)
    Q_PROPERTY(bool dragEnabled READ dragEnabled WRITE setDragEnabled)
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly)
    Q_PROPERTY(bool undoAvailable READ isUndoAvailable)
    Q_PROPERTY(bool redoAvailable READ isRedoAvailable)
    Q_PROPERTY(bool acceptableInput READ hasAcceptableInput)

public:
    explicit QSecureLineEdit(QWidget* parent=0);
    explicit QSecureLineEdit(const secqstring &, QWidget* parent=0);
#ifdef QT3_SUPPORT
    QT3_SUPPORT_CONSTRUCTOR QSecureLineEdit(QWidget* parent, const char* name);
    QT3_SUPPORT_CONSTRUCTOR QSecureLineEdit(const secqstring &, QWidget* parent, const char* name);
    QT3_SUPPORT_CONSTRUCTOR QSecureLineEdit(const secqstring &, const QString &, QWidget* parent=0, const char* name=0);
#endif
    ~QSecureLineEdit();

    secqstring text() const;

    secqstring displayText() const;

    int maxLength() const;
    void setMaxLength(int);

    void setFrame(bool);
    bool hasFrame() const;

    enum EchoMode { Normal, NoEcho, Password, PasswordEchoOnEdit };
    EchoMode echoMode() const;
private:
    void setEchoMode(EchoMode);
public:

    bool isReadOnly() const;
    void setReadOnly(bool);

#ifndef QT_NO_VALIDATOR
    void setValidator(const QValidator *);
    const QValidator * validator() const;
#endif

#ifndef QT_NO_COMPLETER
    void setCompleter(QCompleter *completer);
    QCompleter *completer() const;
#endif

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

    int cursorPosition() const;
    void setCursorPosition(int);
    int cursorPositionAt(const QPoint &pos);

    void setAlignment(Qt::Alignment flag);
    Qt::Alignment alignment() const;

    void cursorForward(bool mark, int steps = 1);
    void cursorBackward(bool mark, int steps = 1);
    void cursorWordForward(bool mark);
    void cursorWordBackward(bool mark);
    void backspace();
    void del();
    void home(bool mark);
    void end(bool mark);

    bool isModified() const;
    void setModified(bool);

    void setSelection(int, int);
    bool hasSelectedText() const;
    secqstring selectedText() const;
    int selectionStart() const;

    bool isUndoAvailable() const;
    bool isRedoAvailable() const;

    void setDragEnabled(bool b);
    bool dragEnabled() const;

    QString inputMask() const;
    void setInputMask(const QString &inputMask);
    bool hasAcceptableInput() const;

public Q_SLOTS:
    void setText(const secqstring &);
    void clear();
    void selectAll();
    void undo();
    void redo();
#ifndef QT_NO_CLIPBOARD
    void cut();
    void copy() const;
    void paste();
#endif

public:
    void deselect();
    void insert(const secqstring &);
#ifndef QT_NO_CONTEXTMENU
    QMenu *createStandardContextMenu();
#endif

Q_SIGNALS:
    void textChanged(const secqstring &);
    void textEdited(const secqstring &);
    void cursorPositionChanged(int, int);
    void returnPressed();
    void editingFinished();
    void selectionChanged();

protected:
    void mousePressEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseDoubleClickEvent(QMouseEvent *);
    void keyPressEvent(QKeyEvent *);
    void focusInEvent(QFocusEvent *);
    void focusOutEvent(QFocusEvent *);
    void paintEvent(QPaintEvent *);
#ifndef QT_NO_DRAGANDDROP
    void dragEnterEvent(QDragEnterEvent *);
    void dragMoveEvent(QDragMoveEvent *e);
    void dragLeaveEvent(QDragLeaveEvent *e);
    void dropEvent(QDropEvent *);
#endif
    void changeEvent(QEvent *);
#ifndef QT_NO_CONTEXTMENU
    void contextMenuEvent(QContextMenuEvent *);
#endif
#ifdef QT3_SUPPORT
    inline QT3_SUPPORT void repaintArea(int, int) { update(); }
#endif

    void inputMethodEvent(QInputMethodEvent *);
    void initStyleOption(QStyleOptionFrame *option) const;
public:
    QVariant inputMethodQuery(Qt::InputMethodQuery) const;
    bool event(QEvent *);
protected:
    QRect cursorRect() const;

public:
#ifdef QT3_SUPPORT
    inline QT3_SUPPORT void clearModified() { setModified(false); }
    inline QT3_SUPPORT void cursorLeft(bool mark, int steps = 1) { cursorForward(mark, -steps); }
    inline QT3_SUPPORT void cursorRight(bool mark, int steps = 1) { cursorForward(mark, steps); }
    QT3_SUPPORT bool validateAndSet(const QString &, int, int, int);
    inline QT3_SUPPORT bool frame() const { return hasFrame(); }
#ifndef QT_NO_VALIDATOR
    inline QT3_SUPPORT void clearValidator() { setValidator(0); }
#endif
    inline QT3_SUPPORT bool hasMarkedText() const { return hasSelectedText(); }
    inline QT3_SUPPORT secqstring markedText() const { return selectedText(); }
    QT3_SUPPORT bool edited() const;
    QT3_SUPPORT void setEdited(bool);
    QT3_SUPPORT int characterAt(int, QChar*) const;
    QT3_SUPPORT bool getSelection(int *, int *);

    QT3_SUPPORT void setFrameRect(QRect) {}
    QT3_SUPPORT QRect frameRect() const { return QRect(); }
    enum DummyFrame { Box, Sunken, Plain, Raised, MShadow, NoFrame, Panel, StyledPanel,
                      HLine, VLine, GroupBoxPanel, WinPanel, ToolBarPanel, MenuBarPanel,
                      PopupPanel, LineEditPanel, TabWidgetPanel, MShape };
    QT3_SUPPORT void setFrameShadow(DummyFrame) {}
    QT3_SUPPORT DummyFrame frameShadow() const { return Plain; }
    QT3_SUPPORT void setFrameShape(DummyFrame) {}
    QT3_SUPPORT DummyFrame frameShape() const { return NoFrame; }
    QT3_SUPPORT void setFrameStyle(int) {}
    QT3_SUPPORT int frameStyle() const  { return 0; }
    QT3_SUPPORT int frameWidth() const { return 0; }
    QT3_SUPPORT void setLineWidth(int) {}
    QT3_SUPPORT int lineWidth() const { return 0; }
    QT3_SUPPORT void setMargin(int margin) { setContentsMargins(margin, margin, margin, margin); }
    QT3_SUPPORT int margin() const
    { int margin; int dummy; getContentsMargins(&margin, &dummy, &dummy, &dummy);  return margin; }
    QT3_SUPPORT void setMidLineWidth(int) {}
    QT3_SUPPORT int midLineWidth() const { return 0; }

Q_SIGNALS:
    QT_MOC_COMPAT void lostFocus();
#endif

private:
    friend class QAbstractSpinBox;
#ifdef QT_KEYPAD_NAVIGATION
    friend class QDateTimeEdit;
#endif
    Q_DISABLE_COPY(QSecureLineEdit)
    QSecureLineEditPrivate * m_d;//Q_DECLARE_PRIVATE(QSecureLineEdit)
    QSecureLineEditPrivate * d_func() { return m_d; }
    const QSecureLineEditPrivate * d_func() const { return m_d; }
    friend class QSecureLineEditPrivate;
    Q_PRIVATE_SLOT(d_func(), void _q_clipboardChanged())
    Q_PRIVATE_SLOT(d_func(), void _q_handleWindowActivate())
    Q_PRIVATE_SLOT(d_func(), void _q_deleteSelected())
#ifndef QT_NO_COMPLETER
    Q_PRIVATE_SLOT(d_func(), void _q_completionHighlighted(QString))
#endif
};

#endif // QT_NO_LINEEDIT

QT_END_NAMESPACE

QT_END_HEADER

#endif // QSECURELINEEDIT_H
