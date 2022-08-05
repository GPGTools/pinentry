dnl qt4.m4
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

dnl Autoconf macro to find either Qt4
dnl
dnl sets PINENTRY_QT4_LIBS and PINENTRY_QT4_CFLAGS
dnl
dnl if Qt4 was found have_qt4_libs is set to yes
dnl
dnl The moc lookup code is based on libpoppler (rev. d821207)

AC_DEFUN([FIND_QT4],
[
  have_qt4_libs="no";

  if test "$enable_pinentry_qt4" != "no"; then
    PKG_CHECK_MODULES(PINENTRY_QT4,
                      QtCore >= 4.6.0 QtGui >= 4.6.0,
                      [have_qt4_libs="yes"],
                      [have_qt4_libs="no"])
  fi
  if test "$have_qt4_libs" = "yes"; then
    AC_CHECK_TOOL(MOC4, moc)
    AC_MSG_CHECKING([moc version])
    mocversion=`$MOC4 -v 2>&1`
    mocversiongrep=`echo $mocversion | grep "Qt 4"`
    if test x"$mocversiongrep" != x"$mocversion"; then
      AC_MSG_RESULT([no])
      # moc was not the qt4 one, try with moc-qt4
      AC_CHECK_TOOL(MOC42, moc-qt4)
      mocversion=`$MOC42 -v 2>&1`
      mocversiongrep=`echo $mocversion | grep "Qt 4"`
      if test x"$mocversiongrep" != x"$mocversion"; then
        # no valid moc found
        have_qt4_libs="no";
        MOC4="not found"
      else
        MOC4=$MOC42
      fi
    fi
  fi
])
