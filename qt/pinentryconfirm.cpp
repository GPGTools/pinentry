/* pinentryconfirm.cpp - A QMessageBox with a timeout

   Copyright (C) 2011 Ben Kibbey <bjk@luxsci.net>

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
#include "pinentryconfirm.h"
#include "pinentrydialog.h"
#include <QAbstractButton>

PinentryConfirm::PinentryConfirm(Icon icon, int timeout, const QString &title,
                                 const QString &desc, StandardButtons buttons, QWidget *parent) :
    QMessageBox(icon, title, desc, buttons, parent)
{
    _timed_out = false;
    if (timeout > 0) {
        _timer = new QTimer(this);
        connect(_timer, SIGNAL(timeout()), this, SLOT(slotTimeout()));
        _timer->start(timeout * 1000);
    }
#ifndef QT_NO_ACCESSIBILITY
    setAccessibleDescription(desc);
    setAccessibleName(title);
#endif
    raiseWindow(this);
}

bool PinentryConfirm::timedOut() const
{
    return _timed_out;
}

void PinentryConfirm::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    raiseWindow(this);
}

void PinentryConfirm::slotTimeout()
{
    QAbstractButton *b = button(QMessageBox::Cancel);
    _timed_out = true;

    if (b) {
        b->animateClick(0);
    }
}

#include "pinentryconfirm.moc"
