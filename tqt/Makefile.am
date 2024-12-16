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

bin_PROGRAMS = pinentry-tqt


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
AM_CXXFLAGS = $(PINENTRY_TQT_CFLAGS)
pinentry_tqt_LDADD = \
	../pinentry/libpinentry.a $(top_builddir)/secmem/libsecmem.a \
	$(COMMON_LIBS) $(PINENTRY_TQT_LIBS) $(libcurses)

if BUILD_PINENTRY_TQT
BUILT_SOURCES = \
	secqlineedit.moc pinentrydialog.moc
endif

CLEANFILES = \
	secqlineedit.moc pinentrydialog.moc

pinentry_tqt_SOURCES = pinentrydialog.h pinentrydialog.cpp \
	main.cpp secqinternal_p.h secqinternal.cpp \
	secqlineedit.h secqlineedit.cpp \
	secqstring.h secqstring.cpp

nodist_pinentry_tqt_SOURCES = \
	secqlineedit.moc pinentrydialog.moc

.h.moc:
	$(TQT_MOC) `test -f '$<' || echo '$(srcdir)/'`$< -o $@
