# Makefile.am - PIN entry W32
# Copyright (C) 2004 g10 Code GmbH
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

logos = logo-32.bmp logo-48.bmp logo-64.bmp logo-96.bmp logo-128.bmp

EXTRA_DIST = $(logos)

bin_PROGRAMS = pinentry-w32

AM_CPPFLAGS = $(COMMON_CFLAGS) -I$(top_srcdir)/secmem \
	      -I$(top_srcdir)/pinentry


pinentry_w32_SOURCES = main.c pinentry-w32.rc resource.h

# Note: For testing you should add -mconsole to LDFLAGS.
pinentry_w32_LDFLAGS = -mwindows
pinentry_w32_LDADD = pinentry-w32.o \
	../pinentry/libpinentry.a ../secmem/libsecmem.a \
        $(COMMON_LIBS)

pinentry-w32.o: pinentry-w32.rc resource.h $(logos)
	$(WINDRES) -I.. -v -o $@ $<

