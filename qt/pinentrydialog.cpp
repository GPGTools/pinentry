/* pinentrydialog.cpp - A (not yet) secure Qt 4 dialog for PIN entry.
 * Copyright (C) 2002, 2008 Klarälvdalens Datakonsult AB (KDAB)
 * Copyright 2007 Ingo Klöcker
 * Copyright 2016 Intevation GmbH
 *
 * Written by Steffen Hansen <steffen@klaralvdalens-datakonsult.se>.
 * Modified by Andre Heinecke <aheinecke@intevation.de>
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
 * SPDX-License-Identifier: GPL-2.0+
 */

#include "pinentrydialog.h"
#include <QGridLayout>

#include <QProgressBar>
#include <QApplication>
#include <QStyle>
#include <QPainter>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QKeyEvent>
#include <QLabel>
#include <QPalette>
#include <QLineEdit>
#include <QAction>
#include <QCheckBox>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

/* I [wk] have no idea for what this code was supposed to do.
   Foregrounding a window is heavily restricted by modern Windows
   versions.  This is the reason why gpg-agent employs its
   AllowSetForegroundWindow callback machinery to ask the supposed to
   be be calling process to allow a pinentry to go into the
   foreground.

   [ah] This is a Hack to workaround the fact that Foregrounding
   a Window is so restricted that it AllowSetForegroundWindow
   does not always work (e.g. when the ForegroundWindow timeout
   has not expired.
   */
#ifdef Q_OS_WIN
WINBOOL SetForegroundWindowEx(HWND hWnd)
{
    //Attach foreground window thread to our thread
    const DWORD ForeGroundID = GetWindowThreadProcessId(::GetForegroundWindow(), NULL);
    const DWORD CurrentID   = GetCurrentThreadId();
    WINBOOL retval;

    AttachThreadInput(ForeGroundID, CurrentID, TRUE);
    //Do our stuff here
    HWND hLastActivePopupWnd = GetLastActivePopup(hWnd);
    retval = SetForegroundWindow(hLastActivePopupWnd);

    //Detach the attached thread
    AttachThreadInput(ForeGroundID, CurrentID, FALSE);
    return retval;
}// End SetForegroundWindowEx
#endif

void raiseWindow(QWidget *w)
{
    /* Maybe Qt will become aggressive enough one day that
     * this is enough on windows too*/
    w->raise();
#ifdef Q_OS_WIN
    HWND wid = (HWND)w->effectiveWinId();
    /* In the meantime we do our own attention grabbing */
    if (!SetForegroundWindow(wid) && !SetForegroundWindowEx(wid)) {
        OutputDebugString("SetForegroundWindow (ex) failed");
        /* Yet another fallback which will not work on some
         * versions and is not recommended by msdn */
        if (!ShowWindow(wid, SW_SHOWNORMAL)) {
            OutputDebugString("ShowWindow failed.");
        }
    }
    /* Even if SetForgeoundWindow / SetForegroundWinowEx don't fail
     * we sometimes are still not in the foreground. So we try yet
     * another hack by using SetWindowPos */
    if (!SetWindowPos(wid, HWND_TOPMOST, 0, 0, 0, 0,
                      SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW)) {
        OutputDebugString("SetWindowPos failed.");
    } else {
        /* Without moving back to NOTOPMOST we just stay on top.
         * Even if the user changes focus. */
        SetWindowPos(wid, HWND_NOTOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    }
#endif
}

QPixmap icon(QStyle::StandardPixmap which)
{
    QPixmap pm = qApp->windowIcon().pixmap(48, 48);

    if (which != QStyle::SP_CustomBase) {
        const QIcon ic = qApp->style()->standardIcon(which);
        QPainter painter(&pm);
        const int emblemSize = 22;
        painter.drawPixmap(pm.width() - emblemSize, 0,
                           ic.pixmap(emblemSize, emblemSize));
    }

    return pm;
}

void PinEntryDialog::slotTimeout()
{
    _timed_out = true;
    reject();
}

PinEntryDialog::PinEntryDialog(QWidget *parent, const char *name,
                               int timeout, bool modal, bool enable_quality_bar,
                               const QString &repeatString,
                               const QString &visibilityTT,
                               const QString &hideTT)
    : QDialog(parent, Qt::WindowStaysOnTopHint),
      mRepeat(NULL),
      _grabbed(false),
      mVisibilityTT(visibilityTT),
      mHideTT(hideTT),
      mVisiActionEdit(NULL),
      mVisiCB(NULL)
{
    _timed_out = false;
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    if (modal) {
        setWindowModality(Qt::ApplicationModal);
    }

    _icon = new QLabel(this);
    _icon->setPixmap(icon());

    _error = new QLabel(this);
    QPalette pal;
    pal.setColor(QPalette::WindowText, Qt::red);
    _error->setPalette(pal);
    _error->hide();

    _desc = new QLabel(this);
    _desc->hide();

    _prompt = new QLabel(this);
    _prompt->hide();

    _edit = new QLineEdit(this);
    _edit->setMaxLength(256);
    _edit->setEchoMode(QLineEdit::Password);

    _prompt->setBuddy(_edit);

    if (enable_quality_bar) {
        _quality_bar_label = new QLabel(this);
        _quality_bar_label->setAlignment(Qt::AlignVCenter);
        _quality_bar = new QProgressBar(this);
        _quality_bar->setAlignment(Qt::AlignCenter);
        _have_quality_bar = true;
    } else {
        _have_quality_bar = false;
    }

    QDialogButtonBox *const buttons = new QDialogButtonBox(this);
    buttons->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    _ok = buttons->button(QDialogButtonBox::Ok);
    _cancel = buttons->button(QDialogButtonBox::Cancel);

    _ok->setDefault(true);

    if (style()->styleHint(QStyle::SH_DialogButtonBox_ButtonsHaveIcons)) {
        _ok->setIcon(style()->standardIcon(QStyle::SP_DialogOkButton));
        _cancel->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));
    }

    if (timeout > 0) {
        _timer = new QTimer(this);
        connect(_timer, SIGNAL(timeout()), this, SLOT(slotTimeout()));
        _timer->start(timeout * 1000);
    } else {
        _timer = NULL;
    }

    connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
    connect(_edit, SIGNAL(textChanged(QString)),
            this, SLOT(updateQuality(QString)));
    connect(_edit, SIGNAL(textChanged(QString)),
            this, SLOT(textChanged(QString)));

    _edit->setFocus();

    QGridLayout *const grid = new QGridLayout(this);
    int row = 1;
    grid->addWidget(_error, row++, 1, 1, 2);
    grid->addWidget(_desc,  row++, 1, 1, 2);
    //grid->addItem( new QSpacerItem( 0, _edit->height() / 10, QSizePolicy::Minimum, QSizePolicy::Fixed ), 1, 1 );
    grid->addWidget(_prompt, row, 1);
    grid->addWidget(_edit, row++, 2);
    if (!repeatString.isNull()) {
        mRepeat = new QLineEdit;
        mRepeat->setMaxLength(256);
        mRepeat->setEchoMode(QLineEdit::Password);
        connect(mRepeat, SIGNAL(textChanged(QString)),
                this, SLOT(textChanged(QString)));
        QLabel *repeatLabel = new QLabel(repeatString);
        repeatLabel->setBuddy(mRepeat);
        grid->addWidget(repeatLabel, row, 1);
        grid->addWidget(mRepeat, row++, 2);
        setTabOrder(_edit, mRepeat);
        setTabOrder(mRepeat, _ok);
    }
    if (enable_quality_bar) {
        grid->addWidget(_quality_bar_label, row, 1);
        grid->addWidget(_quality_bar, row++, 2);
    }
    /* Set up the show password action */
    const QIcon visibilityIcon = QIcon::fromTheme(QLatin1String("visibility"));
    const QIcon hideIcon = QIcon::fromTheme(QLatin1String("hint"));
#if QT_VERSION >= 0x050200
    if (!visibilityIcon.isNull() && !hideIcon.isNull()) {
        mVisiActionEdit = _edit->addAction(visibilityIcon, QLineEdit::TrailingPosition);
        mVisiActionEdit->setVisible(false);
        mVisiActionEdit->setToolTip(mVisibilityTT);
        connect(mVisiActionEdit, SIGNAL(triggered()), this, SLOT(toggleVisibility()));
    } else
#endif
    {
        if (!mVisibilityTT.isNull()) {
            mVisiCB = new QCheckBox(mVisibilityTT);
            connect(mVisiCB, SIGNAL(toggled(bool)), this, SLOT(toggleVisibility()));
            grid->addWidget(mVisiCB, row++, 1, 1, 2, Qt::AlignLeft);
        }
    }
    grid->addWidget(buttons, ++row, 0, 1, 3);

    grid->addWidget(_icon, 0, 0, row - 1, 1, Qt::AlignVCenter | Qt::AlignLeft);

    grid->setSizeConstraint(QLayout::SetFixedSize);


    connect(qApp, SIGNAL(focusChanged(QWidget *, QWidget *)),
            this, SLOT(focusChanged(QWidget *, QWidget *)));
}

void PinEntryDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    raiseWindow(this);
}

void PinEntryDialog::setDescription(const QString &txt)
{
    _desc->setVisible(!txt.isEmpty());
    _desc->setText(txt);
#ifndef QT_NO_ACCESSIBILITY
    _desc->setAccessibleDescription(txt);
#endif
    _icon->setPixmap(icon());
    setError(QString::null);
}

QString PinEntryDialog::description() const
{
    return _desc->text();
}

void PinEntryDialog::setError(const QString &txt)
{
    if (!txt.isNull()) {
        _icon->setPixmap(icon(QStyle::SP_MessageBoxCritical));
    }
    _error->setText(txt);
#ifndef QT_NO_ACCESSIBILITY
    _error->setAccessibleDescription(txt);
#endif
    _error->setVisible(!txt.isEmpty());
}

QString PinEntryDialog::error() const
{
    return _error->text();
}

void PinEntryDialog::setPin(const QString &txt)
{
    _edit->setText(txt);
}

QString PinEntryDialog::pin() const
{
    return _edit->text();
}

void PinEntryDialog::setPrompt(const QString &txt)
{
    _prompt->setText(txt);
    _prompt->setVisible(!txt.isEmpty());
}

QString PinEntryDialog::prompt() const
{
    return _prompt->text();
}

void PinEntryDialog::setOkText(const QString &txt)
{
    _ok->setText(txt);
#ifndef QT_NO_ACCESSIBILITY
    _ok->setAccessibleDescription(txt);
#endif
    _ok->setVisible(!txt.isEmpty());
}

void PinEntryDialog::setCancelText(const QString &txt)
{
    _cancel->setText(txt);
#ifndef QT_NO_ACCESSIBILITY
    _cancel->setAccessibleDescription(txt);
#endif
    _cancel->setVisible(!txt.isEmpty());
}

void PinEntryDialog::setQualityBar(const QString &txt)
{
    if (_have_quality_bar) {
        _quality_bar_label->setText(txt);
#ifndef QT_NO_ACCESSIBILITY
        _quality_bar_label->setAccessibleDescription(txt);
#endif
    }
}

void PinEntryDialog::setQualityBarTT(const QString &txt)
{
    if (_have_quality_bar) {
        _quality_bar->setToolTip(txt);
    }
}

void PinEntryDialog::updateQuality(const QString &txt)
{
    int length;
    int percent;
    QPalette pal;

    if (_timer) {
        _timer->stop();
    }

    if (!_have_quality_bar || !_pinentry_info) {
        return;
    }
    const QByteArray utf8_pin = txt.toUtf8();
    const char *pin = utf8_pin.constData();
    length = strlen(pin);
    percent = length ? pinentry_inq_quality(_pinentry_info, pin, length) : 0;
    if (!length) {
        _quality_bar->reset();
    } else {
        pal = _quality_bar->palette();
        if (percent < 0) {
            pal.setColor(QPalette::Highlight, QColor("red"));
            percent = -percent;
        } else {
            pal.setColor(QPalette::Highlight, QColor("green"));
        }
        _quality_bar->setPalette(pal);
        _quality_bar->setValue(percent);
    }
}

void PinEntryDialog::setPinentryInfo(pinentry_t peinfo)
{
    _pinentry_info = peinfo;
}

void PinEntryDialog::focusChanged(QWidget *old, QWidget *now)
{
    // Grab keyboard. It might be a little weird to do it here, but it works!
    // Previously this code was in showEvent, but that did not work in Qt4.
    if (!_pinentry_info || _pinentry_info->grab) {
        if (_grabbed && old && (old == _edit || old == mRepeat)) {
            old->releaseKeyboard();
            _grabbed = false;
        }
        if (!_grabbed && now && (now == _edit || now == mRepeat)) {
            now->grabKeyboard();
            _grabbed = true;
        }
    }

}

void PinEntryDialog::textChanged(const QString &text)
{
    Q_UNUSED(text);
    if (mRepeat && mRepeat->text() == _edit->text()) {
        _ok->setEnabled(true);
        _ok->setToolTip(QString());
    } else if (mRepeat) {
        _ok->setEnabled(false);
        _ok->setToolTip(mRepeatError);
    }

    if (mVisiActionEdit && sender() == _edit) {
        mVisiActionEdit->setVisible(!_edit->text().isEmpty());
    }
}

void PinEntryDialog::toggleVisibility()
{
    if (sender() == mVisiActionEdit) {
        if (_edit->echoMode() == QLineEdit::Password) {
            mVisiActionEdit->setIcon(QIcon::fromTheme(QLatin1String("hint")));
            mVisiActionEdit->setToolTip(mHideTT);
            _edit->setEchoMode(QLineEdit::Normal);
            if (mRepeat) {
                mRepeat->setEchoMode(QLineEdit::Normal);
            }
        } else {
            mVisiActionEdit->setIcon(QIcon::fromTheme(QLatin1String("visibility")));
            mVisiActionEdit->setToolTip(mVisibilityTT);
            _edit->setEchoMode(QLineEdit::Password);
            if (mRepeat) {
                mRepeat->setEchoMode(QLineEdit::Password);
            }
        }
    }
    if (sender() == mVisiCB) {
        if (mVisiCB->isChecked()) {
            if (mRepeat) {
                mRepeat->setEchoMode(QLineEdit::Normal);
            }
            _edit->setEchoMode(QLineEdit::Normal);
        } else {
            if (mRepeat) {
                mRepeat->setEchoMode(QLineEdit::Password);
            }
            _edit->setEchoMode(QLineEdit::Password);
        }
    }
}

QString PinEntryDialog::repeatedPin() const
{
    if (mRepeat) {
        return mRepeat->text();
    }
    return QString();
}

bool PinEntryDialog::timedOut() const
{
    return _timed_out;
}

void PinEntryDialog::setRepeatErrorText(const QString &err)
{
    mRepeatError = err;
}
#include "pinentrydialog.moc"
