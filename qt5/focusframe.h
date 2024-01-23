/* focusframe.h - A focus indicator for labels.
 * Copyright (C) 2022 g10 Code GmbH
 *
 * Software engineering by Ingo Klöcker <dev@ingo-kloecker.de>
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

#ifndef __FOCUSFRAME_H__
#define __FOCUSFRAME_H__

#include <QFocusFrame>

class FocusFrame : public QFocusFrame
{
    Q_OBJECT
public:
    using QFocusFrame::QFocusFrame;

protected:
    void paintEvent(QPaintEvent *event) override;
};

#endif // __FOCUSFRAME_H__
