/* 
   qsecurelineedit_p.h - QLineEdit that uses secmem

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

#ifndef QSECURELINEEDIT_P_H
#define QSECURELINEEDIT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "QtCore/qglobal.h"

//#include "qsecurelineedit.h" // make last for QT_NO

#ifndef QT_NO_LINEEDIT
//#include "private/qwidget_p.h"
#include "QtGui/qtextlayout.h"
#include "QtGui/qstyleoption.h"
#include "QtCore/qbasictimer.h"
#include "QtGui/qcompleter.h"
#include "QtCore/qpointer.h"

#include "qsecurelineedit.h" // last to not contaminate Qt headers with #defines

QT_BEGIN_NAMESPACE

class QSecureLineEditPrivate// : public QWidgetPrivate
{
    QSecureLineEdit * q_ptr;//Q_DECLARE_PUBLIC(QSecureLineEdit)
    QSecureLineEdit * q_func() { return q_ptr; }
    const QSecureLineEdit * q_func() const { return q_ptr; }
public:

    explicit QSecureLineEditPrivate( QSecureLineEdit * qq )
        : q_ptr(qq), cursor(0), preeditCursor(0), cursorTimer(0), frame(1),
          cursorVisible(0), hideCursor(false), separator(0), readOnly(0),
          dragEnabled(0), contextMenuEnabled(1), echoMode(QSecureLineEdit::Password), textDirty(0),
          selDirty(0), validInput(1), alignment(Qt::AlignLeading | Qt::AlignVCenter), ascent(0),
          maxLength(32767), hscroll(0), vscroll(0), lastCursorPos(-1), maskData(0),
          modifiedState(0), undoState(0), selstart(0), selend(0), userInput(false),
          emitingEditingFinished(false), resumePassword(false)
#ifndef QT_NO_COMPLETER
        , completer(0)
#endif
        {
#ifndef QT_NO_MENU
            actions[UndoAct] = 0;
#endif
        }

    ~QSecureLineEditPrivate()
    {
        delete [] maskData;
    }
    void init(const secqstring&);

    secqstring text;
    int cursor;
    int preeditCursor;
    int cursorTimer; // -1 for non blinking cursor.
    QPoint tripleClick;
    QBasicTimer tripleClickTimer;
    uint frame : 1;
    uint cursorVisible : 1;
    uint hideCursor : 1; // used to hide the cursor inside preedit areas
    uint separator : 1;
    uint readOnly : 1;
    uint dragEnabled : 1;
    uint contextMenuEnabled : 1;
    uint echoMode : 2;
    uint textDirty : 1;
    uint selDirty : 1;
    uint validInput : 1;
    uint alignment;
    int ascent;
    int maxLength;
    int hscroll;
    int vscroll;
    int lastCursorPos;

    enum { UndoAct, RedoAct, CutAct, CopyAct, PasteAct, ClearAct, SelectAllAct, NCountActs };
#ifndef QT_NO_MENU
    QAction *actions[NCountActs];
#endif

    inline void emitCursorPositionChanged();
    bool sendMouseEventToInputContext(QMouseEvent *e);

    void finishChange(int validateFromState = -1, bool update = false, bool edited = true);

    QPointer<QValidator> validator;
    struct MaskInputData {
        enum Casemode { NoCaseMode, Upper, Lower };
        QChar maskChar; // either the separator char or the inputmask
        bool separator;
        Casemode caseMode;
    };
    QString inputMask;
    QChar blank;
    MaskInputData *maskData;
    inline int nextMaskBlank(int pos) {
        int c = findInMask(pos, true, false);
        separator |= (c != pos);
        return (c != -1 ?  c : maxLength);
    }
    inline int prevMaskBlank(int pos) {
        int c = findInMask(pos, false, false);
        separator |= (c != pos);
        return (c != -1 ? c : 0);
    }

    void setCursorVisible(bool visible);


    // undo/redo handling
    enum CommandType { Separator, Insert, Remove, Delete, RemoveSelection, DeleteSelection, SetSelection };
    struct Command {
        inline Command() {}
        inline Command(CommandType t, int p, QChar c, int ss, int se) : type(t),uc(c),pos(p),selStart(ss),selEnd(se) {}
        uint type : 4;
        QChar uc;
        int pos, selStart, selEnd;
    };
    int modifiedState;
    int undoState;
    QVector<Command> history;
    void addCommand(const Command& cmd);
    void insert(const secqstring& s);
    void del(bool wasBackspace = false);
    void remove(int pos);

    inline void separate() { separator = true; }
    void undo(int until = -1);
    void redo();
    inline bool isUndoAvailable() const { return !readOnly && undoState; }
    inline bool isRedoAvailable() const { return !readOnly && undoState < (int)history.size(); }

    // selection
    int selstart, selend;
    inline bool allSelected() const { return !text.empty() && selstart == 0 && selend == (int)text.size(); }
    inline bool hasSelectedText() const { return !text.empty() && selend > selstart; }
    inline void deselect() { selDirty |= (selend > selstart); selstart = selend = 0; }
    void removeSelectedText();
#ifndef QT_NO_CLIPBOARD
    void copy(bool clipboard = true) const;
#endif
    inline bool inSelection(int x) const
    { if (selstart >= selend) return false;
    int pos = xToPos(x, QTextLine::CursorOnCharacter);  return pos >= selstart && pos < selend; }

    // masking
    void parseInputMask(const QString &maskFields);
    bool isValidInput(QChar key, QChar mask) const;
    bool hasAcceptableInput(const secqstring &text) const;
    secqstring maskString(uint pos, const secqstring &str, bool clear = false) const;
    secqstring clearString(uint pos, uint len) const;
    secqstring stripString(const secqstring &str) const;
    int findInMask(int pos, bool forward, bool findSeparator, QChar searchChar = QChar()) const;

    // input methods
    bool composeMode() const { return !textLayout.preeditAreaText().isEmpty(); }

    // complex text layout
    QTextLayout textLayout;
    void updateTextLayout();
    void moveCursor(int pos, bool mark = false);
    void setText(const secqstring& txt, int pos = -1, bool edited = true);
    int xToPos(int x, QTextLine::CursorPosition = QTextLine::CursorBetweenCharacters) const;
    QRect cursorRect() const;
    bool fixup();

    QRect adjustedContentsRect() const;

#ifndef QT_NO_DRAGANDDROP
    // drag and drop
    QPoint dndPos;
    QBasicTimer dndTimer;
    void drag();
#endif

    void _q_clipboardChanged();
    void _q_handleWindowActivate();
    void _q_deleteSelected();
    bool userInput;
    bool emitingEditingFinished;

#ifdef QT_KEYPAD_NAVIGATION
    QBasicTimer deleteAllTimer; // keypad navigation
    QString origText;
#endif

    bool resumePassword;

#ifndef QT_NO_COMPLETER
    QPointer<QCompleter> completer;
    void complete(int key = -1);
    void _q_completionHighlighted(QString);
    bool advanceToEnabledItem(int n);
#endif
};

#endif // QT_NO_LINEEDIT

QT_END_NAMESPACE

#endif // QSECURELINEEDIT_P_H
