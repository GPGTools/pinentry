/* main.cpp - Secure KDE dialog for PIN entry.
   Copyright (C) 2002 Klarälvdalens Datakonsult AB
   Written by Steffen Hansen <steffen@klaralvdalens-datakonsult.se>.
   
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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA  */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


extern "C"
{
#include "memory.h"
#include "secmem-util.h"
}

#include <new>

#ifdef USE_KDE
# include <kapp.h>
# include <kcmdlineargs.h>
# include <kaboutdata.h>
# include <klocale.h>
#else
# include <qapplication.h>
#endif // USE_KDE

#include "pinentrydialog.h"
#include "pinentrycontroller.h"



#ifdef FALLBACK_CURSES
#include <pinentry.h>
#include <pinentry-curses.h>

pinentry_cmd_handler_t pinentry_cmd_handler = curses_cmd_handler;

int curses_main (int argc, char *argv[])
{
  pinentry_init ();

  /* Consumes all arguments.  */
  if (pinentry_parse_opts (argc, argv))
    {
      printf ("pinentry-curses " VERSION "\n");
      exit (EXIT_SUCCESS);
    }

  if (pinentry_loop ())
    return 1;

  return 0;
}
#endif

extern "C++" {
  extern bool is_secure;
};

#ifndef VERSION
#define VERSION "0.1"
#endif

#ifdef USE_KDE
static const char *description =
        I18N_NOOP("Pinentry");
// INSERT A DESCRIPTION FOR YOUR APPLICATION HERE
 
 
static KCmdLineOptions options[] =
{
  { 0, 0, 0 }
  // INSERT YOUR COMMANDLINE OPTIONS HERE
};
#endif // USE_KDE

void my_new_handler()
{
  secmem_term();
  qFatal("Out of memory!");
}

int qt_main( int argc, char *argv[] )
{
    secmem_init( 16384*8 );
    secmem_set_flags(SECMEM_WARN);
    drop_privs();
    set_new_handler(my_new_handler);
    try {
#ifdef USE_KDE
      KAboutData aboutData( "pinentry", I18N_NOOP("Pinentry"),
			    VERSION, description, KAboutData::License_GPL,
			    "(c) 2001, Steffen Hansen, Klarälvdalens Datakonsult AB", 0, 0, "klaralvdalens-datakonsult.se");
      aboutData.addAuthor("Steffen Hansen, Klarälvdalens Datakonsult AB",0, "steffen@klaralvdalens-datakonsult.se");
      KCmdLineArgs::init( argc, argv, &aboutData );
      KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.
      KApplication app;
#else
      QApplication app( argc, argv );
#endif // USE_KDE
      is_secure = true;
      
      PinEntryController ctrl;
      ctrl.exec();
      return 0;
    } catch( std::bad_alloc& ex ) {
      qDebug("Out of memory, got a %s", ex.what());
      return -1;
    }
}

int main( int argc, char* argv[] ) 
{
#ifdef FALLBACK_CURSES
  if( getenv( "DISPLAY" ) ) {
#endif
    return qt_main( argc, argv );
#ifdef FALLBACK_CURSES
  } else {
    return curses_main( argc, argv );
  }
#endif
}
