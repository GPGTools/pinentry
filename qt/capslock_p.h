/* capslock_p.h - Helper to check whether Caps Lock is on
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

#ifndef __PINENTRY_QT_CAPSLOCK_P_H__
#define __PINENTRY_QT_CAPSLOCK_P_H__

#include "capslock.h"

#ifdef PINENTRY_QT_WAYLAND
namespace KWayland
{
namespace Client
{
class Registry;
class Seat;
}
}
#endif

class CapsLockWatcher::Private
{
public:
    explicit Private(CapsLockWatcher *);
#ifdef PINENTRY_QT_WAYLAND
    void watchWayland();
#endif

private:
#ifdef PINENTRY_QT_WAYLAND
    void registry_seatAnnounced(quint32, quint32);
    void seat_hasKeyboardChanged(bool);
    void keyboard_modifiersChanged(quint32);
#endif

private:
    CapsLockWatcher *const q;

#ifdef PINENTRY_QT_WAYLAND
    KWayland::Client::Registry *registry = nullptr;
    KWayland::Client::Seat *seat = nullptr;
#endif
};

#endif // __PINENTRY_QT_CAPSLOCK_P_H__
