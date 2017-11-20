/*
    pinwindow.h - PinWindow is a simple fltk dialog for entring password
    with timeout. if needed description (long text), error message, qualitybar
    and etc should used PassWindow.

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

#ifndef __PINWINDOW_H__
#define __PINWINDOW_H__

#include "config.h"

class Fl_Window;
class Fl_Box;
class Fl_Input;
class Fl_Button;
class Fl_Widget;

#include <assert.h>
#include <string>

class PinWindow
{
protected:
	static const char *TITLE;
	static const char *BUTTON_OK;
	static const char *BUTTON_CANCEL;
	static const char *PROMPT;

protected:
	PinWindow(const PinWindow&);
	PinWindow& operator=(const PinWindow&);

	Fl_Window	*window_;
	Fl_Box		*icon_;

	Fl_Box		*message_;
	Fl_Input	*input_;

	Fl_Button	*ok_, *cancel_;

	std::string cancel_name_;
	char *passwd_;			// SECURE_MEMORY
	unsigned int timeout_; 	// click cancel if timeout

public:
	virtual ~PinWindow();

	static PinWindow* create();

	inline const char*   passwd() const { return passwd_; }

	virtual void timeout(unsigned int time);						// 0 - infinity, seconds
	virtual void title(const char *title);
	virtual void ok(const char* ok);
	virtual void cancel(const char* cancel);
	virtual void prompt(const char *message);

	virtual void showModal();
	virtual void showModal(const int argc, char* argv[]);

protected:
	PinWindow();

	void wipe();		// clear UI memory
	void release();		// clear secure memory
	void update_cancel_label();

	virtual int init(const int cx, const int cy);

	//callbacks
	static void cancel_cb(Fl_Widget *button, void *val);
	static void ok_cb(Fl_Widget *button, void *val);
	static void timeout_cb(void*);

	// ISSUE: Fl_Window component in tinycore works only as Fl_Window::label(...); not Fl_Widget
	template <class TWidget> void set_label(TWidget* widget, const char *label, const char *def)
	{
		assert(NULL != widget); // widget must be created

		if (NULL != widget)
		{
			if (NULL != label && 0 != *label)
				widget->copy_label(label);
			else
				widget->label(def);
		}
	};
};

#endif //#ifndef __PINWINDOW_H__
