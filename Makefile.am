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

EXTRA_DIST = autogen.sh 

if BUILD_PINENTRY_CURSES
pinentry_curses = curses
else
pinentry_curses = 
endif

if BUILD_PINENTRY_GTK
pinentry_gtk = gtk
else
pinentry_gtk = 
endif

if BUILD_PINENTRY_QT
pinentry_qt = qt
else
pinentry_qt = 
endif

SUBDIRS = assuan secmem pinentry ${pinentry_curses} \
	${pinentry_gtk} ${pinentry_qt}

signed-dist: $(distdir).tar.gz.sig

%.sig: %
	gpg -sbav -u 0x57548DCD $@ $<

install-exec-local:
	@list='$(bin_PROGRAMS)'; for p in $$list; do \
	  echo "  $(SETCAP) cap_ipc_lock+p $(DESTDIR)$(bindir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`"; \
	  $(SETCAP) cap_ipc_lock+p $(DESTDIR)$(bindir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'` || true; \
	done
