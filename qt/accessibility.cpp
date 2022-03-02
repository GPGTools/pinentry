/* accessibility.cpp - Helpers for making pinentry accessible
 * Copyright (C) 2021 g10 Code GmbH
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

#include "accessibility.h"

#include <QLabel>
#include <QString>
#include <QTextDocument>
#include <QWidget>

#include "pinentry_debug.h"

namespace Accessibility
{

void setDescription(QWidget *w, const QString &text)
{
    if (w) {
#ifndef QT_NO_ACCESSIBILITY
        w->setAccessibleDescription(text);
#endif
    }
}

void setName(QWidget *w, const QString &text)
{
    if (w) {
#ifndef QT_NO_ACCESSIBILITY
        w->setAccessibleName(text);
#endif
    }
}

void selectLabelText(QLabel *label)
{
    if (!label || label->text().isEmpty()) {
        return;
    }
    if (label->textFormat() == Qt::PlainText) {
        label->setSelection(0, label->text().size());
    } else if (label->textFormat() == Qt::RichText) {
        // unfortunately, there is no selectAll(); therefore, we need
        // to determine the "visual" length of the text by stripping
        // the label's text of all formatting information
        QTextDocument temp;
        temp.setHtml(label->text());
        label->setSelection(0, temp.toRawText().size());
    } else {
        qDebug(PINENTRY_LOG) << "Label with unsupported text format" << label->textFormat() << "got focus";
    }
}

} // namespace Accessibility
