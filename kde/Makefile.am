# Makefile.am
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

# XXX This should be probed for.
#KDE_CXXFLAGS = $(USE_EXCEPTIONS)
CXXFLAGS = -O2 -fno-exceptions -fno-check-new -fexceptions

# XXX The prefix KDE was installed with
XXX_PREFIX = /usr/local

# XXX The defines that will make it compile, link and run if you have a reasonable system.
XXX_KDE_DEFINES = -I$(XXX_PREFIX)/include -I/usr/X11R6/include -DQT_THREAD_SUPPORT  -D_REENTRANT
LIB_QT = -lqt-mt -lpng -lz -lm -ljpeg -lXext -lX11 -lresolv -lSM -lICE \
	-lpthread -lXrender -lstdc++ -lm -lc -lgcc
LIB_KDECORE = -lkdecore
LIB_KDEUI = -lkdeui
KDE_RPATH = -L$(XXX_PREFIX)/lib -L/usr/X11R6/lib \
	 -Wl,--rpath -Wl,$(XXX_PREFIX)/lib -Wl,--rpath -Wl,/usr/X11R6/lib
MOC = $(XXX_PREFIX)/bin/moc
kde_appsdir = $(datadir)/applnk

bin_PROGRAMS = pinentry-kde

# Set the include path for X, qt, KDE and the included libs.
AM_CPPFLAGS = -I$(top_srcdir)/assuan -I$(top_srcdir)/secmem $(XXX_KDE_DEFINES)
pinentry_kde_LDADD = $(top_builddir)/assuan/libassuan.a \
	$(top_builddir)/secmem/libsecmem.a $(LIBCAP) \
	$(LIB_QT) $(LIB_KDECORE) $(LIB_KDEUI)
pinentry_kde_LDFLAGS = $(KDE_RPATH) 

pinentry_kde_SOURCES = pinentrydialog.h pinentrydialog.cpp pinentrydialog.moc.cpp \
	pinentrycontroller.h pinentrycontroller.cpp pinentrycontroller.moc.cpp \
	cppmemory.cpp main.cpp 

EXTRA_DIST = pinentry.desktop  

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(kde_appsdir)/Applications/
	$(INSTALL_DATA) $(srcdir)/pinentry.desktop $(DESTDIR)$(kde_appsdir)/Applications/pinentry.desktop

uninstall-local:
	-rm -f $(DESTDIR)$(kde_appsdir)/Applications/pinentry.desktop

%.moc.cpp: $(srcdir)/%.h
	$(MOC) $< -o $@
