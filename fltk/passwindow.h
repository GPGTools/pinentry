/*
    passwindow.h - PassWindow is a more complex fltk dialog with more longer
    desc field and possibility to show some error text.
    if needed qualitybar - should be used QualityPassWindow.

    Copyright (C) 2016 Anatoly madRat L. Berenblit

    Written by Anatoly madRat L. Berenblit <madrat-@users.noreply.github.com>.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
    SPDX-License-Identifier: GPL-2.0+
*/

#ifndef __PASSWINDOW_H__
#define __PASSWINDOW_H__

#include "pinwindow.h"

class PassWindow : public PinWindow
{
protected:
	static const char *DESCRIPTION;

protected:
	Fl_Box      *error_;
	PassWindow();

public:
	virtual void prompt(const char *message);
	virtual void description(const char *desc);
	virtual void error(const char *err);

	static PassWindow* create();

protected:
	virtual int init(const int cx, const int cy);
};

#endif //#ifndef __PASSWINDOW_H__
