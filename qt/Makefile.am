# Makefile.am
# Copyright (C) 2002 g10 Code GmbH, Klarälvdalens Datakonsult AB
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

EXTRA_DIST = Manifest README.SecQ

bin_PROGRAMS = pinentry-qt

if FALLBACK_CURSES
ncurses_include = $(NCURSES_INCLUDE)
libcurses = ../pinentry/libpinentry-curses.a $(LIBCURSES) $(LIBICONV)
else
ncurses_include =
libcurses =
endif


AM_CPPFLAGS = -I$(top_srcdir)/assuan -I$(top_srcdir)/secmem $(QT_INCLUDES) $(ncurses_include) \
	-I$(top_srcdir)/pinentry
AM_CXXFLAGS = $(QT_CXXFLAGS)
pinentry_qt_LDADD = $(QT_LIBS) $(libcurses) ../pinentry/libpinentry.a \
	$(top_builddir)/assuan/libassuan.a \
	$(top_builddir)/secmem/libsecmem.a $(LIBCAP)
pinentry_qt_LDFLAGS = $(QT_LDFLAGS) $(QT_RPATH)

pinentry_qt_SOURCES = secqstring.h secqstring.cpp \
	secqinternal_p.h secqinternal.cpp \
	secqlineedit.h secqlineedit.cpp \
	pinentrydialog.h pinentrydialog.cpp \
	main.cpp 
nodist_pinentry_qt_SOURCES = secqlineedit.moc.cpp \
	pinentrydialog.moc.cpp

DISTCLEANFILES = $(nodist_pinentry_qt_SOURCES)

%.moc.cpp: $(srcdir)/%.h
	$(MOC) $< -o $@
