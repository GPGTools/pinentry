#include <new>
#include <kapp.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>

#include "memory.h"
#include "util.h"


#include "pinentrydialog.h"
#include "pinentrycontroller.h"

extern "C++" {
  extern bool is_secure;
};

#define VERSION "0.1"

static const char *description =
        I18N_NOOP("Pinentry");
// INSERT A DESCRIPTION FOR YOUR APPLICATION HERE
 
 
static KCmdLineOptions options[] =
{
  { 0, 0, 0 }
  // INSERT YOUR COMMANDLINE OPTIONS HERE
};

void my_new_handler()
{
  secmem_term();
  qFatal("Out of memory!");
}

int main( int argc, char** argv ) 
{
  secmem_init( 16384*8 );
  secmem_set_flags(SECMEM_WARN);
  drop_privs();
  set_new_handler(my_new_handler);
  try {
    KAboutData aboutData( "pinentry", I18N_NOOP("Pinentry"),
			  VERSION, description, KAboutData::License_GPL,
			  "(c) 2001, Steffen Hansen, Klarälvdalens Datakonsult AB", 0, 0, "klaralvdalens-datakonsult.se");
    aboutData.addAuthor("Steffen Hansen, Klarälvdalens Datakonsult AB",0, "steffen@klaralvdalens-datakonsult.se");
    KCmdLineArgs::init( argc, argv, &aboutData );
    KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.
    KApplication app;
    is_secure = true;

    PinEntryController ctrl;
    ctrl.exec();
    return 0;
  } catch( std::bad_alloc& ex ) {
    qDebug("Out of memory, got a %s", ex.what());
  }
}
