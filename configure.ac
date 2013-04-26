# configure.ac
# Copyright (C) 1999 Robert Bihlmeyer <robbe@orcus.priv.at>
# Copyright (C) 2001, 2002, 2003, 2004, 2007 g10 Code GmbH
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
min_automake_version="1.10"

# To build a release you need to create a tag with the version number
# (git tag -s pinentry-n.m.k) and run "./autogen.sh --force".  Please
# bump the version number immediately after the release, do another
# commit, and a push so that the git magic is able to work.
m4_define(mym4_version, [0.8.3])

# Below is m4 magic to extract and compute the git revision number,
# the decimalized short revision number, a beta version string and a
# flag indicating a development version (mym4_isgit).  Note that the
# m4 processing is done by autoconf and not during the configure run.
m4_define([mym4_revision], m4_esyscmd([git branch -v 2>/dev/null \
          | awk '/^\* / {printf "%s",$3}']))
m4_define([mym4_revision_dec],
          m4_esyscmd_s([echo $((0x$(echo ]mym4_revision[|head -c 4)))]))
m4_define([mym4_betastring],
          m4_esyscmd_s([git describe --match 'pinentry-[0-9].[0-9].*[0-9]' \
                           --long|  awk -F- '$3!=0{print"-beta"$3}']))
m4_define([mym4_isgit],m4_if(mym4_betastring,[],[no],[yes]))
m4_define([mym4_full_version],[mym4_version[]mym4_betastring])

AC_INIT([pinentry],[mym4_full_version], [http://bugs.gnupg.org])

AM_CONFIG_HEADER(config.h)
AC_CONFIG_SRCDIR(pinentry/pinentry.h)
AM_INIT_AUTOMAKE($PACKAGE_NAME, $PACKAGE_VERSION)

AC_GNU_SOURCE

AM_MAINTAINER_MODE

AC_CANONICAL_HOST

AH_TOP([
#ifndef PINENTRY_CONFIG_H_INCLUDED
#define PINENTRY_CONFIG_H_INCLUDED

/* Enable gpg-error's strerror macro under W32CE.  */
#define GPG_ERR_ENABLE_ERRNO_MACROS 1
])

AH_BOTTOM([
#endif /*PINENTRY_CONFIG_H_INCLUDED*/
])


dnl Checks for programs.
AC_PROG_MAKE_SET
AM_SANITY_CHECK
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
# We need to check for cplusplus here because we may not do the test
# for Qt and autoconf does does not allow that.
AC_PROG_CXX
AC_PROG_LN_S
AC_CHECK_TOOL(WINDRES, windres, :)
AC_CHECK_PROGS(GITLOG_TO_CHANGELOG, gitlog-to-changelog,
               [build-aux/gitlog-to-changelog])


have_dosish_system=no
have_w32_system=no
have_w32ce_system=no
case "${host}" in
    *-mingw32*)
        AC_DEFINE(USE_ONLY_8DOT3,1,
                  [Set this to limit filenames to the 8.3 format])
        have_dosish_system=yes
        have_w32_system=yes
        case "${host}" in
          *-mingw32ce*)
            have_w32ce_system=yes
            ;;
          *)
            AC_DEFINE(HAVE_DRIVE_LETTERS,1,
                      [Defined if the OS supports drive letters.])
            ;;
        esac
        ;;
    i?86-emx-os2 | i?86-*-os2*emx )
        # OS/2 with the EMX environment
        AC_DEFINE(HAVE_DRIVE_LETTERS)
        have_dosish_system=yes
        ;;

    i?86-*-msdosdjgpp*)
        # DOS with the DJGPP environment
        AC_DEFINE(HAVE_DRIVE_LETTERS)
        have_dosish_system=yes
        ;;
esac

if test "$have_dosish_system" = yes; then
   AC_DEFINE(HAVE_DOSISH_SYSTEM,1,
             [Defined if we run on some of the PCDOS like systems
              (DOS, Windoze. OS/2) with special properties like
              no file modes])
fi
AM_CONDITIONAL(HAVE_DOSISH_SYSTEM, test "$have_dosish_system" = yes)

if test "$have_w32_system" = yes; then
   AC_DEFINE(HAVE_W32_SYSTEM,1, [Defined if we run on a W32 API based system])
   if test "$have_w32ce_system" = yes; then
      AC_DEFINE(HAVE_W32CE_SYSTEM,1,[Defined if we run on WindowsCE])
   fi
fi
AM_CONDITIONAL(HAVE_W32_SYSTEM, test "$have_w32_system" = yes)
AM_CONDITIONAL(HAVE_W32CE_SYSTEM, test "$have_w32ce_system" = yes)




dnl Checks for compiler features.
if test "$GCC" = yes; then
    CFLAGS="$CFLAGS -Wall -Wcast-align -Wshadow -Wstrict-prototypes"
    CPPFLAGS="$CPPFLAGS -Wall"

    AC_MSG_CHECKING([if gcc supports -Wno-pointer-sign])
    _gcc_cflags_save=$CFLAGS
    CFLAGS="-Wno-pointer-sign"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],[])],_gcc_psign=yes,_gcc_psign=no)
    AC_MSG_RESULT($_gcc_psign)
    CFLAGS=$_gcc_cflags_save;
    if test x"$_gcc_psign" = xyes ; then
       CFLAGS="$CFLAGS -Wno-pointer-sign"
    fi
fi


# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(string.h unistd.h langinfo.h termio.h locale.h utime.h wchar.h)

dnl Checks for library functions.
AC_CHECK_FUNCS(seteuid stpcpy mmap)
GNUPG_CHECK_MLOCK

dnl Checks for libassuan.
dnl -> None required becuase we use a stripped down version of libassuan.


dnl Checks for libsecmem.
GNUPG_CHECK_TYPEDEF(byte, HAVE_BYTE_TYPEDEF)
GNUPG_CHECK_TYPEDEF(ulong, HAVE_ULONG_TYPEDEF)

dnl Check for libcap
AC_ARG_WITH([libcap], AC_HELP_STRING([--without-libcap],
            [Disable support for capabilities library]))

if test "x$with_libcap" != "xno"; then
  AC_PATH_PROG(SETCAP, setcap, :, "$PATH:/sbin:/usr/sbin")
  AC_CHECK_LIB(cap, cap_set_proc, [
    AC_DEFINE(USE_CAPABILITIES,1,[The capabilities support library is installed])
    LIBCAP=-lcap
  ])
fi
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
***
*** The curses library is required. The latest version of
*** ncurses is always available from ftp://ftp.gnu.org/gnu/ncurses/.
***]])
  fi
  pinentry_curses=no
  fallback_curses=no
fi
AM_CONDITIONAL(BUILD_LIBPINENTRY_CURSES,
              test "$pinentry_curses" = "yes" -o "$fallback_curses" = "yes")
AM_CONDITIONAL(BUILD_PINENTRY_CURSES, test "$pinentry_curses" = "yes")
AM_CONDITIONAL(FALLBACK_CURSES, test "$fallback_curses" = "yes")

if test "$pinentry_curses" = "yes"; then
  AC_DEFINE(PINENTRY_CURSES, 1,
           [The Curses version of Pinentry is to be build])
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
***
*** GLIB 1.2.0 or newer is required. The latest version of GLIB
*** is always available from ftp://ftp.gtk.org/.
***]])
else
    pinentry_gtk=no
fi)
fi

if test "$pinentry_gtk" != "no"; then
AM_PATH_GTK(1.2.0,
    AC_DEFINE(HAVE_GTK, 1, [Defined if GTK is available]),
if test "$pinentry_gtk" = "yes"; then
    AC_MSG_ERROR([[
***
*** GTK+ 1.2.0 or newer is required. The latest version of GTK+
*** is always available form ftp://ftp.gtk.org/.
***]])
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
  if test "$am_cv_func_iconv" != "yes"; then
    AC_MSG_ERROR([[
***
*** The iconv function is required.  You can specify its location
*** using the --with-libiconv-prefix=PREFIX option to configure.
***]])
  fi
fi

dnl if test "$pinentry_gtk" = "yes"; then
dnl Additional checks for GTK+ pinentry.
dnl End of additional checks for GTK+ pinentry.
dnl fi


dnl
dnl Check for GTK+-2 pinentry program.
dnl
AC_ARG_ENABLE(pinentry-gtk2,
            AC_HELP_STRING([--enable-pinentry-gtk2], [build GTK+-2 pinentry]),
            pinentry_gtk_2=$enableval, pinentry_gtk_2=maybe)

dnl check for pkg-config
if test "$pinentry_gtk_2" != "no"; then
        AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
	if test x"${PKG_CONFIG}" = xno ; then
		pinentry_gtk_2=no
	fi
fi

dnl check if the module gtk+-2.0 exists
if test "$pinentry_gtk_2" != "no"; then
	AC_MSG_CHECKING([for gtk+-2])
	"${PKG_CONFIG}" --exists gtk+-2.0
	if test $? -ne 0 ; then
		AC_MSG_RESULT([no])
		AC_MSG_WARN([pkg-config could not find the module gtk+-2.0])
		pinentry_gtk_2=no
	else
		AC_MSG_RESULT([yes])
		AC_MSG_CHECKING([gtk+-2 version >= 2.4.0])
		modvers=`"${PKG_CONFIG}" --modversion gtk+-2.0`
		AC_MSG_RESULT([$modvers])
		"${PKG_CONFIG}" --atleast-version=2.4.0 gtk+-2.0
		if test $? -ne 0 ; then
			AC_MSG_WARN([building GTK+-2 pinentry disabled])
			pinentry_gtk_2=no
		else
			GTK2CFLAGS=`"${PKG_CONFIG}" --cflags gtk+-2.0`
			GTK2LIBS=`"${PKG_CONFIG}" --libs gtk+-2.0`
			AC_SUBST(GTK2CFLAGS)
			AC_SUBST(GTK2LIBS)
			pinentry_gtk_2=yes
		fi
	fi
fi
AM_CONDITIONAL(BUILD_PINENTRY_GTK_2, test "$pinentry_gtk_2" = "yes")


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
	  AC_MSG_ERROR([[
***
*** Qt ($qt_minversion) $ac_qt_notfound not found. Please check your
*** installation!  For more details about this problem, look at the
*** end of config.log.$missing_qt_mt
***]])
	else
	  pinentry_qt="no"
	fi
fi
if test $have_moc = "no"; then
	if test $pinentry_qt = "yes"; then
        AC_MSG_ERROR([[
***
*** No Qt meta object compiler (moc) found!
*** Please check whether you installed Qt correctly.
*** You need to have a running moc binary.
*** configure tried to run $ac_cv_path_moc and the test didn't
*** succeed. If configure shouldn't have tried this one, set
*** the environment variable MOC to the right one before running
*** configure.
***]])
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


dnl
dnl Check for Qt4 pinentry program.
dnl
AC_ARG_ENABLE(pinentry-qt4,
            AC_HELP_STRING([--enable-pinentry-qt4], [build Qt4 pinentry]),
            pinentry_qt4=$enableval, pinentry_qt4=maybe)


dnl
dnl Checks for Qt4 libraries.  Deal correctly with $pinentry_qt4 = maybe.
dnl
if test "$pinentry_qt4" != "no"; then
PKG_CHECK_MODULES(QT4_CORE, QtCore,,
if test "$pinentry_qt4" = "yes"; then
    AC_MSG_ERROR([[
***
*** Qt4 Core is required.
***]])
else
    pinentry_qt4=no
fi)
fi

if test "$pinentry_qt4" != "no"; then
PKG_CHECK_MODULES(QT4_GUI, QtGui,,
if test "$pinentry_qt4" = "yes"; then
    AC_MSG_ERROR([[
***
*** Qt4 Gui is required.
***]])
else
    pinentry_qt4=no
fi)
fi

dnl If we have come so far, Qt4 pinentry can be build.
if test "$pinentry_qt4" != "no"; then
    pinentry_qt4=yes
fi
AM_CONDITIONAL(BUILD_PINENTRY_QT4, test "$pinentry_qt4" = "yes")
if test "$pinentry_qt4" = "yes"; then
  AC_DEFINE(PINENTRY_QT4, 1, [The Qt4 version of Pinentry is to be build])
fi

dnl if test "$pinentry_qt4" = "yes"; then
dnl Additional checks for Qt4 pinentry.
dnl End of additional checks for Qt4 pinentry.
dnl fi


#
# Check whether we should build the W32 pinentry.  This is actually
# the simplest check as we do this only for that platform.
#
pinentry_w32=no
test $have_w32_system = yes && pinentry_w32=yes
AM_CONDITIONAL(BUILD_PINENTRY_W32, test "$pinentry_w32" = "yes")


# Figure out the default pinentry.  We are very conservative here.
# Please change the order only after verifying that the preferred
# pinentry really is better (more feature-complete and more secure).

if test "$pinentry_gtk_2" = "yes"; then
   PINENTRY_DEFAULT=pinentry-gtk-2
else
  if test "$pinentry_gtk" = "yes"; then
    PINENTRY_DEFAULT=pinentry-gtk
  else
    if test "$pinentry_qt" = "yes"; then
      PINENTRY_DEFAULT=pinentry-qt
    else
      if test "$pinentry_qt4" = "yes"; then
        PINENTRY_DEFAULT=pinentry-qt4
      else
        if test "$pinentry_curses" = "yes"; then
          PINENTRY_DEFAULT=pinentry-curses
        else
          if test "$pinentry_w32" = "yes"; then
            PINENTRY_DEFAULT=pinentry-w32
          else
            AC_MSG_ERROR([[No pinentry enabled.]])
          fi
        fi
      fi
    fi
  fi
fi
AC_SUBST(PINENTRY_DEFAULT)


AC_CONFIG_FILES([
assuan/Makefile
secmem/Makefile
pinentry/Makefile
curses/Makefile
gtk/Makefile
gtk+-2/Makefile
qt/Makefile
qt4/Makefile
w32/Makefile
doc/Makefile
Makefile
])

AC_OUTPUT

AC_MSG_NOTICE([

	Pinentry v${VERSION} has been configured as follows:

	Revision:  mym4_revision  (mym4_revision_dec)
	Platform:  $host

	Curses Pinentry ..: $pinentry_curses
	GTK+ Pinentry ....: $pinentry_gtk
	GTK+-2 Pinentry ..: $pinentry_gtk_2
	Qt Pinentry ......: $pinentry_qt
	Qt4 Pinentry .....: $pinentry_qt4
	W32 Pinentry .....: $pinentry_w32

	Fallback to Curses: $fallback_curses

	Default Pinentry .: $PINENTRY_DEFAULT
])
