/*
    qualitypasswindow.cxx - QualityPassWindow pin entry
    with Password QualityBar and etc

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

#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Progress.H>

#include "qualitypasswindow.h"

const char *QualityPassWindow::QUALITY = "Quality";

QualityPassWindow::QualityPassWindow(QualityPassWindow::GetQualityFn qualify, void* ptr)
						: get_quality_(qualify)
						,get_quality_user_(ptr)
						,quality_(NULL)
{
	assert(NULL != qualify);
}

void QualityPassWindow::input_changed(Fl_Widget *input, void *val)
{
    QualityPassWindow  *self = reinterpret_cast<QualityPassWindow*>(val);

    assert(NULL != self->get_quality_);   // function should be assigned in ctor
    assert(NULL != self->quality_);       // quality progress bar must be created in init

	if (NULL != self->quality_ && NULL != self->get_quality_)
	{
		int result = self->get_quality_(self->input_->value(), self->get_quality_user_);
		bool isErr = (result <= 0);
		if (isErr)
			result = -result;
		self->quality_->selection_color(isErr?FL_RED:FL_GREEN);
		self->quality_->value(std::min(result, 100));
	}
}

QualityPassWindow* QualityPassWindow::create(QualityPassWindow::GetQualityFn qualify, void *user)
{
	QualityPassWindow *p = new QualityPassWindow(qualify, user);
	p->init(460, 215);
	p->window_->end();
	p->input_->take_focus();
	return p;
}

void QualityPassWindow::quality(const char *name)
{
	set_label(quality_, name, QUALITY);
}

int QualityPassWindow::init(const int cx, const int cy)
{
	int y = PassWindow::init(cx, cy);
	assert(window_ == Fl_Group::current()); // make_window should all add current

	input_->when(FL_WHEN_CHANGED);
	input_->callback(input_changed, this);

	y = input_->y() + input_->h();

	quality_ = new Fl_Progress(input_->x(), y+5, input_->w(), 25, QUALITY);
	quality_->align(Fl_Align(FL_ALIGN_LEFT | FL_ALIGN_CLIP | FL_ALIGN_WRAP));
	quality_->maximum(100.1);
	quality_->minimum(0.0);
	y = quality_->y() + quality_->h();

	error_->position(error_->x(), y+5);

	return error_->y() + error_->h();
}
