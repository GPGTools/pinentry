/*
    qualitypasswindow.h - QualityPassWindow pin entry with Password QualityBar
    and etc

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

#ifndef __QUALITYPASSWINDOW_H__
#define __QUALITYPASSWINDOW_H__

#include "passwindow.h"
class Fl_Progress;

class QualityPassWindow : public PassWindow
{
protected:
	static const char *QUALITY;

public:
	typedef int (*GetQualityFn)(const char *passwd, void *ptr);

	static QualityPassWindow* create(GetQualityFn qualify, void* user);

	void quality(const char *name);

protected:
	QualityPassWindow(GetQualityFn qualify, void*);

	const GetQualityFn  get_quality_;
	void* const         get_quality_user_;

	Fl_Progress *quality_;
	virtual int init(const int cx, const int cy);

	static void input_changed(Fl_Widget *input, void *val);
};

#endif //#ifndef __QUALITYPASSWINDOW_H__
