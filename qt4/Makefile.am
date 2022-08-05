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

bin_PROGRAMS = pinentry-qt4

EXTRA_DIST = document-encrypt.png pinentry.qrc


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
AM_CXXFLAGS = $(PINENTRY_QT4_CFLAGS)
pinentry_qt4_LDADD = \
	../pinentry/libpinentry.a $(top_builddir)/secmem/libsecmem.a \
	$(COMMON_LIBS) $(PINENTRY_QT4_LIBS) $(libcurses) $(LIBCAP)
pinentry_qt4_LDFLAGS = $(PINENTRY_QT4_LDFLAGS)

if BUILD_PINENTRY_QT4
BUILT_SOURCES = \
	pinentryconfirm.moc pinentrydialog.moc pinlineedit.moc
endif

CLEANFILES = \
	pinentryconfirm.moc pinentrydialog.moc pinlineedit.moc

pinentry_qt4_SOURCES = pinentrydialog.h pinentrydialog.cpp \
	main.cpp qrc_pinentry.cpp pinentryconfirm.cpp pinentryconfirm.h \
	pinlineedit.h pinlineedit.cpp

nodist_pinentry_qt4_SOURCES = \
	pinentryconfirm.moc pinentrydialog.moc pinlineedit.moc

.h.moc:
	$(MOC4) `test -f '$<' || echo '$(srcdir)/'`$< -o $@
