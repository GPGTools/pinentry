# Makefile.am
# Copyright (C) 2002 g10 Code GmbH, Klarälvdalens Datakonsult AB
# Copyright (C) 2008, 2015 g10 Code GmbH
#
# This file is part of PINENTRY.
#
# PINENTRY is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# PINENTRY is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <https://www.gnu.org/licenses/>.
# SPDX-License-Identifier: GPL-2.0+

## Process this file with automake to produce Makefile.in

bin_PROGRAMS = pinentry-qt

EXTRA_DIST = pinentryrc.qrc

SUBDIRS = icons


if FALLBACK_CURSES
ncurses_include = $(NCURSES_INCLUDE)
libcurses = ../pinentry/libpinentry-curses.a $(LIBCURSES) $(LIBICONV)
else
ncurses_include =
libcurses =
endif


AM_CPPFLAGS = $(COMMON_CFLAGS) \
	-I$(top_srcdir) -I$(top_srcdir)/secmem \
	$(ncurses_include) -I$(top_srcdir)/pinentry
AM_CXXFLAGS = $(PINENTRY_QT_CFLAGS)
pinentry_qt_LDADD = \
	../pinentry/libpinentry.a $(top_builddir)/secmem/libsecmem.a \
	$(COMMON_LIBS) $(PINENTRY_QT_LIBS) $(libcurses)
pinentry_qt_LDFLAGS = $(PINENTRY_QT_LDFLAGS)

if BUILD_PINENTRY_QT
BUILT_SOURCES = \
	pinentryconfirm.moc pinentrydialog.moc pinlineedit.moc capslock.moc \
	pinentryrc.cpp
endif

CLEANFILES = $(BUILT_SOURCES)

if HAVE_W32_SYSTEM
pinentry_qt_platform_SOURCES = capslock_win.cpp
else
pinentry_qt_platform_SOURCES = capslock_unix.cpp
endif

pinentry_qt_SOURCES = pinentrydialog.h pinentrydialog.cpp \
	main.cpp pinentryconfirm.cpp pinentryconfirm.h \
	pinlineedit.h pinlineedit.cpp capslock.cpp capslock.h capslock_p.h \
	pinentry_debug.cpp pinentry_debug.h util.h accessibility.cpp \
	accessibility.h qti18n.cpp pinentryrc.qrc \
	$(pinentry_qt_platform_SOURCES)

nodist_pinentry_qt_SOURCES = $(BUILT_SOURCES)

.h.moc:
	$(MOC) `test -f '$<' || echo '$(srcdir)/'`$< -o $@

pinentryrc.cpp: pinentryrc.qrc
	$(RCC) `test -f '$<' || echo '$(srcdir)/'`$< -o $@
