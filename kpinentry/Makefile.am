CXXFLAGS += $(USE_EXCEPTIONS)
bin_PROGRAMS = kpinentry
kpinentry_SOURCES = pinentrydialog.cpp pinentrycontroller.cpp secmem.cpp cppmemory.cpp util.cpp main.cpp 
kpinentry_LDADD   =  $(LIB_QT) $(LIB_KDECORE) $(LIB_KDEUI)

EXTRA_DIST = main.cpp pinentrydialog.cpp pinentrydialog.h \
	pinentrycontroller.cpp pinentrycontroller.h \
	secmem.cpp memory.h cppmemory.cpp cppmemory.h util.cpp util.h pinentry.desktop  

install-data-local:
	$(mkinstalldirs) $(kde_appsdir)/Applications/
	$(INSTALL_DATA) $(srcdir)/pinentry.desktop $(kde_appsdir)/Applications/pinentry.desktop
	#$(mkinstalldirs) $(kde_icondir)/locolor/32x32/apps/
	#$(INSTALL_DATA) $(srcdir)/lo32-app-pinentry.png $(kde_icondir)/locolor/32x32/apps/pinentry.png
	#$(mkinstalldirs) $(kde_icondir)/locolor/16x16/apps/
	#$(INSTALL_DATA) $(srcdir)/lo16-app-pinentry.png $(kde_icondir)/locolor/16x16/apps/pinentry.png

uninstall-local:
	-rm -f $(kde_appsdir)/Applications/pinentry.desktop
	#-rm -f $(kde_icondir)/locolor/32x32/apps/pinentry.png
	#-rm -f $(kde_icondir)/locolor/16x16/apps/pinentry.png

####### kdevelop will overwrite this part!!! (end)############
# this 10 paths are KDE specific. Use them:
# kde_htmldir       Where your docs should go to. (contains lang subdirs)
# kde_appsdir       Where your application file (.kdelnk) should go to. 
# kde_icondir       Where your icon should go to.
# kde_minidir       Where your mini icon should go to.
# kde_datadir       Where you install application data. (Use a subdir)
# kde_locale        Where translation files should go to.(contains lang subdirs)
# kde_cgidir        Where cgi-bin executables should go to.
# kde_confdir       Where config files should go to.
# kde_mimedir       Where mimetypes should go to.
# kde_toolbardir    Where general toolbar icons should go to.
# kde_wallpaperdir  Where general wallpapers should go to.

# set the include path for X, qt and KDE
INCLUDES= $(all_includes) $(ASSUAN_INCLUDES)

METASOURCES = AUTO

# the library search path. 
kpinentry_LDFLAGS = $(all_libraries) $(ASSUAN_LDFLAGS) $(KDE_RPATH)

# Uncomment the following two lines if you add a ui.rc file for your application to make use of
# KDE´s XML GUI builing
#rcdir = $(kde_datadir)/pinentry
#rc_DATA = pinentryui.rc

#WARNING: if you use a ui.rc file above, use:

# messages: rc.cpp

# instead of 

# messages:

messages: 
	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
	if test -n "$$LIST"; then \
	 $(XGETTEXT) $$LIST -o $(podir)/pinentry.pot; \
	fi

