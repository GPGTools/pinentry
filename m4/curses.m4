dnl IU_LIB_NCURSES, IU_LIB_CURSES and IU_LIB_TERMCAP are:
dnl Copyright (C) 1996, 1997, 1998 Free Software Foundation, Inc.
dnl Written by Miles Bader <miles@gnu.ai.mit.edu>
dnl
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2, or (at your option)
dnl any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
dnl

dnl IU_LIB_NCURSES -- check for, and configure, ncurses
dnl
dnl If libncurses is found to exist on this system and the --disable-ncurses
dnl flag wasn't specified, defines LIBNCURSES with the appropriate linker
dnl specification, and possibly defines NCURSES_INCLUDE with the appropriate
dnl -I flag to get access to ncurses include files.
dnl
AC_DEFUN([IU_LIB_NCURSES], [
  AC_ARG_ENABLE(ncurses,    [  --disable-ncurses       don't prefer -lncurses over -lcurses],
              , enable_ncurses=yes)
  if test "$enable_ncurses" = yes; then
    AC_CHECK_LIB(ncursesw, initscr, LIBNCURSES="-lncursesw",
      AC_CHECK_LIB(ncurses, initscr, LIBNCURSES="-lncurses"))
    if test "$ac_cv_lib_ncursesw_initscr" = yes; then
      have_ncursesw=yes
    else
      have_ncursesw=no
    fi
    if test "$LIBNCURSES"; then
      # Use ncurses header files instead of the ordinary ones, if possible;
      # is there a better way of doing this, that avoids looking in specific
      # directories?
      AC_ARG_WITH(ncurses-include-dir,
[  --with-ncurses-include-dir=DIR
                          Set directory containing the include files for
                          use with -lncurses, when it isn't installed as
                          the default curses library.  If DIR is "none",
                          then no special ncurses include files are used.
  --without-ncurses-include-dir
                          Equivalent to --with-ncurses-include-dir=none])dnl
      if test "${with_ncurses_include_dir+set}" = set; then
        AC_MSG_CHECKING(for ncurses include dir)
	case "$with_ncurses_include_dir" in
	  no|none)
	    inetutils_cv_includedir_ncurses=none;;
	  *)
	    inetutils_cv_includedir_ncurses="$with_ncurses_include_dir";;
	esac
        AC_MSG_RESULT($inetutils_cv_includedir_ncurses)
      else
	AC_CACHE_CHECK(for ncurses include dir,
		       inetutils_cv_includedir_ncurses,
          if test "$have_ncursesw" = yes; then
            ncursesdir=ncursesw
          else
            ncursesdir=ncurses
          fi
	  for D in $includedir $prefix/include /local/include /usr/local/include /include /usr/include; do
	    if test -d $D/$ncursesdir; then
	      inetutils_cv_includedir_ncurses="$D/$ncursesdir"
	      break
	    fi
	    test "$inetutils_cv_includedir_ncurses" \
	      || inetutils_cv_includedir_ncurses=none
	  done)
      fi
      if test "$inetutils_cv_includedir_ncurses" = none; then
        NCURSES_INCLUDE=""
      else
        NCURSES_INCLUDE="-I$inetutils_cv_includedir_ncurses"
      fi
    fi
    if test $have_ncursesw = yes; then
      AC_DEFINE(HAVE_NCURSESW, 1, [Define if you have working ncursesw])
    fi
  fi
  AC_SUBST(NCURSES_INCLUDE)
  AC_SUBST(LIBNCURSES)])dnl

dnl IU_LIB_TERMCAP -- check for various termcap libraries
dnl
dnl Checks for various common libraries implementing the termcap interface,
dnl including ncurses (unless --disable ncurses is specified), curses (which
dnl does on some systems), termcap, and termlib.  If termcap is found, then
dnl LIBTERMCAP is defined with the appropriate linker specification.
dnl 
AC_DEFUN([IU_LIB_TERMCAP], [
  AC_REQUIRE([IU_LIB_NCURSES])
  if test "$LIBNCURSES"; then
    LIBTERMCAP="$LIBNCURSES"
  else
    AC_CHECK_LIB(curses, tgetent, LIBTERMCAP=-lcurses)
    if test "$ac_cv_lib_curses_tgetent" = no; then
      AC_CHECK_LIB(termcap, tgetent, LIBTERMCAP=-ltermcap)
    fi
    if test "$ac_cv_lib_termcap_tgetent" = no; then
      AC_CHECK_LIB(termlib, tgetent, LIBTERMCAP=-ltermlib)
    fi
  fi
  AC_SUBST(LIBTERMCAP)])dnl

dnl IU_LIB_CURSES -- checke for curses, and associated libraries
dnl
dnl Checks for varions libraries implementing the curses interface, and if
dnl found, defines LIBCURSES to be the appropriate linker specification,
dnl *including* any termcap libraries if needed (some versions of curses
dnl don't need termcap).
dnl
AC_DEFUN([IU_LIB_CURSES], [
  AC_REQUIRE([IU_LIB_TERMCAP])
  AC_REQUIRE([IU_LIB_NCURSES])
  if test "$LIBNCURSES"; then
    LIBCURSES="$LIBNCURSES"	# ncurses doesn't require termcap
  else
    _IU_SAVE_LIBS="$LIBS"
    LIBS="$LIBTERMCAP"
    AC_CHECK_LIB(curses, initscr, LIBCURSES="-lcurses")
    if test "$LIBCURSES" -a "$LIBTERMCAP" -a "$LIBCURSES" != "$LIBTERMCAP"; then
      AC_CACHE_CHECK(whether curses needs $LIBTERMCAP,
		     inetutils_cv_curses_needs_termcap,
	LIBS="$LIBCURSES"
	AC_TRY_LINK([#include <curses.h>], [initscr ();],
		    [inetutils_cv_curses_needs_termcap=no],
		    [inetutils_cv_curses_needs_termcap=yes]))
      if test $inetutils_cv_curses_needs_termcap = yes; then
	  LIBCURSES="$LIBCURSES $LIBTERMCAP"
      fi
    fi
    LIBS="$_IU_SAVE_LIBS"
  fi
  AC_SUBST(LIBCURSES)])dnl
