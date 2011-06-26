# Makefile.am
# Copyright (C) 2002 g10 Code GmbH, Klarälvdalens Datakonsult AB
# Copyright (C) 2008 g10 Code GmbH
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

bin_PROGRAMS = pinentry-qt4

EXTRA_DIST = document-encrypt.png pinentry.qrc


if FALLBACK_CURSES
ncurses_include = $(NCURSES_INCLUDE)
libcurses = ../pinentry/libpinentry-curses.a $(LIBCURSES) $(LIBICONV)
else
ncurses_include =
libcurses =
endif


AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/assuan -I$(top_srcdir)/secmem \
	$(ncurses_include) -I$(top_srcdir)/pinentry
AM_CXXFLAGS = $(QT4_CORE_CFLAGS) $(QT4_GUI_CFLAGS)
pinentry_qt4_LDADD = $(QT4_CORE_LIBS) $(QT4_GUI_LIBS) $(libcurses) \
	../pinentry/libpinentry.a $(top_builddir)/assuan/libassuan.a \
	$(top_builddir)/secmem/libsecmem.a $(LIBCAP)

pinentry_qt4_SOURCES = pinentrydialog.h pinentrydialog.cpp \
	main.cpp secstring.h secstring.cpp qsecurelineedit.h \
	qsecurelineedit.cpp pinentrydialog.moc qsecurelineedit.moc \
	qrc_pinentry.cpp \
	qsecurelineedit_p.h \
	pinentryconfirm.cpp pinentryconfirm.h pinentryconfirm.moc
