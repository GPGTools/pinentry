/* accessibility.h - Helpers for making pinentry accessible
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

#ifndef __PINENTRY_QT_ACCESSIBILITY_H__
#define __PINENTRY_QT_ACCESSIBILITY_H__

class QString;
class QWidget;

namespace Accessibility
{

/* Wrapper for QWidget::setAccessibleDescription which does nothing if
   QT_NO_ACCESSIBILITY is defined. */
void setDescription(QWidget *w, const QString &text);

/* Wrapper for QWidget::setAccessibleName which does nothing if
   QT_NO_ACCESSIBILITY is defined. */
void setName(QWidget *w, const QString &text);

} // namespace Accessibility

#endif // __PINENTRY_QT_ACCESSIBILITY_H__
