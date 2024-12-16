/* pinentryconfirm.h - A QMessageBox with a timeout
 *
 * Copyright (C) 2011 Ben Kibbey <bjk@luxsci.net>
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

#ifndef PINENTRYCONFIRM_H
#define PINENTRYCONFIRM_H

#include <QMessageBox>
#include <QTimer>

class PinentryConfirm : public QMessageBox
{
    Q_OBJECT
public:
    PinentryConfirm(Icon, int timeout, const QString &title,
                    const QString &desc, StandardButtons buttons,
                    QWidget *parent);
    bool timedOut() const;

private slots:
    void slotTimeout();

private:
    QTimer *_timer;
    bool _timed_out;

protected:
    /* reimp */ void showEvent(QShowEvent *event);
};

#endif
