/*
    passwindow.cxx - PassWindow is a more complex fltk dialog with more longer
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

#include "passwindow.h"

#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>

const char *PassWindow::DESCRIPTION  = "Please enter the passphrase:";

PassWindow::PassWindow() : error_(NULL)
{
}

void PassWindow::prompt(const char *name)
{
	set_label(input_, name, PROMPT);
}

void PassWindow::description(const char *name)
{
	set_label(message_, name, DESCRIPTION);
}

void PassWindow::error(const char *name)
{
	set_label(error_, name, "");
}

int PassWindow::init(const int cx, const int cy)
{
	int y = PinWindow::init(cx, cy);

	assert(window_ == Fl_Group::current()); // make_window should all add current

	y = icon_->y(); // move back to icon's

	const int mx = icon_->x()+icon_->w();
	message_->resize(mx, icon_->y(), cx-mx-10, icon_->h());
	message_->align(Fl_Align(FL_ALIGN_LEFT | FL_ALIGN_CLIP | FL_ALIGN_WRAP | FL_ALIGN_INSIDE));
	description(NULL);
	y += icon_->h();

	input_->resize(130, y+5, cx-150, 25);
	input_->labeltype(FL_NORMAL_LABEL);
	prompt(NULL);
	y = input_->y()+input_->h();

	error_ = new Fl_Box(20, y+5, cx-30, 30);
	error_->labelcolor(FL_RED);
	error_->align(Fl_Align(FL_ALIGN_CENTER | FL_ALIGN_WRAP | FL_ALIGN_INSIDE)); // if not fit - user can read
	y = error_->y()+error_->h();
	return y;
}

PassWindow* PassWindow::create()
{
	PassWindow* p = new PassWindow;
	 p->init(460, 185);
	 p->window_->end();
	 p->input_->take_focus();
	 return p;
}
