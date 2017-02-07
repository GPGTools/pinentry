# Makefile.am - PIN entry FLTK frontend.

bin_PROGRAMS = pinentry-fltk

if FALLBACK_CURSES
ncurses_include = $(NCURSES_INCLUDE)
libcurses = ../pinentry/libpinentry-curses.a $(LIBCURSES) $(LIBICONV)
else
ncurses_include =
libcurses =
endif

AM_CPPFLAGS = $(COMMON_CFLAGS) $(FLTKCXXFLAGS) $(ncurses_include) -I$(top_srcdir)/secmem -I$(top_srcdir)/pinentry
LDADD = ../pinentry/libpinentry.a ../secmem/libsecmem.a $(COMMON_LIBS) $(LIBCAP) $(FLTKLIBS) $(libcurses)

pinentry_fltk_SOURCES = main.cxx pinwindow.cxx passwindow.cxx qualitypasswindow.cxx
