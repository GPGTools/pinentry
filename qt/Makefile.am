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

bin_PROGRAMS = pinentry-qt

if FALLBACK_CURSES
ncurses_include = -I$(top_srcdir)/pinentry $(NCURSES_INCLUDE)
libcurses = ../pinentry/libpinentry-curses.a ../pinentry/libpinentry.a $(LIBCURSES) $(LIBICONV)
else
ncurses_include =
libcurses =
endif


AM_CPPFLAGS = -I$(top_srcdir)/assuan -I$(top_srcdir)/secmem $(QT_INCLUDES) $(ncurses_include)
AM_CXXFLAGS = $(QT_CXXFLAGS)
pinentry_qt_LDADD = $(top_builddir)/assuan/libassuan.a \
	$(top_builddir)/secmem/libsecmem.a $(LIBCAP) \
	$(QT_LIBS) $(libcurses)
pinentry_qt_LDFLAGS = $(QT_LDFLAGS)

pinentry_qt_SOURCES = pinentrydialog.h pinentrydialog.cpp \
	pinentrycontroller.h pinentrycontroller.cpp \
	cppmemory.cpp main.cpp 
nodist_pinentry_qt_SOURCES = pinentrydialog.moc.cpp pinentrycontroller.moc.cpp

DISTCLEANFILES = $(nodist_pinentry_qt_SOURCES)

%.moc.cpp: $(srcdir)/%.h
	$(MOC) $< -o $@
