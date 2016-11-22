# configure.ac
# Copyright (C) 1999 Robert Bihlmeyer <robbe@orcus.priv.at>
# Copyright (C) 2001, 2002, 2003, 2004, 2007, 2015, 2016 g10 Code GmbH
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
AC_PREREQ(2.69)
min_automake_version="1.14"

# To build a release you need to create a tag with the version number
# (git tag -s pinentry-n.m.k) and run "./autogen.sh --force".  Please
# bump the version number immediately after the release, do another
# commit, and a push so that the git magic is able to work.
m4_define(mym4_version, [1.0.0])

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

AC_INIT([pinentry],[mym4_full_version], [https://bugs.gnupg.org])

AC_CONFIG_AUX_DIR([build-aux])
AM_CONFIG_HEADER(config.h)
AC_CONFIG_SRCDIR(pinentry/pinentry.h)
AM_INIT_AUTOMAKE([serial-tests dist-bzip2 no-dist-gzip])

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
    # Check whether gcc does not emit a diagnositc for unknown -Wno-*
    # options.  This is the case for gcc >= 4.6
    AC_MSG_CHECKING([if gcc ignores unknown -Wno-* options])
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 6 )
#kickerror
#endif]],[])],[_gcc_silent_wno=yes],[_gcc_silent_wno=no])
    AC_MSG_RESULT($_gcc_silent_wno)

    if test "$USE_MAINTAINER_MODE" = "yes"; then
        CFLAGS="$CFLAGS -Wall -Wcast-align -Wshadow -Wstrict-prototypes"
        CFLAGS="$CFLAGS -Wformat -Wno-format-y2k -Wformat-security"

        if test x"$_gcc_silent_wno" = xyes ; then
          _gcc_warn=yes
        else
          AC_MSG_CHECKING([if gcc supports -Wno-missing-field-initializers])
          _gcc_cflags_save=$CFLAGS
          CFLAGS="-Wno-missing-field-initializers"
          AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],[])],
                            [_gcc_warn=yes],[_gcc_warn=no])
          AC_MSG_RESULT($_gcc_warn)
          CFLAGS=$_gcc_cflags_save;
        fi
        if test x"$_gcc_warn" = xyes ; then
          CFLAGS="$CFLAGS -W -Wno-sign-compare -Wno-missing-field-initializers"
        fi

        AC_MSG_CHECKING([if gcc supports -Wdeclaration-after-statement])
        _gcc_cflags_save=$CFLAGS
        CFLAGS="-Wdeclaration-after-statement"
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],[])],_gcc_warn=yes,_gcc_warn=no)
        AC_MSG_RESULT($_gcc_warn)
        CFLAGS=$_gcc_cflags_save;
        if test x"$_gcc_warn" = xyes ; then
          CFLAGS="$CFLAGS -Wdeclaration-after-statement"
        fi

    else
        # Not in maintainer mode: Use standard warnings.
        CFLAGS="$CFLAGS -Wall"
    fi

    CPPFLAGS="$CPPFLAGS -Wall"

    if test x"$_gcc_silent_wno" = xyes ; then
      _gcc_warn=yes
    else
      AC_MSG_CHECKING([if gcc supports -Wno-pointer-sign])
      _gcc_cflags_save=$CFLAGS
      CFLAGS="-Wno-pointer-sign"
      AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],[])],[_gcc_warn=yes],[_gcc_warn=no])
      AC_MSG_RESULT($_gcc_warn)
      CFLAGS=$_gcc_cflags_save;
    fi
    if test x"$_gcc_warn" = xyes ; then
       CFLAGS="$CFLAGS -Wno-pointer-sign"
    fi

    AC_MSG_CHECKING([if gcc supports -Wpointer-arith])
    _gcc_cflags_save=$CFLAGS
    CFLAGS="-Wpointer-arith"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],[])],_gcc_warn=yes,_gcc_warn=no)
    AC_MSG_RESULT($_gcc_warn)
    CFLAGS=$_gcc_cflags_save;
    if test x"$_gcc_warn" = xyes ; then
       CFLAGS="$CFLAGS -Wpointer-arith"
    fi
fi


# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(string.h unistd.h langinfo.h termio.h locale.h utime.h wchar.h)

dnl Checks for library functions.
AC_CHECK_FUNCS(seteuid stpcpy mmap)
GNUPG_CHECK_MLOCK

dnl Checks for standard types.
AC_TYPE_UINT32_T

# Common libraries and cflags.
COMMON_CFLAGS=
COMMON_LIBS=
AC_SUBST(COMMON_CFLAGS)
AC_SUBST(COMMON_LIBS)

dnl Checks for libgpg-error
#
# libgpg-error is a library with error codes shared between GnuPG
# related projects.
#
NEED_GPG_ERROR_VERSION=1.16
have_gpg_error=no

AM_PATH_GPG_ERROR("$NEED_GPG_ERROR_VERSION",
                  have_gpg_error=yes,have_gpg_error=no)
COMMON_CFLAGS="$GPG_ERROR_CFLAGS $COMMON_CFLAGS"
COMMON_LIBS="$GPG_ERROR_LIBS $COMMON_LIBS"

AC_DEFINE_UNQUOTED(GPG_ERR_ENABLE_GETTEXT_MACROS, 1,
                   [Under Windows we use the gettext code from libgpg-error])

AC_DEFINE_UNQUOTED(GPG_ERR_ENABLE_ERRNO_MACROS, 1,
                   [Under WindowsCE we use the strerror replacement from libgpg-error.])


dnl Checks for libassuan.
#
# libassuan is used for IPC
#
NEED_LIBASSUAN_API=2
NEED_LIBASSUAN_VERSION=2.1.0
have_libassuan=no
AM_PATH_LIBASSUAN("$NEED_LIBASSUAN_API:$NEED_LIBASSUAN_VERSION",
                  have_libassuan=yes,have_libassuan=no)
if test "$have_libassuan" = "yes"; then
  AC_DEFINE_UNQUOTED(GNUPG_LIBASSUAN_VERSION, "$libassuan_version",
                     [version of the libassuan library])
fi

COMMON_CFLAGS="$LIBASSUAN_CFLAGS $COMMON_CFLAGS"
COMMON_LIBS="$LIBASSUAN_LIBS $COMMON_LIBS"


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
dnl Check for tty pinentry program.
dnl
AC_ARG_ENABLE(pinentry-tty,
            AC_HELP_STRING([--enable-pinentry-tty], [build tty pinentry]),
            pinentry_tty=$enableval, pinentry_tty=maybe)
AM_CONDITIONAL(BUILD_PINENTRY_TTY, test "$pinentry_tty" = "yes")

if test "$pinentry_tty" = "yes"; then
  AC_DEFINE(PINENTRY_TTY, 1,
           [The TTY version of Pinentry is to be build])
fi


dnl
dnl Additional checks pinentry Curses.
dnl
if test "$pinentry_curses" = "yes" \
        -o "$fallback_curses" = "yes" ; then
  AM_ICONV
  if test "$am_cv_func_iconv" != "yes"; then
    AC_MSG_ERROR([[
***
*** The iconv function is required.  You can specify its location
*** using the --with-libiconv-prefix=PREFIX option to configure.
***]])
  fi
fi

dnl
dnl Check for emacs pinentry program.
dnl
AC_ARG_ENABLE(pinentry-emacs,
            AC_HELP_STRING([--enable-pinentry-emacs], [build emacs pinentry]),
            pinentry_emacs=$enableval, pinentry_emacs=no)
AC_ARG_ENABLE(inside-emacs,
            AC_HELP_STRING([--enable-inside-emacs], [include emacs hack]),
            inside_emacs=$enableval, inside_emacs=maybe)

if test "$pinentry_emacs" != "no" -o "$inside_emacs" != "no"; then
  AC_MSG_CHECKING([if Unix domain socket is supported])
  AC_TRY_COMPILE([
#include <sys/socket.h>
#include <sys/un.h>
],
	         [int s = socket (AF_UNIX, SOCK_STREAM, 0);],
		 [_unixsock_works=yes],
		 [_unixsock_works=no])
  AC_MSG_RESULT($_unixsock_works)
  if test "$_unixsock_works" = "yes"; then
    if test "$pinentry_emacs" != "no"; then
      pinentry_emacs=yes
    fi
    if test "$inside_emacs" != "no"; then
      inside_emacs=yes
      AC_DEFINE(INSIDE_EMACS, 1,
                [The GUI pinentries should respect INSIDE_EMACS envvar.])
    fi
  else
    if test "$pinentry_emacs" = "yes" -o "$inside_emacs" = "yes"; then
      AC_MSG_ERROR([[
***
*** Support for Unix domain sockets is required.
***]])
    fi
    pinentry_emacs=no
    inside_emacs=no
  fi
fi

AM_CONDITIONAL(BUILD_LIBPINENTRY_EMACS,
              test "$pinentry_emacs" = "yes" -o "$inside_emacs" = "yes")
AM_CONDITIONAL(BUILD_PINENTRY_EMACS, test "$pinentry_emacs" = "yes")
AM_CONDITIONAL(INSIDE_EMACS, test "$inside_emacs" = "yes")

if test "$pinentry_emacs" = "yes"; then
  AC_DEFINE(PINENTRY_EMACS, 1,
            [The Emacs version of Pinentry is to be build])
fi



dnl
dnl Check for GTK+-2 / GNOME3 pinentry programs.
dnl
AC_ARG_ENABLE(pinentry-gtk2,
            AC_HELP_STRING([--enable-pinentry-gtk2], [build GTK+-2 pinentry]),
            pinentry_gtk_2=$enableval, pinentry_gtk_2=maybe)

AC_ARG_ENABLE(pinentry-gnome3,
            AC_HELP_STRING([--enable-pinentry-gnome3], [build GNOME 3 pinentry]),
            pinentry_gnome_3=$enableval, pinentry_gnome_3=maybe)

dnl check for pkg-config
if test "$pinentry_gtk_2" != "no" -o "$pinentry_gnome_3" != "no"; then
        AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
	if test x"${PKG_CONFIG}" = xno ; then
		pinentry_gtk_2=no
		pinentry_gnome_3=no
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
			if test "$pinentry_gtk_2" != "no"
			then
			  pinentry_gtk_2=yes
			fi
		fi
	fi
fi
AM_CONDITIONAL(BUILD_PINENTRY_GTK_2, test "$pinentry_gtk_2" = "yes")

if test "$pinentry_gnome_3" != "no"; then
	AC_MSG_CHECKING([for gcr])
	"${PKG_CONFIG}" --exists gcr-base-3
	if test $? -ne 0 ; then
		AC_MSG_RESULT([no])
		AC_MSG_WARN([pkg-config could not find the module gcr-base-3])
		pinentry_gnome_3=no
	else
		AC_MSG_RESULT([yes])
		GNOME3CFLAGS=`"${PKG_CONFIG}" --cflags gcr-base-3`
		GNOME3LIBS=`"${PKG_CONFIG}" --libs gcr-base-3`
		AC_SUBST(GNOME3CFLAGS)
		AC_SUBST(GNOME3LIBS)
		AC_DEFINE(GCR_API_SUBJECT_TO_CHANGE, 1, [Nod nod])
		pinentry_gnome_3=yes
	fi
fi

AM_CONDITIONAL(BUILD_PINENTRY_GNOME_3, test "$pinentry_gnome_3" = "yes")

dnl
dnl Check for libsecret.
dnl
AC_ARG_ENABLE(libsecret,
            AC_HELP_STRING([--enable-libsecret],
	    [optionally cache passphrases using libsecret]),
            libsecret=$enableval, libsecret=maybe)

dnl check for pkg-config
if test "$libsecret" != "no" -o "$pinentry-qt" != "no"; then
        AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
  if test x"${PKG_CONFIG}" = xno ; then
    if test "$pinentry_qt" = "yes"; then
      AC_MSG_ERROR([[
                    ***
                    *** pkg-config is required to find the Qt libraries for pinentry-qt
                    ***]])
    else
      pinentry_qt=no
    fi
    libsecret=no
  fi
fi

dnl check if the module libsecret exists
if test "$libsecret" != "no"; then
	AC_MSG_CHECKING([for libsecret])
	"${PKG_CONFIG}" --exists 'libsecret-1'
	if test $? -ne 0 ; then
		AC_MSG_RESULT([no])
		AC_MSG_WARN([pkg-config could not find the modules libsecret-1])
		libsecret=no
	else
		AC_MSG_RESULT([yes])
		LIBSECRET_CFLAGS=`"${PKG_CONFIG}" --cflags 'libsecret-1'`
		LIBSECRET_LIBS=`"${PKG_CONFIG}" --libs 'libsecret-1'`
		libsecret=yes
	fi
fi
AM_CONDITIONAL(BUILD_WITH_LIBSECRET, test "$libsecret" = "yes")
if test "$libsecret" = "yes"; then
  AC_DEFINE(HAVE_LIBSECRET, 1,
    [The pinentries should optionally cache the passphrase using libsecret.])

  COMMON_CFLAGS="$LIBSECRET_CFLAGS $COMMON_CFLAGS"
  COMMON_LIBS="$LIBSECRET_LIBS $COMMON_LIBS"
fi

dnl
dnl Check for Qt pinentry program.
dnl
AC_ARG_ENABLE(pinentry-qt,
            AC_HELP_STRING([--enable-pinentry-qt], [build qt pinentry]),
            pinentry_qt=$enableval, pinentry_qt=maybe)


dnl
dnl Checks for qt libraries.  Deal correctly with $pinentry_qt = maybe.
dnl Tries to find Qt5, falls back on Qt4
dnl
if test "$pinentry_qt" != "no"; then
  FIND_QT
  if test "$have_qt4_libs" != "yes" -a "$have_qt5_libs" != "yes"; then
    if test "$pinentry_qt" = "yes"; then
        AC_MSG_ERROR([[
    ***
    *** Qt4 (QtCore, QtGui) or Qt5 (Qt5Core, Qt5Gui, Qt5Widgets) is required.
    ***]])
    else
        pinentry_qt=no
    fi
  fi
fi

AC_SUBST(PINENTRY_QT_CFLAGS)
AC_SUBST(PINENTRY_QT_LIBS)
AC_SUBST(MOC)

dnl If we have come so far, qt pinentry can be build.
if test "$pinentry_qt" != "no"; then
    pinentry_qt=yes
fi
AM_CONDITIONAL(BUILD_PINENTRY_QT, test "$pinentry_qt" = "yes")
if test "$pinentry_qt" = "yes"; then
  AC_DEFINE(PINENTRY_QT, 1, [The qt version of Pinentry is to be build])
  if test "$have_qt4_libs" = "yes"; then
    pinentry_qt_lib_version="(Qt4)"
  else
    pinentry_qt_lib_version="(Qt5)"
  fi
fi

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
  if test "$pinentry_qt" = "yes"; then
    PINENTRY_DEFAULT=pinentry-qt
  else
    if test "$pinentry_gnome_3" = "yes"; then
      PINENTRY_DEFAULT=pinentry-gnome3
    else
      if test "$pinentry_curses" = "yes"; then
        PINENTRY_DEFAULT=pinentry-curses
      else
        if test "$pinentry_tty" = "yes"; then
          PINENTRY_DEFAULT=pinentry-tty
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

#
# Print errors here so that they are visible all
# together and the user can acquire them all together.
#
die=no
if test "$have_gpg_error" = "no"; then
   die=yes
   AC_MSG_NOTICE([[
***
*** You need libgpg-error to build this program.
**  This library is for example available at
***   ftp://ftp.gnupg.org/gcrypt/libgpg-error
*** (at least version $NEED_GPG_ERROR_VERSION is required.)
***]])
fi

if test "$have_libassuan" = "no"; then
   die=yes
   AC_MSG_NOTICE([[
***
*** You need libassuan to build this program.
*** This library is for example available at
***   ftp://ftp.gnupg.org/gcrypt/libassuan/
*** (at least version $NEED_LIBASSUAN_VERSION (API $NEED_LIBASSUAN_API) is required).
***]])
fi

if test "$die" = "yes"; then
    AC_MSG_ERROR([[
***
*** Required libraries not found. Please consult the above messages
*** and install them before running configure again.
***]])
fi



#
# To avoid double inclusion of config.h which might happen at some
# places, we add the usual double inclusion protection at the top of
# config.h.
#
AH_TOP([
#ifndef GNUPG_CONFIG_H_INCLUDED
#define GNUPG_CONFIG_H_INCLUDED
])

#
# Stuff which goes at the bottom of config.h.
#
AH_BOTTOM([
#ifdef GPG_ERR_SOURCE_DEFAULT
# error GPG_ERR_SOURCE_DEFAULT already defined
#endif
#define GPG_ERR_SOURCE_DEFAULT  GPG_ERR_SOURCE_PINENTRY
#endif /*GNUPG_CONFIG_H_INCLUDED*/
])


AC_CONFIG_FILES([
m4/Makefile
secmem/Makefile
pinentry/Makefile
curses/Makefile
tty/Makefile
emacs/Makefile
gtk+-2/Makefile
gnome3/Makefile
qt/Makefile
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
	TTY Pinentry .....: $pinentry_tty
	Emacs Pinentry ...: $pinentry_emacs
	GTK+-2 Pinentry ..: $pinentry_gtk_2
	GNOME 3 Pinentry .: $pinentry_gnome_3
	Qt Pinentry ......: $pinentry_qt $pinentry_qt_lib_version
	W32 Pinentry .....: $pinentry_w32

	Fallback to Curses: $fallback_curses
	Emacs integration : $inside_emacs

	libsecret ........: $libsecret

	Default Pinentry .: $PINENTRY_DEFAULT
])
