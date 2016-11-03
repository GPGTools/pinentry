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
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA

## Process this file with automake to produce Makefile.in

bin_PROGRAMS = pinentry-qt

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
AM_CXXFLAGS = $(PINENTRY_QT_CFLAGS)
pinentry_qt_LDADD = \
	../pinentry/libpinentry.a $(top_builddir)/secmem/libsecmem.a \
	$(COMMON_LIBS) $(PINENTRY_QT_LIBS) $(libcurses) $(LIBCAP)

BUILT_SOURCES = \
	pinentryconfirm.moc pinentrydialog.moc

CLEANFILES = \
	pinentryconfirm.moc pinentrydialog.moc

pinentry_qt_SOURCES = pinentrydialog.h pinentrydialog.cpp \
	main.cpp qrc_pinentry.cpp pinentryconfirm.cpp pinentryconfirm.h

nodist_pinentry_qt_SOURCES = \
	pinentryconfirm.moc pinentrydialog.moc

.h.moc:
	$(MOC) `test -f '$<' || echo '$(srcdir)/'`$< -o $@
