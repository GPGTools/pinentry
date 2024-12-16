# Makefile.am - PIN entry FLTK frontend.
# SPDX-License-Identifier: GPL-2.0+

bin_PROGRAMS = pinentry-fltk

if FALLBACK_CURSES
ncurses_include = $(NCURSES_INCLUDE)
libcurses = ../pinentry/libpinentry-curses.a $(LIBCURSES) $(LIBICONV)
else
ncurses_include =
libcurses =
endif

AM_CPPFLAGS = $(COMMON_CFLAGS) $(FLTKCXXFLAGS) $(ncurses_include) -I$(top_srcdir)/secmem -I$(top_srcdir)/pinentry
AM_CXXFLAGS = -std=c++11
LDADD = ../pinentry/libpinentry.a ../secmem/libsecmem.a $(COMMON_LIBS) $(FLTKLIBS) $(libcurses)

pinentry_fltk_SOURCES = main.cxx pinwindow.cxx pinwindow.h \
			passwindow.cxx passwindow.h \
			qualitypasswindow.cxx qualitypasswindow.h

EXTRA_DIST = encrypt.xpm icon.xpm
