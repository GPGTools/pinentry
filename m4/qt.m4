##   -*- autoconf -*-


dnl    This file is part of the KDE libraries/packages
dnl    Copyright (C) 1997 Janos Farkas (chexum@shadow.banki.hu)
dnl              (C) 1997,98,99 Stephan Kulow (coolo@kde.org)
dnl              (C) 2002 g10 Code GmbH
dnl              Modified for PINENTRY by Marcus Brinkmann.
dnl
dnl    This file is free software; you can redistribute it and/or
dnl    modify it under the terms of the GNU Library General Public
dnl    License as published by the Free Software Foundation; either
dnl    version 2 of the License, or (at your option) any later version.

dnl    This library is distributed in the hope that it will be useful,
dnl    but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
dnl    Library General Public License for more details.

dnl    You should have received a copy of the GNU Library General Public License
dnl    along with this library; see the file COPYING.LIB.  If not, write to
dnl    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
dnl    Boston, MA 02111-1307, USA.

dnl ------------------------------------------------------------------------
dnl Find a file (or one of more files in a list of dirs)
dnl ------------------------------------------------------------------------
dnl
AC_DEFUN([QT_FIND_FILE],
[
$3=NO
for i in $2;
do
  for j in $1;
  do
    echo "configure: __oline__: $i/$j" >&AS_MESSAGE_LOG_FD
    if test -r "$i/$j"; then
      echo "taking that" >&AS_MESSAGE_LOG_FD
      $3=$i
      break 2
    fi
  done
done
])

dnl ------------------------------------------------------------------------
dnl Find the meta object compiler in the PATH,
dnl in $QTDIR/bin, and some more usual places
dnl ------------------------------------------------------------------------
dnl
AC_DEFUN([QT_PATH_MOC],
[
   qt_bindirs=""
   for dir in $qt_dirs; do
      qt_bindirs="$qt_bindirs:$dir/bin:$dir/src/moc"
   done
   qt_bindirs="$qt_bindirs:/usr/bin:/usr/X11R6/bin:/usr/local/qt/bin"
   if test ! "$ac_qt_bindir" = "NO"; then
      qt_bindirs="$ac_qt_bindir:$qt_bindirs"
   fi

   AC_PATH_PROG(MOC, moc, no, [$qt_bindirs])
   if test "$MOC" = no; then
    #AC_MSG_ERROR([No Qt meta object compiler (moc) found!
    #Please check whether you installed Qt correctly.
    #You need to have a running moc binary.
    #configure tried to run $ac_cv_path_moc and the test didn't
    #succeed. If configure shouldn't have tried this one, set
    #the environment variable MOC to the right one before running
    #configure.
    #])
    have_moc="no"
   else
    have_moc="yes"
    AC_SUBST(MOC)
   fi
])


dnl ------------------------------------------------------------------------
dnl Find the header files and libraries for the X Window System.
dnl Extended the macro AC_PATH_XTRA.
dnl ------------------------------------------------------------------------
dnl
AC_DEFUN([QT_PATH_X],
[
AC_ARG_ENABLE(
  embedded,
  [  --enable-embedded       link to Qt-embedded, don't use X],
  qt_use_emb=$enableval,
  qt_use_emb=no
)

AC_ARG_ENABLE(
  palmtop,
  [  --enable-palmtop       link to Qt-embedded, don't use X, link to the Qt Palmtop Environment],
  qt_use_emb_palm=$enableval,
  qt_use_emb_palm=no
)

if test "$qt_use_emb" = "no"; then
  AC_PATH_X
  AC_PATH_XTRA
  if test "$no_x" = yes; then
    AC_MSG_ERROR([Can't find X. Please check your installation and add the correct paths!])
  fi
  QT_CXXFLAGS="$X_CFLAGS"
  QT_LDFLAGS="$X_LIBS"
  QT_LIBS="$X_PRE_LIBS -lXext -lX11 $X_EXTRA_LIBS"
  QTE_NORTTI=""
else
  dnl We're using QT Embedded
  QT_CXXFLAGS="-fno-rtti -DQWS"
  QT_LDFLAGS="-DQWS"
  QT_LIBS=""
  QTE_NORTTI="-fno-rtti -DQWS"
fi
AC_SUBST(QT_CXXFLAGS)
AC_SUBST(QT_LDFLAGS)
AC_SUBST(QT_LIBS)
AC_SUBST(QTE_NORTTI)
])

AC_DEFUN([QT_PRINT_PROGRAM],
[
AC_REQUIRE([QT_CHECK_VERSION])
cat > conftest.$ac_ext <<EOF
#include "confdefs.h"
#include <qglobal.h>
#include <qapplication.h>
EOF
if test "$qt_ver" = "2"; then
cat >> conftest.$ac_ext <<EOF
#include <qevent.h>
#include <qstring.h>
#include <qstyle.h>
EOF

if test $qt_subver -gt 0; then
cat >> conftest.$ac_ext <<EOF
#include <qiconview.h>
EOF
fi
fi

if test "$qt_ver" = "3"; then
cat >> conftest.$ac_ext <<EOF
#include <qcursor.h>
#include <qstylefactory.h>
#include <private/qucomextra_p.h>
EOF
fi

echo "#if ! ($qt_verstring)" >> conftest.$ac_ext
cat >> conftest.$ac_ext <<EOF
#error 1
#endif

int main() {
EOF
if test "$qt_ver" = "2"; then
cat >> conftest.$ac_ext <<EOF
    QStringList *t = new QStringList();
    Q_UNUSED(t);
EOF
if test $qt_subver -gt 0; then
cat >> conftest.$ac_ext <<EOF
    QIconView iv(0);
    iv.setWordWrapIconText(false);
    QString s;
    s.setLatin1("Elvis is alive", 14);
EOF
fi
fi
if test "$qt_ver" = "3"; then
cat >> conftest.$ac_ext <<EOF
    (void)QStyleFactory::create(QString::null);
    QCursor c(Qt::WhatsThisCursor);
EOF
fi
cat >> conftest.$ac_ext <<EOF
    return 0;
}
EOF
])


AC_DEFUN([QT_CHECK_VERSION],
[
if test -z "$1"; then
  qt_ver=3
  qt_subver=1
else
  qt_subver=`echo "$1" | sed -e 's#[0-9][0-9]*\.\([0-9][0-9]*\).*#\1#'`
  # following is the check if subversion isn´t found in passed argument
  if test "$qt_subver" = "$1"; then
    qt_subver=1
  fi
  qt_ver=`echo "$1" | sed -e 's#^\([0-9][0-9]*\)\..*#\1#'`
  if test "$qt_ver" = "1"; then
    qt_subver=42
  fi
fi

if test -z "$2"; then
  if test "$qt_ver" = "2"; then
    if test $qt_subver -gt 0; then
      qt_minversion=">= Qt 2.2.2"
    else
      qt_minversion=">= Qt 2.0.2"
    fi
  fi
  if test "$qt_ver" = "3"; then
    qt_minversion=">= Qt 3.0.1"
  fi
  if test "$qt_ver" = "1"; then
    qt_minversion=">= 1.42 and < 2.0"
  fi
else
   qt_minversion=$2
fi

if test -z "$3"; then
   if test $qt_ver = 3; then
     qt_verstring="QT_VERSION >= 301"
   fi
   if test $qt_ver = 2; then
     if test $qt_subver -gt 0; then
       qt_verstring="QT_VERSION >= 222"
     else
       qt_verstring="QT_VERSION >= 200"
     fi
   fi
   if test $qt_ver = 1; then
    qt_verstring="QT_VERSION >= 142 && QT_VERSION < 200"
   fi
else
   qt_verstring=$3
fi

if test $qt_ver = 3; then
  qt_dirs="$QTDIR /usr/lib/qt3 /usr/lib/qt"
fi
if test $qt_ver = 2; then
  qt_dirs="$QTDIR /usr/lib/qt2 /usr/lib/qt"
fi
if test $qt_ver = 1; then
  qt_dirs="$QTDIR /usr/lib/qt"
fi
])


dnl ------------------------------------------------------------------------
dnl Try to find the Qt headers and libraries.
dnl $(QT_LDFLAGS) will be -Lqtliblocation (if needed)
dnl and $(QT_INCLUDES) will be -Iqthdrlocation (if needed)
dnl ------------------------------------------------------------------------
dnl
AC_DEFUN([QT_PATH_1_3],
[
AC_REQUIRE([QT_PATH_X])
AC_REQUIRE([QT_CHECK_VERSION])

dnl ------------------------------------------------------------------------
dnl Add configure flag to enable linking to MT version of Qt library.
dnl ------------------------------------------------------------------------

AC_ARG_ENABLE(
  mt,
  [  --disable-mt            link to non-threaded Qt (deprecated)],
  qt_use_mt=$enableval,
  [
    if test $qt_ver = 3; then
      qt_use_mt=yes
    else
      qt_use_mt=no
    fi
  ]
)

USING_QT_MT=""

dnl ------------------------------------------------------------------------
dnl If we not get --disable-qt-mt then adjust some vars for the host.
dnl ------------------------------------------------------------------------

QT_MT_LDFLAGS=
QT_MT_LIBS=
if test "x$qt_use_mt" = "xyes"; then
  QT_CHECK_THREADING
  if test "x$qt_use_threading" = "xyes"; then
    QT_CXXFLAGS="$USE_THREADS -DQT_THREAD_SUPPORT $QT_CXXFLAGS"
    QT_MT_LDFLAGS="$USE_THREADS"
    QT_MT_LIBS="$LIBPTHREAD"
  else
    qt_use_mt=no
  fi
fi
AC_SUBST(QT_MT_LDFLAGS)
AC_SUBST(QT_MT_LIBS)

dnl ------------------------------------------------------------------------
dnl If we haven't been told how to link to Qt, we work it out for ourselves.
dnl ------------------------------------------------------------------------
if test -z "$LIBQT_GLOB"; then
  if test "x$qt_use_emb" = "xyes"; then
    LIBQT_GLOB="libqte.*"
  else
    LIBQT_GLOB="libqt.*"
  fi
fi

if test -z "$LIBQT"; then
dnl ------------------------------------------------------------
dnl If we got --enable-embedded then adjust the Qt library name.
dnl ------------------------------------------------------------
  if test "x$qt_use_emb" = "xyes"; then
    qtlib="qte"
  else
    qtlib="qt"
  fi
fi

if test -z "$LIBQPE"; then
dnl ------------------------------------------------------------
dnl If we got --enable-palmtop then add -lqpe to the link line
dnl ------------------------------------------------------------
  if test "x$qt_use_emb" = "xyes"; then
    if test "x$qt_use_emb_palm" = "xyes"; then
      LIB_QPE="-lqpe"
    else
      LIB_QPE=""
    fi
  else
    LIB_QPE=""
  fi
fi

dnl ------------------------------------------------------------------------
dnl If we got --enable-qt-mt then adjust the Qt library name for the host.
dnl ------------------------------------------------------------------------

if test "x$qt_use_mt" = "xyes"; then
  if test -z "$LIBQT"; then
    LIBQT="-l$qtlib-mt"
  else
    LIBQT="$qtlib-mt"
  fi
  LIBQT_GLOB="lib$qtlib-mt.*"
  USING_QT_MT="using -mt"
else
  LIBQT="-l$qtlib"
fi

AC_MSG_CHECKING([for Qt])

QT_LIBS="$LIBQT $QT_LIBS"
test -z "$QT_MT_LIBS" || QT_LIBS="$QT_LIBS $QT_MT_LIBS"

ac_qt_includes=NO ac_qt_libraries=NO ac_qt_bindir=NO
qt_libraries=""
qt_includes=""
AC_ARG_WITH(qt-dir,
    [  --with-qt-dir=DIR       where the root of Qt is installed ],
    [  ac_qt_includes="$withval"/include
       ac_qt_libraries="$withval"/lib
       ac_qt_bindir="$withval"/bin
    ])

AC_ARG_WITH(qt-includes,
    [  --with-qt-includes=DIR  where the Qt includes are. ],
    [
       ac_qt_includes="$withval"
    ])

kde_qt_libs_given=no

AC_ARG_WITH(qt-libraries,
    [  --with-qt-libraries=DIR where the Qt library is installed.],
    [  ac_qt_libraries="$withval"
       kde_qt_libs_given=yes
    ])

AC_CACHE_VAL(ac_cv_have_qt,
[#try to guess Qt locations

qt_incdirs=""
for dir in $qt_dirs; do
   qt_incdirs="$qt_incdirs $dir/include $dir"
done
qt_incdirs="$QTINC $qt_incdirs /usr/local/qt/include  /usr/include /usr/X11R6/include/X11/qt /usr/X11R6/include/qt /usr/X11R6/include/qt2 /usr/include/qt3 $x_includes"
if test ! "$ac_qt_includes" = "NO"; then
   qt_incdirs="$ac_qt_includes $qt_incdirs"
fi

if test "$qt_ver" != "1"; then
  kde_qt_header=qstyle.h
else
  kde_qt_header=qglobal.h
fi

QT_FIND_FILE($kde_qt_header, $qt_incdirs, qt_incdir)
ac_qt_includes="$qt_incdir"

qt_libdirs=""
for dir in $qt_dirs; do
   qt_libdirs="$qt_libdirs $dir/lib $dir"
done
qt_libdirs="$QTLIB $qt_libdirs /usr/X11R6/lib /usr/lib /usr/local/qt/lib $x_libraries"
if test ! "$ac_qt_libraries" = "NO"; then
  qt_libdir=$ac_qt_libraries
else
  qt_libdirs="$ac_qt_libraries $qt_libdirs"
  # if the Qt was given, the chance is too big that libqt.* doesn't exist
  qt_libdir=NONE
  for dir in $qt_libdirs; do
    try="ls -1 $dir/${LIBQT_GLOB}"
    if test -n "`$try 2> /dev/null`"; then qt_libdir=$dir; break; else echo "tried $dir" >&AS_MESSAGE_LOG_FD; fi
  done
fi

ac_qt_libraries="$qt_libdir"

AC_LANG_PUSH(C++)

ac_cxxflags_safe="$CXXFLAGS"
ac_ldflags_safe="$LDFLAGS"
ac_libs_safe="$LIBS"

CXXFLAGS="$CXXFLAGS -I$qt_incdir $QT_CXXFLAGS"
LDFLAGS="$LDFLAGS -L$qt_libdir $QT_LDFLAGS"
LIBS="$LIBS $QT_LIBS"

QT_PRINT_PROGRAM

if AC_TRY_EVAL(ac_link) && test -s conftest; then
  rm -f conftest*
else
  echo "configure: failed program was:" >&AS_MESSAGE_LOG_FD
  cat conftest.$ac_ext >&AS_MESSAGE_LOG_FD
  ac_qt_libraries="NO"
fi
rm -f conftest*
CXXFLAGS="$ac_cxxflags_safe"
LDFLAGS="$ac_ldflags_safe"
LIBS="$ac_libs_safe"

AC_LANG_POP(C++)
if test "$ac_qt_includes" = NO || test "$ac_qt_libraries" = NO; then
  ac_cv_have_qt="have_qt=no"
  ac_qt_notfound=""
  missing_qt_mt=""
  if test "$ac_qt_includes" = NO; then
    if test "$ac_qt_libraries" = NO; then
      ac_qt_notfound="(headers and libraries)";
    else
      ac_qt_notfound="(headers)";
    fi
  else
    if test "x$qt_use_mt" = "xyes"; then
       missing_qt_mt="
Make sure that you have compiled Qt with thread support!"
       ac_qt_notfound="(library $qtlib-mt)";
    else
       ac_qt_notfound="(library $qtlib)";
    fi
  fi

  #AC_MSG_ERROR([Qt ($qt_minversion) $ac_qt_notfound not found. Please check your installation!
  #For more details about this problem, look at the end of config.log.$missing_qt_mt])
  have_qt="no"
else
  have_qt="yes"
fi
])

eval "$ac_cv_have_qt"

if test "$have_qt" != yes; then
  AC_MSG_RESULT([$have_qt]);
else
  ac_cv_have_qt="have_qt=yes \
    ac_qt_includes=$ac_qt_includes ac_qt_libraries=$ac_qt_libraries"
  AC_MSG_RESULT([libraries $ac_qt_libraries, headers $ac_qt_includes $USING_QT_MT])

  qt_libraries="$ac_qt_libraries"
  qt_includes="$ac_qt_includes"
fi

AC_SUBST(qt_libraries)
AC_SUBST(qt_includes)

if test "$qt_includes" = "$x_includes" || test -z "$qt_includes"; then
 QT_INCLUDES=""
else
 QT_INCLUDES="-I$qt_includes"
fi

if test "$qt_libraries" != "$x_libraries" && test -n "$qt_libraries"; then
 QT_LDFLAGS="$QT_LDFLAGS -L$qt_libraries"
fi
test -z "$QT_MT_LDFLAGS" || QT_LDFLAGS="$QT_LDFLAGS $QT_MT_LDFLAGS"

AC_SUBST(QT_INCLUDES)
QT_PATH_MOC


AC_SUBST(LIB_QPE)
])

AC_DEFUN([QT_PATH],
[
QT_PATH_1_3
QT_CHECK_RPATH
])


AC_DEFUN([QT_CHECK_COMPILER_FLAG],
[
AC_MSG_CHECKING(whether $CXX supports -$1)
kde_cache=`echo $1 | sed 'y% .=/+-%____p_%'`
AC_CACHE_VAL(kde_cv_prog_cxx_$kde_cache,
[
  AC_LANG_PUSH(C++)
  save_CXXFLAGS="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS -$1"
  AC_TRY_LINK([],[ return 0; ], [eval "kde_cv_prog_cxx_$kde_cache=yes"], [])
  CXXFLAGS="$save_CXXFLAGS"
  AC_LANG_POP(C++)
])
if eval "test \"`echo '$kde_cv_prog_cxx_'$kde_cache`\" = yes"; then
 AC_MSG_RESULT(yes)
 :
 $2
else
 AC_MSG_RESULT(no)
 :
 $3
fi
])

dnl QT_REMOVE_FORBIDDEN removes forbidden arguments from variables
dnl use: QT_REMOVE_FORBIDDEN(CC, [-forbid -bad-option whatever])
dnl it's all white-space separated
AC_DEFUN([QT_REMOVE_FORBIDDEN],
[ __val=$$1
  __forbid=" $2 "
  if test -n "$__val"; then
    __new=""
    ac_save_IFS=$IFS
    IFS=" 	"
    for i in $__val; do
      case "$__forbid" in
        *" $i "*) AC_MSG_WARN([found forbidden $i in $1, removing it]) ;;
	*) # Careful to not add spaces, where there were none, because otherwise
	   # libtool gets confused, if we change e.g. CXX
	   if test -z "$__new" ; then __new=$i ; else __new="$__new $i" ; fi ;;
      esac
    done
    IFS=$ac_save_IFS
    $1=$__new
  fi
])

dnl QT_VALIDIFY_CXXFLAGS checks for forbidden flags the user may have given
AC_DEFUN([QT_VALIDIFY_CXXFLAGS],
[dnl
if test "x$qt_use_emb" != "xyes"; then
 QT_REMOVE_FORBIDDEN(CXX, [-fno-rtti -rpath])
 QT_REMOVE_FORBIDDEN(CXXFLAGS, [-fno-rtti -rpath])
else
 QT_REMOVE_FORBIDDEN(CXX, [-rpath])
 QT_REMOVE_FORBIDDEN(CXXFLAGS, [-rpath])
fi
])

AC_DEFUN([QT_CHECK_COMPILERS],
[
  AC_PROG_CXX

  QT_CHECK_COMPILER_FLAG(fexceptions,[QT_CXXFLAGS="$QT_CXXFLAGS -fexceptions"])

  case "$host" in
      *-*-irix*)  test "$GXX" = yes && QT_CXXFLAGS="-D_LANGUAGE_C_PLUS_PLUS -D__LANGUAGE_C_PLUS_PLUS $QT_CXXFLAGS" ;;
      *-*-sysv4.2uw*) QT_CXXFLAGS="-D_UNIXWARE $QT_CXXFLAGS";;
      *-*-sysv5uw7*) QT_CXXFLAGS="-D_UNIXWARE7 $QT_CXXFLAGS";;
      *-*-solaris*) 
        if test "$GXX" = yes; then
          libstdcpp=`$CXX -print-file-name=libstdc++.so`
          if test ! -f $libstdcpp; then
             AC_MSG_ERROR([You've compiled gcc without --enable-shared. This doesn't work with the Qt pinentry. Please recompile gcc with --enable-shared to receive a libstdc++.so])
          fi
        fi
        ;;
  esac

  QT_VALIDIFY_CXXFLAGS

  AC_PROG_CXXCPP
])

AC_DEFUN([QT_CHECK_RPATH],
[
AC_MSG_CHECKING(for rpath)
AC_ARG_ENABLE(rpath,
      [  --disable-rpath         do not use the rpath feature of ld],
      USE_RPATH=$enableval, USE_RPATH=yes)

if test -z "$QT_RPATH" && test "$USE_RPATH" = "yes"; then

  QT_RPATH=""
  if test -n "$qt_libraries"; then
    QT_RPATH="$QT_RPATH -Wl,--rpath -Wl,\$(qt_libraries)"
  fi
  dnl $x_libraries is set to /usr/lib in case
  if test -n "$X_LIBS"; then
    QT_RPATH="$QT_RPATH -Wl,--rpath -Wl,\$(x_libraries)"
  fi
fi
AC_SUBST(x_libraries)
AC_SUBST(QT_RPATH)
AC_MSG_RESULT($USE_RPATH)
])


AC_DEFUN([QT_CHECK_LIBPTHREAD],
[
AC_CHECK_LIB(pthread, pthread_create, [LIBPTHREAD="-lpthread"] )
AC_SUBST(LIBPTHREAD)
])

AC_DEFUN([QT_CHECK_PTHREAD_OPTION],
[
    AC_ARG_ENABLE(kernel-threads, [  --enable-kernel-threads Enable the use of the LinuxThreads port on FreeBSD/i386 only.],
	kde_use_kernthreads=$enableval, kde_use_kernthreads=no)

    if test "$kde_use_kernthreads" = "yes"; then
      ac_save_CXXFLAGS="$CXXFLAGS"
      ac_save_CFLAGS="$CXXFLAGS"
      CXXFLAGS="-I/usr/local/include/pthread/linuxthreads $CXXFLAGS"
      CFLAGS="-I/usr/local/include/pthread/linuxthreads $CFLAGS"
      AC_CHECK_HEADERS(pthread/linuxthreads/pthread.h)
      CXXFLAGS="$ac_save_CXXFLAGS"
      CFLAGS="$ac_save_CFLAGS"
      if test "$ac_cv_header_pthread_linuxthreads_pthread_h" = "no"; then
        kde_use_kernthreads=no
      else
        dnl Add proper -I and -l statements
        AC_CHECK_LIB(lthread, pthread_join, [LIBPTHREAD="-llthread -llgcc_r"]) dnl for FreeBSD
        if test "x$LIBPTHREAD" = "x"; then
          kde_use_kernthreads=no
        else
          USE_THREADS="-D_THREAD_SAFE -I/usr/local/include/pthread/linuxthreads"
        fi
      fi
    else 
      USE_THREADS=""
      if test -z "$LIBPTHREAD"; then
        QT_CHECK_COMPILER_FLAG(pthread, [USE_THREADS="-pthread"] )
      fi
    fi

    case $host_os in
 	solaris*)
		QT_CHECK_COMPILER_FLAG(mt, [USE_THREADS="-mt"])
                QT_CXXFLAGS="$QT_CXXFLAGS -D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS -DUSE_SOLARIS -DSVR4"
    		;;
        freebsd*)
                QT_CXXFLAGS="$QT_CXXFLAGS -D_THREAD_SAFE"
                ;;
        aix*)
                QT_CXXFLAGS="$QT_CXXFLAGS -D_THREAD_SAFE"
                LIBPTHREAD="$LIBPTHREAD -lc_r"
                ;;
        linux*) QT_CXXFLAGS="$QT_CXXFLAGS -D_REENTRANT"
                if test "$CXX" = "KCC"; then
                  QT_CXXFLAGS="$QT_CXXFLAGS --thread_safe"
                fi
                ;;
	*)
		;;
    esac
    AC_SUBST(USE_THREADS)
    AC_SUBST(LIBPTHREAD)
])

AC_DEFUN([QT_CHECK_THREADING],
[
  AC_REQUIRE([QT_CHECK_LIBPTHREAD])
  AC_REQUIRE([QT_CHECK_PTHREAD_OPTION])
  dnl default is yes if libpthread is found and no if no libpthread is available
  if test -z "$LIBPTHREAD"; then
    if test -z "$USE_THREADS"; then
      kde_check_threading_default=no
    else
      kde_check_threading_default=yes
    fi
  else
    kde_check_threading_default=yes
  fi
  AC_ARG_ENABLE(threading, [  --disable-threading     disables threading even if libpthread found ],
   qt_use_threading=$enableval, qt_use_threading=$kde_check_threading_default)
  if test "x$qt_use_threading" = "xyes"; then
    AC_DEFINE(HAVE_LIBPTHREAD, 1, [Define if you have a working libpthread (will enable threaded code)])
  fi
])
