# configure.ac
# Copyright (C) 1999 Robert Bihlmeyer <robbe@orcus.priv.at>
# Copyright (C) 2001, 2002 g10 Code GmbH
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
AC_REVISION($Revision$)

AC_PREREQ(2.52)
AC_INIT(pinentry, 0.6, [gpa-dev@gnupg.org])
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

dnl Checks for compiler features.
if test "$GCC" = yes; then
    CFLAGS="$CFLAGS -Wall -Wcast-align -Wshadow -Wstrict-prototypes"
fi

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_RANLIB

dnl Checks for library functions.
AC_CHECK_FUNCS(seteuid stpcpy)
GNUPG_CHECK_MLOCK

dnl Checks for libassuan.
AC_CHECK_FUNCS(fopencookie,,[
    AC_MSG_ERROR([[
***
*** fopencookie(3) is needed to build this package.
*** We will provide an replacement in a later release.
***]])])

dnl Checks for libsecmem.
GNUPG_CHECK_TYPEDEF(byte, HAVE_BYTE_TYPEDEF)
GNUPG_CHECK_TYPEDEF(ulong, HAVE_ULONG_TYPEDEF)


dnl
dnl Check for curses pinentry program.
dnl
AC_ARG_ENABLE(pinentry-curses,
            AC_HELP_STRING([--enable-pinentry-curses], [build curses pinentry]),
            pinentry_curses=$enableval, pinentry_curses=maybe)

dnl
dnl Checks for curses libraries.  Deal correctly with $pinentry_curses = maybe.
dnl
if test "$pinentry_curses" != "no"; then
  IU_LIB_CURSES
fi
if test "$pinentry_curses" != no -a "$LIBCURSES"; then
  pinentry_curses=yes
else
  if test "$pinentry_curses" = yes; then
    AC_MSG_ERROR([[
*** The curses library is required. The latest version of
*** ncurses is always available from ftp://ftp.gnu.org/gnu/ncurses/.
]])
  fi
  pinentry_curses=no
fi
AM_CONDITIONAL(BUILD_PINENTRY_CURSES, test "$pinentry_curses" = "yes")


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
dnl Additional checks for GTK+ pinentry.
AC_PATH_PROG(SETCAP, setcap, :, "$PATH:/sbin:/usr/sbin")
AC_CHECK_LIB(cap, cap_set_proc, [
  AC_DEFINE(USE_CAPABILITIES,1,[The capabilities support library is installed])
  LIBCAP=-lcap
])
AC_SUBST(LIBCAP)

AC_CHECK_HEADERS(inttypes.h, , need_inttypes=yes)
if test x$need_inttypes = xyes; then
  AC_CHECK_SIZEOF(unsigned int, 4)
  AC_CHECK_SIZEOF(unsigned long, 4)
fi
dnl End of additional checks for GTK+ pinentry.
fi


dnl
dnl Check for KDE pinentry program.
dnl
AC_ARG_ENABLE(pinentry-kde,
            AC_HELP_STRING([--enable-pinentry-kde], [build KDE pinentry]),
            pinentry_kde=$enableval, pinentry_kde=no)
dnl
dnl Checks for KDE libraries.  Deal correctly with $pinentry_kde = maybe.
dnl
if test "$pinentry_kde" != "no"; then
AC_CHECK_COMPILERS
AC_PATH_KDE
fi

dnl If we have come so far, KDE pinentry can probably be build.
if test "$pinentry_kde" != "no"; then
    pinentry_kde=yes
fi
AM_CONDITIONAL(BUILD_PINENTRY_KDE, test "$pinentry_kde" = "yes")

dnl if test "$pinentry_kde" = "yes"; then
dnl dnl Additional checks for KDE pinentry.
dnl dnl End of checks for KDE pinentry.
dnl fi


AC_CONFIG_FILES([
assuan/Makefile
secmem/Makefile
pinentry/Makefile
curses/Makefile
gtk/Makefile
kde/Makefile
Makefile
])

AC_OUTPUT

AC_MSG_NOTICE([
	Pinentry v${VERSION} has been configured as follows:

	Curses Pinentry: $pinentry_curses
	GTK+ Pinentry: $pinentry_gtk
	KDE Pinentry: $pinentry_kde
])
