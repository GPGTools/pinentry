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

bin_PROGRAMS = pinentry-kde

AM_CPPFLAGS = -I$(top_srcdir)/assuan -I$(top_srcdir)/secmem $(all_includes)
pinentry_kde_LDADD = $(top_builddir)/assuan/libassuan.a \
	$(top_builddir)/secmem/libsecmem.a $(LIBCAP) \
	$(LIB_QT) # $(LIB_KDECORE) $(LIB_KDEUI)
pinentry_kde_LDFLAGS = $(all_libraries) $(KDE_RPATH) 

pinentry_kde_SOURCES = pinentrydialog.h pinentrydialog.cpp \
	pinentrycontroller.h pinentrycontroller.cpp \
	cppmemory.cpp main.cpp 
nodist_pinentry_kde_SOURCES = pinentrydialog.moc.cpp pinentrycontroller.moc.cpp

EXTRA_DIST = pinentry.desktop  

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(kde_appsdir)/Applications/
	$(INSTALL_DATA) $(srcdir)/pinentry.desktop \
		$(DESTDIR)$(kde_appsdir)/Applications/pinentry.desktop

uninstall-local:
	-rm -f $(DESTDIR)$(kde_appsdir)/Applications/pinentry.desktop

%.moc.cpp: $(srcdir)/%.h
	$(MOC) $< -o $@
