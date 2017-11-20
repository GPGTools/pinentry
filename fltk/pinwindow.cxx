/*
    pinwindow.cxx - PinWindow is a simple fltk dialog for entring password
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

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Secret_Input.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Pixmap.H>

#include "memory.h"

#include "encrypt.xpm"
#include "icon.xpm"

#include "pinwindow.h"

const char *PinWindow::TITLE 		= "Password";
const char *PinWindow::BUTTON_OK 	= "OK";
const char *PinWindow::BUTTON_CANCEL = "Cancel";
const char *PinWindow::PROMPT		= "Passphrase:";

static const char *timeout_format = "%s(%d)";

static Fl_Pixmap encrypt(encrypt_xpm);
static Fl_Pixmap icon(icon_xpm);

PinWindow::PinWindow() : window_(NULL)
				,message_(NULL) ,input_(NULL) ,ok_(NULL) ,cancel_(NULL)
				,cancel_name_(BUTTON_CANCEL)
				,passwd_(NULL) ,timeout_(0)
{
}

PinWindow::~PinWindow()
{
	wipe();
	release();
	delete window_;
}

void PinWindow::release()
{
	if (NULL != passwd_)
	{
		memset(passwd_, 0, strlen(passwd_));
		secmem_free(passwd_);
	}
	passwd_ = NULL;
}

void PinWindow::title(const char *name)
{
	set_label(window_, name, TITLE);
}

void PinWindow::ok(const char* name)
{
	set_label(ok_, name, BUTTON_OK);
}

void PinWindow::cancel(const char* label)
{
	if (NULL != label && 0 != *label)
		cancel_name_ = label;
	else
		cancel_name_ = BUTTON_CANCEL;

	update_cancel_label();
}

void PinWindow::prompt(const char *name)
{
	set_label(message_, name, PROMPT);
}

void PinWindow::timeout(unsigned int time)
{
	if (timeout_ == time)
		return;

	// A xor B ~ A != B
	if ( (time>0) != (timeout_>0))
	{
		//enable or disable
		if (time>0)
			Fl::add_timeout(1.0, timeout_cb, this);
		else
			Fl::remove_timeout(timeout_cb, this);
	}

	timeout_=time;
	update_cancel_label();
	--timeout_;
}

void PinWindow::showModal()
{
	if (NULL != window_)
	{
		window_->show();
		Fl::run();
	}
	Fl::check();
}

void PinWindow::showModal(const int argc, char* argv[])
{
	if (NULL != window_)
	{
		window_->show(argc, argv);
		Fl::run();
	}
	Fl::check();
}

int PinWindow::init(const int cx, const int cy)
{
	assert(NULL == window_);
	window_ = new Fl_Window(cx, cy, TITLE);

	Fl_RGB_Image app(&icon);
	window_->icon(&app);

	icon_ = new Fl_Box(10, 10, 64, 64);
	icon_->image(encrypt);

    message_ = new Fl_Box(79, 5, cx-99, 44, PROMPT);
	message_->align(Fl_Align(FL_ALIGN_LEFT_TOP | FL_ALIGN_WRAP | FL_ALIGN_INSIDE)); // left

	input_ = new Fl_Secret_Input(79, 59, cx-99, 25);
	input_->labeltype(FL_NO_LABEL);


	const int button_y = cy-40;
    ok_ = new Fl_Return_Button(cx-300, button_y, 120, 25, BUTTON_OK);
	ok_->callback(ok_cb, this);

	cancel_ = new Fl_Button(cx-160, button_y, 120, 25);
	update_cancel_label();
	cancel_->callback(cancel_cb, this);

	window_->hotspot(input_);
	window_->set_modal();

	return 84;
};

void PinWindow::update_cancel_label()
{
	if (timeout_ == 0)
	{
		cancel_->label(cancel_name_.c_str());
	}
	else
	{
		const size_t len = cancel_name_.size()+strlen(timeout_format)+10+1;
		char *buf = new char[len];
		snprintf(buf, len, timeout_format, cancel_name_.c_str(), timeout_);
		cancel_->copy_label(buf);
		delete[] buf; // no way to attach label
	}
}

void PinWindow::timeout_cb(void* val)
{
	PinWindow *self = reinterpret_cast<PinWindow*>(val);
	if (self->timeout_ == 0)
	{
		cancel_cb(self->cancel_, self);
	}
	else
	{
		self->update_cancel_label();
		--self->timeout_;
		Fl::repeat_timeout(1.0, timeout_cb, val);
	}
}

void PinWindow::cancel_cb(Fl_Widget *button, void *val)
{
	PinWindow *self = reinterpret_cast<PinWindow*>(val);

	self->wipe();
	self->release();
	self->window_->hide();
}

void PinWindow::ok_cb(Fl_Widget *button, void *val)
{
	PinWindow *self = reinterpret_cast<PinWindow*>(val);

	self->release();

	const char *passwd = self->input_->value();
	size_t len = strlen(passwd)+1;
	self->passwd_ = reinterpret_cast<char*>(secmem_malloc(len));
	if (NULL != self->passwd_)
		memcpy(self->passwd_, passwd, len);

	self->wipe();
	self->window_->hide();
}

void PinWindow::wipe()
{
	int len = input_->size();
	char* emul = new char[len+1];
	for (int i=0; i<len; ++i)
	{
		emul[i] = 0x20 + rand()%(128-20); // [20..127]
	}
	emul[len] = 0;
	input_->replace(0, len, emul, len);
	delete[] emul;

	input_->value(TITLE); // hide size too
}

PinWindow*  PinWindow::create()
{
	PinWindow* p = new  PinWindow;
	 p->init(410, 140);
	 p->window_->end();
	 p->input_->take_focus();
	 return p;
}
