/*
   main.cpp - A (not yet) secure Qt 4 dialog for PIN entry.

   Copyright (C) 2002, 2008 Klar‰lvdalens Datakonsult AB (KDAB)
   Copyright (C) 2003 g10 Code GmbH
   Copyright 2007 Ingo Kl√∂cker

   Written by Steffen Hansen <steffen@klaralvdalens-datakonsult.se>.
   Modified by Marcus Brinkmann <marcus@g10code.de>.
   Modified by Marc Mutz <marc@kdab.com>

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
*/


#include "pinentrydialog.h"
#include "pinentry.h"

#include <qapplication.h>
#include <QString>
#include <qwidget.h>
#include <qmessagebox.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#ifdef FALLBACK_CURSES
#include <pinentry-curses.h>
#endif

/* Hack for creating a QWidget with a "foreign" window ID */
class ForeignWidget : public QWidget
{
public:
  explicit ForeignWidget( WId wid ) : QWidget( 0 )
  {
    QWidget::destroy();
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
  QWidget *parent = 0;

  int want_pass = !!pe->pin;

  if (want_pass)
    {
      /* FIXME: Add parent window ID to pinentry and GTK.  */
      if (pe->parent_wid)
	parent = new ForeignWidget ((WId) pe->parent_wid);

      PinEntryDialog pinentry (parent, 0, true, !!pe->quality_bar);

      pinentry.setPinentryInfo (pe);
      pinentry.setPrompt (QString::fromUtf8 (pe->prompt));
      pinentry.setDescription (QString::fromUtf8 (pe->description));
      /* If we reuse the same dialog window.  */
      pinentry.setPin (secqstring());

      if (pe->ok)
	pinentry.setOkText (QString::fromUtf8 (pe->ok));
      if (pe->cancel)
	pinentry.setCancelText (QString::fromUtf8 (pe->cancel));
      if (pe->error)
	pinentry.setError (QString::fromUtf8 (pe->error));
      if (pe->quality_bar)
	pinentry.setQualityBar (QString::fromUtf8 (pe->quality_bar));
      if (pe->quality_bar_tt)
	pinentry.setQualityBarTT (QString::fromUtf8 (pe->quality_bar_tt));

      bool ret = pinentry.exec ();
      if (!ret)
	return -1;

      const secstring pinUtf8 = toUtf8( pinentry.pin() );
      const char *pin = pinUtf8.data();
      if (!pin)
	return -1;

      int len = strlen (pin);
      if (len >= 0)
	{
	  pinentry_setbufferlen (pe, len + 1);
	  if (pe->pin)
	    {
	      strcpy (pe->pin, pin);
	      return len;
	    }
	}
      return -1;
    }
  else
    {
      QString desc = QString::fromUtf8 (pe->description? pe->description :"");
      QString ok   = QString::fromUtf8 (pe->ok ? pe->ok : "OK");
      QString can  = QString::fromUtf8 (pe->cancel ? pe->cancel : "Cancel");
      bool ret;

      ret = QMessageBox::information (parent, "", desc, ok, can );

      return !ret;
    }
}

pinentry_cmd_handler_t pinentry_cmd_handler = qt_cmd_handler;

int
main (int argc, char *argv[])
{
  pinentry_init ("pinentry-qt4");

#ifdef FALLBACK_CURSES
  if (!pinentry_have_display (argc, argv))
    pinentry_cmd_handler = curses_cmd_handler;
  else
#endif
    {
      /* Qt does only understand -display but not --display; thus we
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
          fprintf (stderr, "pinentry-qt4: can't fixup argument list: %s\n",
                   strerror (errno));
          exit (EXIT_FAILURE);

        }
      for (done=0,p=*new_argv,i=0; i < argc; i++)
        if (!done && !strcmp (argv[i], "--display"))
          {
            new_argv[i] = "-display";
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
      new QApplication (i, new_argv);
    }


  /* Consumes all arguments.  */
  if (pinentry_parse_opts (argc, argv))
    {
      printf ("pinentry-qt4 (pinentry) " /* VERSION */ "\n");
      exit (EXIT_SUCCESS);
    }

  if (pinentry_loop ())
    return 1;

  return 0;
}
