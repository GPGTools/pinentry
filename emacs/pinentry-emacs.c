/* pinentry-emacs.c - A secure emacs dialog for PIN entry, library version
 * Copyright (C) 2015 Daiki Ueno
 *
 * This file is part of PINENTRY.
 *
 * PINENTRY is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * PINENTRY is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "pinentry.h"
#include "pinentry-emacs.h"

pinentry_cmd_handler_t pinentry_cmd_handler = emacs_cmd_handler;



int
main (int argc, char *argv[])
{
  pinentry_init ("pinentry-emacs");

  pinentry_parse_opts (argc, argv);

  if (!pinentry_emacs_init ())
    return 1;

  if (pinentry_loop ())
    return 1;

  return 0;
}
