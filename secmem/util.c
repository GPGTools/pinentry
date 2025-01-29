/* Quintuple Agent
 * Copyright (C) 1999 Robert Bihlmeyer <robbe@orcus.priv.at>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _GNU_SOURCE 1

#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "util.h"

#ifndef HAVE_DOSISH_SYSTEM
static int uid_set = 0;
static uid_t real_uid, file_uid;
#endif /*!HAVE_DOSISH_SYSTEM*/

/* Write DATA of size BYTES to FD, until all is written or an error
   occurs.  */
ssize_t
xwrite(int fd, const void *data, size_t bytes)
{
  char *ptr;
  size_t todo;
  ssize_t written = 0;

  for (ptr = (char *)data, todo = bytes; todo; ptr += written, todo -= written)
    {
      do
        written = write (fd, ptr, todo);
      while (
             written == -1 && errno == EINTR
             );
      if (written < 0)
        break;
    }
  return written;
}

#if 0
extern int debug;

int
debugmsg(const char *fmt, ...)
{
  va_list va;
  int ret;

  if (debug) {
    va_start(va, fmt);
    fprintf(stderr, "\e[4m");
    ret = vfprintf(stderr, fmt, va);
    fprintf(stderr, "\e[24m");
    va_end(va);
    return ret;
  } else
    return 0;
}
#endif

/* initialize uid variables */
#ifndef HAVE_DOSISH_SYSTEM
static void
init_uids(void)
{
  real_uid = getuid();
  file_uid = geteuid();
  uid_set = 1;
}
#endif


/* drop all additional privileges */
void
drop_privs(void)
{
#ifndef HAVE_DOSISH_SYSTEM
  if (!uid_set)
    init_uids();
  if (real_uid != file_uid) {
    if (setuid(real_uid) < 0) {
      perror("dropping privileges failed");
      exit(EXIT_FAILURE);
    }
    file_uid = real_uid;
  }
#endif
}
