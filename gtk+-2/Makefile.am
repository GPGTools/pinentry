# Makefile.am - PIN entry GTK+ frontend.
# Copyright (C) 2002 g10 Code GmbH
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

bin_PROGRAMS = pinentry-gtk-2

if FALLBACK_CURSES
ncurses_include = $(NCURSES_INCLUDE)
libcurses = ../pinentry/libpinentry-curses.a $(LIBCURSES) $(LIBICONV)
else
ncurses_include =
libcurses =
endif

AM_CPPFLAGS = $(GTK2CFLAGS) $(ncurses_include) \
	-I$(top_srcdir)/secmem -I$(top_srcdir)/pinentry
LDADD = ../pinentry/libpinentry.a ../assuan/libassuan.a ../secmem/libsecmem.a \
	$(LIBCAP) $(GTK2LIBS) $(libcurses)

pinentry_gtk_2_SOURCES = pinentry-gtk-2.c \
	gtksecentry.c gtksecentry.h gseal-gtk-compat.h
