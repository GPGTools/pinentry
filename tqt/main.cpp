/* main.cpp - Secure KDE dialog for PIN entry.
 * Copyright (C) 2002 Klarälvdalens Datakonsult AB
 * Copyright (C) 2003 g10 Code GmbH
 * Written by Steffen Hansen <steffen@klaralvdalens-datakonsult.se>.
 * Modified by Marcus Brinkmann <marcus@g10code.de>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <errno.h>

#include <ntqapplication.h>
#include <ntqwidget.h>
#include <ntqmessagebox.h>
#include "secqstring.h"

#include "pinentrydialog.h"

#include "pinentry.h"

#ifdef FALLBACK_CURSES
#include <pinentry-curses.h>
#endif

static TQString escape_accel( const TQString & s ) {

  TQString result;
  result.reserve( 2 * s.length());

  bool afterUnderscore = false;

  for ( unsigned int i = 0, end = s.length() ; i != end ; ++i ) {
    const TQChar ch = s[i];
    if ( ch == TQChar ( '_' ) )
      {
        if ( afterUnderscore ) // escaped _
          {
            result += TQChar ( '_' );
            afterUnderscore = false;
          }
        else // accel
          {
            afterUnderscore = true;
          }
      }
    else
      {
        if ( afterUnderscore || // accel
             ch == TQChar ( '&' ) ) // escape & from being interpreted by TQt
          result += TQChar ( '&' );
        result += ch;
        afterUnderscore = false;
      }
  }

  if ( afterUnderscore )
    // trailing single underscore: shouldn't happen, but deal with it robustly:
    result += TQChar ( '_' );

  return result;
}


/* Hack for creating a TQWidget with a "foreign" window ID */
class ForeignWidget : public TQWidget
{
public:
  ForeignWidget( WId wid ) : TQWidget( 0 )
  {
    TQWidget::destroy();
    create( wid, false, false );
  }

  ~ForeignWidget()
  {
    destroy( false, false );
  }
};

static int
qt_cmd_handler (pinentry_t pe)
{
  TQWidget *parent = 0;

  int want_pass = !!pe->pin;

  if (want_pass)
    {
      /* FIXME: Add parent window ID to pinentry and GTK.  */
      if (pe->parent_wid)
	parent = new ForeignWidget (pe->parent_wid);

      PinEntryDialog pinentry (parent, NULL, true, !!pe->quality_bar);

      pinentry.setPinentryInfo (pe);
      pinentry.setPrompt (TQString::fromUtf8 (pe->prompt));
      pinentry.setDescription (TQString::fromUtf8 (pe->description));
      /* If we reuse the same dialog window.  */
#if 0
      pinentry.setText (SecTQString::null);
#endif

      if (pe->ok)
	pinentry.setOkText (escape_accel (TQString::fromUtf8 (pe->ok)));
      else if (pe->default_ok)
	pinentry.setOkText (escape_accel (TQString::fromUtf8 (pe->default_ok)));

      if (pe->cancel)
	pinentry.setCancelText (escape_accel (TQString::fromUtf8 (pe->cancel)));
      else if (pe->default_cancel)
	pinentry.setCancelText
          (escape_accel (TQString::fromUtf8 (pe->default_cancel)));

      if (pe->error)
	pinentry.setError (TQString::fromUtf8 (pe->error));
      if (pe->quality_bar)
	pinentry.setQualityBar (TQString::fromUtf8 (pe->quality_bar));
      if (pe->quality_bar_tt)
	pinentry.setQualityBarTT (TQString::fromUtf8 (pe->quality_bar_tt));

      bool ret = pinentry.exec ();
      if (!ret)
	return -1;

      char *pin = (char *) pinentry.text().utf8();
      if (!pin)
	return -1;

      int len = strlen (pin);
      if (len >= 0)
	{
	  pinentry_setbufferlen (pe, len + 1);
	  if (pe->pin)
	    {
	      strcpy (pe->pin, pin);
	      ::secmem_free (pin);
	      return len;
	    }
	}
      ::secmem_free (pin);
      return -1;
    }
  else
    {
      TQString desc = TQString::fromUtf8 (pe->description? pe->description : "");
      TQString ok   = escape_accel
        (TQString::fromUtf8 (pe->ok ? pe->ok :
                            pe->default_ok ? pe->default_ok : "_OK"));
      TQString can  = escape_accel
        (TQString::fromUtf8 (pe->cancel ? pe->cancel :
                            pe->default_cancel? pe->default_cancel: "_Cancel"));
      bool ret;

      ret = TQMessageBox::information (parent, "", desc, ok, can );

      return !ret;
    }
}

pinentry_cmd_handler_t pinentry_cmd_handler = qt_cmd_handler;

int
main (int argc, char *argv[])
{
  pinentry_init ("pinentry-tqt");

#ifdef FALLBACK_CURSES
  if (!pinentry_have_display (argc, argv))
    pinentry_cmd_handler = curses_cmd_handler;
  else
#endif
    {
      /* TQt does only understand -display but not --display; thus we
         are fixing that here.  The code is pretty simply and may get
         confused if an argument is called "--display". */
      char **new_argv, *p;
      size_t n;
      int i, done;

      for (n=0,i=0; i < argc; i++)
        n += strlen (argv[i])+1;
      n++;
      new_argv = (char**)calloc (argc+1, sizeof *new_argv);
      if (new_argv)
        *new_argv = (char*)malloc (n);
      if (!new_argv || !*new_argv)
        {
          fprintf (stderr, "pinentry-tqt: can't fixup argument list: %s\n",
                   strerror (errno));
          exit (EXIT_FAILURE);

        }
      for (done=0,p=*new_argv,i=0; i < argc; i++)
        if (!done && !strcmp (argv[i], "--display"))
          {
            new_argv[i] = (char*)"-display";
            done = 1;
          }
        else
          {
            new_argv[i] = strcpy (p, argv[i]);
            p += strlen (argv[i]) + 1;
          }

      /* We use a modal dialog window, so we don't need the application
         window anymore.  */
      i = argc;
      new TQApplication (i, new_argv);
    }


  /* Consumes all arguments.  */
  pinentry_parse_opts (argc, argv);
//   if (pinentry_parse_opts (argc, argv))
//     {
//       printf ("pinentry-tqt (pinentry) " VERSION "\n");
//       exit (EXIT_SUCCESS);
//     }

  if (pinentry_loop ())
    return 1;

  return 0;
}
