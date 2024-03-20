/* capslock_unix.cpp - Helper to check whether Caps Lock is on
 * Copyright (C) 2021, 2024 g10 Code GmbH
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

#include "capslock.h"
#include "capslock_p.h"

#include <QDebug>
#include <QGuiApplication>

LockState capsLockState()
{
    static bool reportUnsupportedPlatform = true;

#if PINENTRY_KGUIADDONS
    if (qApp->platformName() != QLatin1String("wayland") && qApp->platformName() != QLatin1String("xcb") && reportUnsupportedPlatform) {
#else
    if (reportUnsupportedPlatform) {
#endif
        qWarning() << "Checking for Caps Lock not possible on unsupported platform:" << qApp->platformName();
    }
    reportUnsupportedPlatform = false;
#if PINENTRY_KGUIADDONS
    static KModifierKeyInfo keyInfo;
    return keyInfo.isKeyLocked(Qt::Key_CapsLock) ? LockState::On : LockState::Off;
#endif
    return LockState::Unknown;
}

#if PINENTRY_KGUIADDONS
void CapsLockWatcher::Private::watch()
{
    keyInfo = new KModifierKeyInfo();
    connect(keyInfo, &KModifierKeyInfo::keyLocked, q, [=](Qt::Key key, bool locked){
        if (key == Qt::Key_CapsLock) {
            Q_EMIT q->stateChanged(locked);
        }
    });
}
#endif
