dnl qt.m4
dnl Copyright (C) 2015 Intevation GmbH
dnl
dnl This file is part of PINENTRY.
dnl
dnl PINENTRY is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2 of the License, or
dnl (at your option) any later version.
dnl
dnl PINENTRY is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA

dnl Autoconf macro to find Qt5
dnl
dnl sets PINENTRY_QT_LIBS and PINENTRY_QT_CFLAGS
dnl
dnl if Qt5 was found have_qt5_libs is set to yes
dnl
dnl The moc lookup code is based on libpoppler (rev. d821207)

AC_DEFUN([FIND_QT],
[
  AC_ARG_ENABLE(pinentry-qt5,
                AS_HELP_STRING([--disable-pinentry-qt5],
                           [Don't use qt5 even if it is available.]),
                enable_pinentry_qt5=$enableval,
                enable_pinentry_qt5="try")

  have_qt5_libs="no";
  require_qt_cpp11="no";

  if test "$enable_pinentry_qt5" != "no"; then
    PKG_CHECK_MODULES(PINENTRY_QT,
                      Qt5Core >= 5.0.0 Qt5Gui >= 5.0.0 Qt5Widgets >= 5.0.0,
                      [have_qt5_libs="yes"],
                      [have_qt5_libs="no"])

    if "$PKG_CONFIG" --variable qt_config Qt5Core | grep -q "reduce_relocations"; then
      PINENTRY_QT_CFLAGS="$PINENTRY_QT_CFLAGS -fpic"
    fi
  fi
  if test "$have_qt5_libs" = "yes"; then
    PKG_CHECK_MODULES(PINENTRY_QT_REQUIRE_CPP11,
                      Qt5Core >= 5.7.0,
                      [require_qt_cpp11="yes"],
                      [require_qt_cpp11="no"])

    if test "${require_qt_cpp11}" = "yes"; then
      PINENTRY_QT_CFLAGS="$PINENTRY_QT_CFLAGS -std=c++11"
    fi

    qtlibdir=`"$PKG_CONFIG" --variable libdir Qt5Core`
    if test -n "$qtlibdir"; then
      if test "$enable_rpath" != "no"; then
        PINENTRY_QT_LDFLAGS="$PINENTRY_QT_LDFLAGS -Wl,-rpath \"$qtlibdir\""
      fi
    fi

    if test "$have_x11" = "yes"; then
      PKG_CHECK_MODULES(
        PINENTRY_QT_X11_EXTRAS,
        Qt5X11Extras >= 5.1.0,
        [have_qt5_x11extras="yes"],
        [
          AC_MSG_WARN([pinentry-qt will be built without Caps Lock warning on X11])
          have_qt5_x11extras="no"
        ])
      if test "$have_qt5_x11extras" = "yes"; then
        PINENTRY_QT_CFLAGS="$LIBX11_CFLAGS $PINENTRY_QT_CFLAGS $PINENTRY_QT_X11_EXTRAS_CFLAGS"
        PINENTRY_QT_LIBS="$LIBX11_LIBS $PINENTRY_QT_LIBS $PINENTRY_QT_X11_EXTRAS_LIBS"
      fi
    fi

    AC_CHECK_TOOL(MOC, moc)
    AC_MSG_CHECKING([moc version])
    mocversion=`$MOC -v 2>&1`
    mocversiongrep=`echo $mocversion | grep -E "Qt 5|moc 5"`
    if test x"$mocversiongrep" != x"$mocversion"; then
      AC_MSG_RESULT([no])
      # moc was not the qt5 one, try with moc-qt5
      AC_CHECK_TOOL(MOC2, moc-qt5)
      mocversion=`$MOC2 -v 2>&1`
      mocversiongrep=`echo $mocversion | grep -E "Qt 5|moc-qt5 5|moc 5"`
      if test x"$mocversiongrep" != x"$mocversion"; then
        AC_CHECK_TOOL(QTCHOOSER, qtchooser)
        qt5tooldir=`QT_SELECT=qt5 qtchooser -print-env | grep QTTOOLDIR | cut -d '=' -f 2 | cut -d \" -f 2`
        mocversion=`$qt5tooldir/moc -v 2>&1`
        mocversiongrep=`echo $mocversion | grep -E "Qt 5|moc 5"`
        if test x"$mocversiongrep" != x"$mocversion"; then
          # no valid moc found
          have_qt5_libs="no";
        else
          MOC=$qt5tooldir/moc
        fi
      else
        MOC=$MOC2
      fi
    fi

    AC_CHECK_TOOL(RCC, rcc)
    AC_MSG_CHECKING([rcc version])
    rccversion=`$RCC -v 2>&1`
    rccversiongrep=`echo $rccversion | grep -E "Qt 5|rcc 5"`
    if test x"$rccversiongrep" != x"$rccversion"; then
      AC_MSG_RESULT([no])
      # rcc was not the qt5 one, try with rcc-qt5
      AC_CHECK_TOOL(RCC2, rcc-qt5)
      rccversion=`$RCC2 -v 2>&1`
      rccversiongrep=`echo $rccversion | grep -E "Qt 5|rcc-qt5 5|rcc 5"`
      if test x"$rccversiongrep" != x"$rccversion"; then
        AC_CHECK_TOOL(QTCHOOSER, qtchooser)
        qt5tooldir=`QT_SELECT=qt5 qtchooser -print-env | grep QTTOOLDIR | cut -d '=' -f 2 | cut -d \" -f 2`
        rccversion=`$qt5tooldir/rcc -v 2>&1`
        rccversiongrep=`echo $rccversion | grep -E "Qt 5|rcc 5"`
        if test x"$rccversiongrep" != x"$rccversion"; then
          # no valid rcc found
          have_qt5_libs="no";
        else
          RCC=$qt5tooldir/rcc
        fi
      else
        RCC=$RCC2
      fi
    fi

  fi
])
