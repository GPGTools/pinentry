# Makefile.am - PIN entry GTK+ frontend.
# Copyright (C) 2002, 2015 g10 Code GmbH
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

bin_PROGRAMS = pinentry-gnome3

if FALLBACK_CURSES
ncurses_include = $(NCURSES_INCLUDE)
libcurses = ../pinentry/libpinentry-curses.a $(LIBCURSES) $(LIBICONV)
else
ncurses_include =
libcurses =
endif

AM_CPPFLAGS = $(COMMON_CFLAGS) $(GNOME3_CFLAGS) \
	$(ncurses_include) -I$(top_srcdir)/secmem -I$(top_srcdir)/pinentry
LDADD = ../pinentry/libpinentry.a ../secmem/libsecmem.a \
	$(COMMON_LIBS) $(GNOME3_LIBS) $(libcurses)

pinentry_gnome3_SOURCES = pinentry-gnome3.c
