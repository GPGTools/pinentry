dnl Autoconf macros used by PINENTRY
dnl
dnl Copyright (C) 2002, 2022 g10 Code GmbH
dnl
dnl
######################################################################
# Check whether mlock is broken (hpux 10.20 raises a SIGBUS if mlock
# is not called from uid 0 (not tested whether uid 0 works)
# For DECs Tru64 we have also to check whether mlock is in librt
# mlock is there a macro using memlk()
######################################################################
dnl GNUPG_CHECK_MLOCK
dnl
define(GNUPG_CHECK_MLOCK,
  [ AC_CHECK_FUNCS(mlock)
    if test "$ac_cv_func_mlock" = "no"; then
        AC_CHECK_HEADERS(sys/mman.h)
        if test "$ac_cv_header_sys_mman_h" = "yes"; then
            # Add librt to LIBS:
            AC_CHECK_LIB(rt, memlk)
            AC_CACHE_CHECK([whether mlock is in sys/mman.h],
                            gnupg_cv_mlock_is_in_sys_mman,
                [AC_LINK_IFELSE(
                   [AC_LANG_PROGRAM([[
                    #include <assert.h>
                    #ifdef HAVE_SYS_MMAN_H
                    #include <sys/mman.h>
                    #endif
                    ]], [[
int i;

/* glibc defines this for functions which it implements
 * to always fail with ENOSYS.  Some functions are actually
 * named something starting with __ and the normal name
 * is an alias.  */
#if defined (__stub_mlock) || defined (__stub___mlock)
choke me
#else
mlock(&i, 4);
#endif
; return 0;
                    ]])],
                gnupg_cv_mlock_is_in_sys_mman=yes,
                gnupg_cv_mlock_is_in_sys_mman=no)])
            if test "$gnupg_cv_mlock_is_in_sys_mman" = "yes"; then
                AC_DEFINE(HAVE_MLOCK,1,
                          [Defined if the system supports an mlock() call])
            fi
        fi
    fi
    if test "$ac_cv_func_mlock" = "yes"; then
        AC_CHECK_FUNCS(sysconf getpagesize)
        AC_MSG_CHECKING(whether mlock is broken)
          AC_CACHE_VAL(gnupg_cv_have_broken_mlock,
             AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>

int main()
{
    char *pool;
    int err;
    long int pgsize;

#if defined(HAVE_SYSCONF) && defined(_SC_PAGESIZE)
    pgsize = sysconf (_SC_PAGESIZE);
#elif defined (HAVE_GETPAGESIZE)
    pgsize = getpagesize();
#else
    pgsize = -1;
#endif

    if (pgsize == -1)
      pgsize = 4096;

    pool = malloc( 4096 + pgsize );
    if( !pool )
        return 2;
    pool += (pgsize - ((size_t)pool % pgsize));

    err = mlock( pool, 4096 );
    if( !err || errno == EPERM || errno == EAGAIN)
        return 0; /* okay */

    return 1;  /* hmmm */
}
            ]])],
            gnupg_cv_have_broken_mlock="no",
            gnupg_cv_have_broken_mlock="yes",
            gnupg_cv_have_broken_mlock="assume-no"
           )
         )
         if test "$gnupg_cv_have_broken_mlock" = "yes"; then
             AC_DEFINE(HAVE_BROKEN_MLOCK,1,
                       [Defined if the mlock() call does not work])
             AC_MSG_RESULT(yes)
         else
            if test "$gnupg_cv_have_broken_mlock" = "no"; then
                AC_MSG_RESULT(no)
            else
                AC_MSG_RESULT(assuming no)
            fi
         fi
    fi
  ])
