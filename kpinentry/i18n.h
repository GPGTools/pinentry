/* Internationalization support
 * Copyright (C) 1999 Robert Bihlmeyer <robbe@orcus.priv.at>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _I18N_H
#define _I18N_H

#include <locale.h>
#include <libintl.h>
#define _(a) (a)
#ifdef gettext_noop
# define N_(a) gettext_noop (a)
#else
# define N_(a) (a)
#endif

#endif /* _I18N_H */
