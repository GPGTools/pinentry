/* capslock.h - Helper to check whether Caps Lock is on
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <QGuiApplication>

#ifdef PINENTRY_QT_X11
# include <QX11Info>
# include <X11/XKBlib.h>
# undef Status
#endif

#include <QDebug>

bool capsLockIsOn()
{
    static bool reportUnsupportedPlatform = true;
    if (qApp) {
#ifdef PINENTRY_QT_X11
        if (qApp->platformName() == QLatin1String("xcb")) {
            unsigned int state;
            XkbGetIndicatorState(QX11Info::display(), XkbUseCoreKbd, &state);
            return (state & 0x01) == 1;
        }
#endif
        if (reportUnsupportedPlatform) {
            qWarning() << "Checking for Caps Lock not possible on unsupported platform:" << qApp->platformName();
            reportUnsupportedPlatform = false;
        }
    }
    return false;
}
