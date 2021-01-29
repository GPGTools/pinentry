/* pinlineedit.cpp - Modified QLineEdit widget.
 * Copyright (C) 2018 Damien Goutte-Gattat
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 * SPDX-License-Identifier: GPL-2.0+
 */

#include "pinlineedit.h"

#include <QKeyEvent>

PinLineEdit::PinLineEdit(QWidget *parent) : QLineEdit(parent)
{
}

void
PinLineEdit::keyPressEvent(QKeyEvent *e)
{
    QLineEdit::keyPressEvent(e);

    if ( e->key() == Qt::Key::Key_Backspace )
	emit backspacePressed();
}

#include "pinlineedit.moc"
