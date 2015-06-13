# Makefile.am - PIN entry curses frontend.
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
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA

## Process this file with automake to produce Makefile.in

bin_PROGRAMS = pinentry-curses

AM_CPPFLAGS = $(COMMON_CFLAGS) $(NCURSES_INCLUDE) -I$(top_srcdir)/pinentry
LDADD = ../pinentry/libpinentry.a ../pinentry/libpinentry-curses.a \
	../secmem/libsecmem.a $(COMMON_LIBS) $(LIBCAP) $(LIBCURSES) $(LIBICONV)

pinentry_curses_SOURCES = pinentry-curses.c
