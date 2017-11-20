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

#ifndef PINENTRY_EMACS_H
#define PINENTRY_EMACS_H

#include "pinentry.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Enable pinentry command handler which interacts with Emacs, if
   INSIDE_EMACS envvar is set.  This function shall be called upon
   receiving an Assuan request "OPTION allow-emacs-prompt".  */
void pinentry_enable_emacs_cmd_handler (void);

/* Initialize the Emacs interface, return true if success.  */
int pinentry_emacs_init (void);

int emacs_cmd_handler (pinentry_t pinentry);

#ifdef __cplusplus
}
#endif

#endif	/* PINENTRY_EMACS_H */
