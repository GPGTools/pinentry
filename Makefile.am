# Makefile.am
# Copyright (C) 2002, 2012, 2015 g10 Code GmbH
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

# Location of the released tarball archives.  This is prefixed by
# the variable RELEASE_ARCHIVE in ~/.gnupg-autogen.rc.  For example:
# RELEASE_ARCHIVE=wk@somehost:archive/tarballs
RELEASE_ARCHIVE_SUFFIX = pinentry

ACLOCAL_AMFLAGS = -I m4
DISTCHECK_CONFIGURE_FLAGS = --disable-pinentry-qt --enable-pinentry-emacs

GITLOG_TO_CHANGELOG=gitlog-to-changelog

EXTRA_DIST = autogen.sh autogen.rc README.GIT ChangeLog-old VERSION \
	     build-aux/gitlog-to-changelog \
	     build-aux/git-log-fix build-aux/git-log-footer

if BUILD_PINENTRY_CURSES
pinentry_curses = curses
else
pinentry_curses =
endif

if BUILD_PINENTRY_TTY
pinentry_tty = tty
else
pinentry_tty =
endif

if BUILD_PINENTRY_EMACS
pinentry_emacs = emacs
else
pinentry_emacs =
endif

if BUILD_PINENTRY_GTK_2
pinentry_gtk_2 = gtk+-2
else
pinentry_gtk_2 =
endif

if BUILD_PINENTRY_GNOME_3
pinentry_gnome_3 = gnome3
else
pinentry_gnome_3 =
endif

if BUILD_PINENTRY_QT6
pinentry_qt = qt
else
pinentry_qt =
endif

if BUILD_PINENTRY_QT5
pinentry_qt5 = qt5
else
pinentry_qt5 =
endif

if BUILD_PINENTRY_QT4
pinentry_qt4 = qt4
else
pinentry_qt4 =
endif

if BUILD_PINENTRY_TQT
pinentry_tqt = tqt
else
pinentry_tqt =
endif

if BUILD_PINENTRY_W32
pinentry_w32 = w32
else
pinentry_w32 =
endif

if BUILD_PINENTRY_FLTK
pinentry_fltk = fltk
else
pinentry_fltk =
endif

if BUILD_PINENTRY_EFL
pinentry_efl = efl
else
pinentry_efl =
endif

if BUILD_DOC
doc = doc
else
doc =
endif

SUBDIRS = m4 secmem pinentry ${pinentry_curses} ${pinentry_tty} \
	${pinentry_emacs} ${pinentry_gtk_2} ${pinentry_gnome_3} \
	${pinentry_qt} ${pinentry_qt5} ${pinentry_qt4}  ${pinentry_tqt} ${pinentry_w32} \
	${pinentry_fltk} ${pinentry_efl} ${doc}


install-exec-local:
	@list='$(bin_PROGRAMS)'; for p in $$list; do \
	  echo "  $(SETCAP) cap_ipc_lock+p $(DESTDIR)$(bindir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`"; \
	  $(SETCAP) cap_ipc_lock+p $(DESTDIR)$(bindir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'` || true; \
	done

	(cd $(DESTDIR)$(bindir); \
	rm -f pinentry$(EXEEXT); \
	$(LN_S) $(PINENTRY_DEFAULT)$(EXEEXT) pinentry$(EXEEXT))

dist-hook: gen-ChangeLog

distcheck-hook:
	set -e; ( \
	pref="#+macro: pinentry_" ;\
	reldate="$$(date -u +%Y-%m-%d)" ;\
        echo "$${pref}ver  $(PACKAGE_VERSION)"  ;\
        echo "$${pref}date $${reldate}" ;\
        list='$(DIST_ARCHIVES)'; for i in $$list; do \
	  case "$$i" in *.tar.bz2) \
            echo "$${pref}size $$(wc -c <$$i|awk '{print int($$1/1024)}')k" ;\
	    echo "$${pref}sha1 $$(sha1sum <$$i|cut -d' ' -f1)" ;\
	    echo "$${pref}sha2 $$(sha256sum <$$i|cut -d' ' -f1)" ;;\
	  esac;\
	done ) | tee $(distdir).swdb


gen_start_date = 2012-08-08T00:00:00
.PHONY: gen-ChangeLog
gen-ChangeLog:
	if test -d $(top_srcdir)/.git; then				\
	  (cd $(top_srcdir) &&                      			\
	    $(GITLOG_TO_CHANGELOG) --append-dot --tear-off		\
	    --amend=build-aux/git-log-fix				\
	    --since=$(gen_start_date) ) > $(distdir)/cl-t;		\
          cat $(top_srcdir)/build-aux/git-log-footer >> $(distdir)/cl-t;\
	  rm -f $(distdir)/ChangeLog;					\
	  mv $(distdir)/cl-t $(distdir)/ChangeLog;			\
	fi

# Macro to help the release target.
RELEASE_NAME = $(PACKAGE_TARNAME)-$(PACKAGE_VERSION)

release:
	+(set -e;\
	 if [ "$(abs_top_builddir)" = "$(abs_top_srcdir)" ]; then \
           echo "error: build directory must not be the source directory" >&2;\
	   exit 2;\
         fi ;\
	 echo "/* Build started at $$(date -uIseconds) */" ;\
	 cd $(top_srcdir); \
	 ./autogen.sh --force; \
	 cd $(abs_top_builddir); \
	 rm -rf dist; mkdir dist ; cd dist ; \
	 $(abs_top_srcdir)/configure --enable-maintainer-mode; \
	 $(MAKE) distcheck; \
	 echo "/* Build finished at $$(date -uIseconds) */" ;\
         echo "/*" ;\
	 echo " * Please run the final step interactivly:" ;\
	 echo " *   make sign-release" ;\
	 echo " */" ;\
	) 2>&1 | tee "$(RELEASE_NAME).buildlog"

sign-release:
	 +(set -e; \
	  test $$(pwd | sed 's,.*/,,') = dist || cd dist; \
	  x=$$(grep '^[[:blank:]]*RELEASE_ARCHIVE[[:blank:]]*=' \
                     $$HOME/.gnupg-autogen.rc|cut -d= -f2|xargs);\
          if [ -z "$$x" ]; then \
             echo "error: RELEASE_ARCHIVE missing in ~/.gnupg-autogen.rc">&2; \
             exit 2;\
          fi;\
          myarchive="$$x/$(RELEASE_ARCHIVE_SUFFIX)";\
	  x=$$(grep '^[[:blank:]]*RELEASE_SIGNKEY[[:blank:]]*=' \
                     $$HOME/.gnupg-autogen.rc|cut -d= -f2|xargs);\
          if [ -z "$$x" ]; then \
             echo "error: RELEASE_SIGNKEY missing in ~/.gnupg-autogen.rc">&2; \
             exit 2;\
          fi;\
	  mysignkey="$$x";\
	  files1="$(RELEASE_NAME).tar.bz2" ; \
	  files2="$(RELEASE_NAME).tar.bz2.sig \
		  $(RELEASE_NAME).swdb \
		  $(RELEASE_NAME).buildlog" ;\
	  echo "/* Signing the source tarball ..." ;\
	  gpg -sbu $$mysignkey $(RELEASE_NAME).tar.bz2 ;\
	  cat $(RELEASE_NAME).swdb >swdb.snippet;\
	  echo >>swdb.snippet ;\
	  sha1sum $${files1} >>swdb.snippet ;\
          cat "../$(RELEASE_NAME).buildlog" swdb.snippet \
               | gzip >$(RELEASE_NAME).buildlog ;\
          echo "Copying to archive $$myarchive ..." ;\
	  scp -p $${files1} $${files2} $${myarchive}/ || true;\
	  echo '/*' ;\
	  echo ' * All done; for checksums see dist/swdb.snippet' ;\
	  echo ' */' ;\
	 )
