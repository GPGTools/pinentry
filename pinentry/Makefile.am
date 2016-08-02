# Pinentry support library Makefile
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

EXTRA_DIST =

if BUILD_LIBPINENTRY_CURSES
pinentry_curses = libpinentry-curses.a
else
pinentry_curses =
endif

if BUILD_LIBPINENTRY_EMACS
pinentry_emacs_sources = pinentry-emacs.h pinentry-emacs.c
else
pinentry_emacs_sources =
endif

noinst_LIBRARIES = libpinentry.a $(pinentry_curses)

LDADD = $(COMMON_LIBS)
AM_CPPFLAGS = $(COMMON_CFLAGS) -I$(top_srcdir)/secmem

libpinentry_a_SOURCES = pinentry.h pinentry.c argparse.c argparse.h \
	password-cache.h password-cache.c $(pinentry_emacs_sources)
libpinentry_curses_a_SOURCES = pinentry-curses.h pinentry-curses.c
libpinentry_curses_a_CFLAGS = @NCURSES_CFLAGS@
