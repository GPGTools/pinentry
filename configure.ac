# configure.ac
# Copyright (C) 1999 Robert Bihlmeyer <robbe@orcus.priv.at>
# Copyright (C) 2001, 2002, 2003, 2004 g10 Code GmbH
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
 
# (Process this file with autoconf to produce a configure script.)
AC_PREREQ(2.57)
min_automake_version="1.7.6"

# Remember to remove the "-cvs" suffix *before* a release and to bump the
# version number immediately *after* a release and to re-append the suffix.
AC_INIT(pinentry, 0.7.2-cvs, [gpa-dev@gnupg.org])
AM_CONFIG_HEADER(config.h)
AC_CONFIG_SRCDIR(pinentry/pinentry.h)
AM_INIT_AUTOMAKE($PACKAGE_NAME, $PACKAGE_VERSION)
AH_TOP([
/* We need this, because some autoconf tests rely on this (e.g. stpcpy)
   and it should be used for new programs anyway */
#define _GNU_SOURCE  1
])
AM_MAINTAINER_MODE

AC_CANONICAL_HOST

dnl Checks for programs.
missing_dir=`cd $ac_aux_dir && pwd`
AM_MISSING_PROG(ACLOCAL, aclocal, $missing_dir)
AM_MISSING_PROG(AUTOCONF, autoconf, $missing_dir)
AM_MISSING_PROG(AUTOMAKE, automake, $missing_dir)
AM_MISSING_PROG(AUTOHEADER, autoheader, $missing_dir)
AM_MISSING_PROG(MAKEINFO, makeinfo, $missing_dir)
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_RANLIB
# We need to check for cplusplus here becuase we may not do the test
# for Qt and autoconf does does not allow that.
AC_PROG_CXX

dnl Checks for compiler features.
if test "$GCC" = yes; then
    CFLAGS="$CFLAGS -Wall -Wcast-align -Wshadow -Wstrict-prototypes"
    CPPFLAGS="$CPPFLAGS -Wall"
fi

dnl Checks for library functions.
AC_CHECK_FUNCS(seteuid stpcpy mmap)
GNUPG_CHECK_MLOCK

dnl Checks for libassuan.
dnl -> None required becuase we use a stripped down version of libassuan.


dnl Checks for libsecmem.
GNUPG_CHECK_TYPEDEF(byte, HAVE_BYTE_TYPEDEF)
GNUPG_CHECK_TYPEDEF(ulong, HAVE_ULONG_TYPEDEF)

AC_PATH_PROG(SETCAP, setcap, :, "$PATH:/sbin:/usr/sbin")
AC_CHECK_LIB(cap, cap_set_proc, [
  AC_DEFINE(USE_CAPABILITIES,1,[The capabilities support library is installed])
  LIBCAP=-lcap
])
AC_SUBST(LIBCAP)

dnl
dnl Check for curses pinentry program.
dnl
AC_ARG_ENABLE(pinentry-curses,
            AC_HELP_STRING([--enable-pinentry-curses], [build curses pinentry]),
            pinentry_curses=$enableval, pinentry_curses=maybe)
AC_ARG_ENABLE(fallback-curses,
            AC_HELP_STRING([--enable-fallback-curses], [include curses fallback]),
            fallback_curses=$enableval, fallback_curses=maybe)

dnl
dnl Checks for curses libraries.  Deal correctly with $pinentry_curses = maybe
dnl and $fallback_curses = maybe.
dnl
if test "$pinentry_curses" != "no" -o "$fallback_curses" != "no"; then
  IU_LIB_CURSES
fi
if test "$LIBCURSES"; then
  if test "$pinentry_curses" != "no"; then
    pinentry_curses=yes
  fi
  if test "$fallback_curses" != "no"; then
    fallback_curses=yes
    AC_DEFINE(FALLBACK_CURSES, 1,
	      [The GUI pinentries should fall back to curses if X is not available.])
  fi
else
  if test "$pinentry_curses" = "yes" -o "$fallback_curses" = "yes"; then
    AC_MSG_ERROR([[
*** The curses library is required. The latest version of
*** ncurses is always available from ftp://ftp.gnu.org/gnu/ncurses/.
]])
  fi
  pinentry_curses=no
  fallback_curses=no
fi
AM_CONDITIONAL(BUILD_LIBPINENTRY_CURSES, test "$pinentry_curses" = "yes" -o "$fallback_curses" = "yes")
AM_CONDITIONAL(BUILD_PINENTRY_CURSES, test "$pinentry_curses" = "yes")
AM_CONDITIONAL(FALLBACK_CURSES, test "$fallback_curses" = "yes")

if test "$pinentry_curses" = "yes"; then
  AC_DEFINE(PINENTRY_CURSES, 1, [The Curses version of Pinentry is to be build])
fi


dnl
dnl Check for GTK+ pinentry program.
dnl
AC_ARG_ENABLE(pinentry-gtk,
            AC_HELP_STRING([--enable-pinentry-gtk], [build GTK+ pinentry]),
            pinentry_gtk=$enableval, pinentry_gtk=maybe)

dnl
dnl Checks for GTK+ libraries.  Deal correctly with $pinentry_gtk = maybe.
dnl
if test "$pinentry_gtk" != "no"; then
AM_PATH_GLIB(1.2.0,,
if test "$pinentry_gtk" = "yes"; then
    AC_MSG_ERROR([[
*** GLIB 1.2.0 or newer is required. The latest version of GLIB
*** is always available from ftp://ftp.gtk.org/.
]])
else
    pinentry_gtk=no
fi)
fi

if test "$pinentry_gtk" != "no"; then
AM_PATH_GTK(1.2.0,
    AC_DEFINE(HAVE_GTK, 1, [Defined if GTK is available]),
if test "$pinentry_gtk" = "yes"; then
    AC_MSG_ERROR([[
*** GTK+ 1.2.0 or newer is required. The latest version of GTK+
*** is always available form ftp://ftp.gtk.org/.
]])
else
    pinentry_gtk=no
fi)
fi

dnl If we have come so far, GTK+ pinentry can be build.
if test "$pinentry_gtk" != "no"; then
    pinentry_gtk=yes
fi
AM_CONDITIONAL(BUILD_PINENTRY_GTK, test "$pinentry_gtk" = "yes")
if test "$pinentry_gtk" = "yes"; then
  AC_DEFINE(PINENTRY_GTK, 1, [The GTK+ version of Pinentry is to be build])
fi

if test "$pinentry_curses" = "yes" \
        -o "$fallback_curses" = "yes" \
        -o "$pinentry_gtk" = "yes"; then
dnl Additional checks pinentry Curses/GTK+.
AM_ICONV
fi

dnl if test "$pinentry_gtk" = "yes"; then
dnl Additional checks for GTK+ pinentry.
dnl End of additional checks for GTK+ pinentry.
dnl fi


dnl
dnl Check for Qt pinentry program.
dnl
AC_ARG_ENABLE(pinentry-qt,
            AC_HELP_STRING([--enable-pinentry-qt], [build Qt pinentry]),
            pinentry_qt=$enableval, pinentry_qt=maybe)
dnl
dnl Checks for Qt libraries.  Deal correctly with $pinentry_qt = maybe.
dnl
if test "$pinentry_qt" != "no"; then
QT_CHECK_COMPILERS
QT_PATH
if test $have_qt = "no"; then
	if test $pinentry_qt = "yes"; then
	  AC_MSG_ERROR([Qt ($qt_minversion) $ac_qt_notfound not found. Please check your installation!
	  For more details about this problem, look at the end of config.log.$missing_qt_mt])
	else
	  pinentry_qt="no"	  
	fi
fi
if test $have_moc = "no"; then
	if test $pinentry_qt = "yes"; then
        AC_MSG_ERROR([No Qt meta object compiler (moc) found!
        Please check whether you installed Qt correctly.
        You need to have a running moc binary.
        configure tried to run $ac_cv_path_moc and the test didn't
        succeed. If configure shouldn't have tried this one, set
        the environment variable MOC to the right one before running
        configure.
        ])
	else
	  pinentry_qt="no"	  
	fi
fi
fi

dnl If we have come so far, Qt pinentry can probably be build.
if test "$pinentry_qt" != "no"; then
    pinentry_qt=yes
fi
AM_CONDITIONAL(BUILD_PINENTRY_QT, test "$pinentry_qt" = "yes")

if test "$pinentry_qt" = "yes"; then
  AC_DEFINE(PINENTRY_QT, 1, [The Qt version of Pinentry is to be build])
fi

dnl if test "$pinentry_qt" = "yes"; then
dnl dnl Additional checks for Qt pinentry.
dnl dnl End of checks for Qt pinentry.
dnl fi


AC_CONFIG_FILES([
assuan/Makefile
secmem/Makefile
pinentry/Makefile
curses/Makefile
gtk/Makefile
qt/Makefile
doc/Makefile
Makefile
])

AC_OUTPUT

AC_MSG_NOTICE([
	Pinentry v${VERSION} has been configured as follows:

	Curses Pinentry: $pinentry_curses
	GTK+ Pinentry: $pinentry_gtk
	Qt Pinentry: $pinentry_qt

	Fallback to Curses: $fallback_curses
])
