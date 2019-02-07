/*
   main.cpp - A Fltk based dialog for PIN entry.

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define PGMNAME (PACKAGE_NAME"-fltk")

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <assert.h>

#include "memory.h"
#include <memory>

#include <pinentry.h>
#ifdef FALLBACK_CURSES
#include <pinentry-curses.h>
#endif

#include <string>
#include <string.h>
#include <stdexcept>


#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/fl_ask.H>

#include "pinwindow.h"
#include "passwindow.h"
#include "qualitypasswindow.h"

#define CONFIRM_STRING "Confirm"
#define REPEAT_ERROR_STRING "Texts do not match"
#define OK_STRING "OK"
#define CANCEL_STRING "Cancel"

char *application = NULL;

static std::string escape_accel_utf8(const char *s)
{
	std::string result;
	if (NULL != s)
	{
		result.reserve(strlen(s));
		for (const char *p = s; *p; ++p)
		{
			if ('&' == *p)
				result.push_back(*p);
			result.push_back(*p);
		}
	}
	return result;
}

// For button labels
// Accelerator '_' (used e.g. by GPG2) is converted to '&' (for FLTK)
// '&' is escaped as in escape_accel_utf8()
static std::string convert_accel_utf8(const char *s)
{
	static bool last_was_underscore = false;
	std::string result;
	if (NULL != s)
	{
		result.reserve(strlen(s));
		for (const char *p = s; *p; ++p)
		{
			// & => &&
			if ('&' == *p)
				result.push_back(*p);
			// _ => & (handle '__' as escaped underscore)
			if ('_' == *p)
			{
				if (last_was_underscore)
				{
					result.push_back(*p);
					last_was_underscore = false;
				}
				else
					last_was_underscore = true;
			}
			else
			{
				if (last_was_underscore)
					result.push_back('&');
				result.push_back(*p);
				last_was_underscore = false;
			}
		}
	}
	return result;
}

class cancel_exception
{

};

static int get_quality(const char *passwd, void *ptr)
{
	if (NULL == passwd || 0 == *passwd)
		return 0;

	pinentry_t* pe = reinterpret_cast<pinentry_t*>(ptr);
	return pinentry_inq_quality(*pe, passwd, strlen(passwd));
}

bool is_short(const char *str)
{
	return fl_utf_nb_char(reinterpret_cast<const unsigned char*>(str), strlen(str)) < 16;
}

bool is_empty(const char *str)
{
	return (NULL == str) || (0 == *str);
}

static int fltk_cmd_handler(pinentry_t pe)
{
	int ret = -1;

	try
	{
		// TODO: Add parent window to pinentry-fltk window
		//if (pe->parent_wid){}
		std::string title  = !is_empty(pe->title)?pe->title:PGMNAME;
		std::string ok 	   = convert_accel_utf8(pe->ok?pe->ok:(pe->default_ok?pe->default_ok:OK_STRING));
		std::string cancel = convert_accel_utf8(pe->cancel?pe->cancel:(pe->default_cancel?pe->default_cancel:CANCEL_STRING));

		if (!!pe->pin) // password (or confirmation)
		{
			std::unique_ptr<PinWindow> window;

			bool isSimple = (NULL == pe->quality_bar) &&	// pinenty.h: If this is not NULL ...
							is_empty(pe->error) && is_empty(pe->description) &&
							is_short(pe->prompt);
			if (isSimple)
			{
				assert(NULL == pe->description);
				window.reset(PinWindow::create());
				window->prompt(pe->prompt);
			}
			else
			{
				PassWindow *pass = NULL;

				if (pe->quality_bar) // pinenty.h: If this is not NULL ...
				{
					QualityPassWindow *p = QualityPassWindow::create(get_quality, &pe);
					window.reset(p);
					pass = p;
					p->quality(pe->quality_bar);
				}
				else
				{
					pass = PassWindow::create();
					window.reset(pass);
				}

				if (NULL == pe->description)
				{
					pass->description(pe->prompt);
					pass->prompt(" ");
				}
				else
				{
					pass->description(pe->description);
					pass->prompt(escape_accel_utf8(pe->prompt).c_str());
				}
				pass->description(pe->description);
				pass->prompt(escape_accel_utf8(pe->prompt).c_str());


				if (NULL != pe->error)
					pass->error(pe->error);
			}

			window->ok(ok.c_str());
			window->cancel(cancel.c_str());
			window->title(title.c_str());
			window->showModal((NULL != application)?1:0, &application);

			if (NULL == window->passwd())
				throw cancel_exception();

			const std::string password = window->passwd();
			window.reset();

			if (pe->repeat_passphrase)
			{
				const char *dont_match = NULL;
				do
				{
					if (NULL == dont_match && is_short(pe->repeat_passphrase))
					{
						window.reset(PinWindow::create());
						window->prompt(escape_accel_utf8(pe->repeat_passphrase).c_str());
					}
					else
					{
						PassWindow *pass = PassWindow::create();
						window.reset(pass);
						pass->description(pe->repeat_passphrase);
						pass->prompt(" ");
						pass->error(dont_match);
					}

					window->ok(ok.c_str());
					window->cancel(cancel.c_str());
					window->title(title.c_str());
					window->showModal();

					if (NULL == window->passwd())
						throw cancel_exception();

					if (password == window->passwd())
					{
						pe->repeat_okay = 1;
						ret = 1;
						break;
					}
					else
					{
							dont_match = (NULL!=pe->repeat_error_string)? pe->repeat_error_string:REPEAT_ERROR_STRING;
						}
					} while (true);
				}
				else
					ret = 1;

				pinentry_setbufferlen(pe, password.size()+1);
				if (pe->pin)
				{
					memcpy(pe->pin, password.c_str(), password.size()+1);
					pe->result = password.size();
					ret = password.size();
				}
			}
			else
			{
				// Confirmation or Message Dialog title, desc
				Fl_Window dummy(0,0, 1,1);

				dummy.border(0);
				dummy.show((NULL != application)?1:0, &application);
				dummy.hide();

				fl_message_title(title.c_str());

				int result = -1;

				const char *message = (NULL != pe->description)?pe->description:CONFIRM_STRING;

				if (pe->one_button)
				{
					fl_ok = ok.c_str();
					fl_message("%s", message);
					result = 1; // OK
				}
				else if (pe->notok)
				{
					switch (fl_choice("%s", ok.c_str(), cancel.c_str(), pe->notok, message))
					{
					case 0: result = 1; break;
					case 2: result = 0; break;
					default:
					case 1: result = -1;break;
					}
				}
				else
				{
					switch (fl_choice("%s", ok.c_str(), cancel.c_str(), NULL, message))
					{
					case 0: result = 1; break;
					default:
					case 1: result = -1;break;
					}
				}

				// cancel ->         pe->canceled = true, 0
				// ok/y -> 1
				// no -> 0
				if (-1 == result)
					pe->canceled = true;
				ret = (1 == result);
			}
			Fl::check();
		}
		catch (const cancel_exception&)
		{
			ret = -1;
		}
		catch (...)
		{
			ret = -1;
		}
			// do_touch_file(pe); only for NCURSES?
		return ret;
	}

pinentry_cmd_handler_t pinentry_cmd_handler = fltk_cmd_handler;

int main(int argc, char *argv[])
{
	application = *argv;
	pinentry_init(PGMNAME);

#ifdef FALLBACK_CURSES
	if (!pinentry_have_display(argc, argv))
		pinentry_cmd_handler = curses_cmd_handler;
	else
#endif
	{
		//FLTK understood only -D (--display)
		// and should be converted into -di[splay]
		const static struct option long_options[] =
		{
			{"display", required_argument,	0, 'D' },
			{NULL, 		no_argument,		0, 0   }
		};

		for (int i = 0; i < argc-1; ++i)
		{
			switch (getopt_long(argc-i, argv+i, "D:", long_options, NULL))
			{
				case 'D':
					{
						char* emul[] = {application, (char*)"-display", optarg};
						Fl::args(3, emul);
						i = argc;
						break;
					}
				default:
					break;
			}
		}
	}

	pinentry_parse_opts(argc, argv);
	return pinentry_loop() ?EXIT_FAILURE:EXIT_SUCCESS;
}
