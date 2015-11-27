noinst_PROGRAMS = pinentry-mac
libexec_PROGRAMS = pinentry-mac.app

if FALLBACK_CURSES
ncurses_include = $(NCURSES_INCLUDE)
libcurses = ../pinentry/libpinentry-curses.a $(LIBCURSES) $(LIBICONV)
else
ncurses_include =
libcurses =
endif

FRAMEWORKS = -framework Cocoa -framework Security
AM_LDFLAGS = $(FRAMEWORKS) -Wl,-rpath,@executable_path/../Frameworks

AM_OBJCFLAGS = $(COMMON_CFLAGS) $(ncurses_include) -I$(top_srcdir)/secmem -I$(top_srcdir)/pinentry -fobjc-arc
LDADD = ../pinentry/libpinentry.a ../secmem/libsecmem.a \
	$(COMMON_LIBS) $(LIBCAP) $(libcurses)

pinentry_mac_SOURCES = KeychainSupport.m NSStringExtensions.m PinentryMac.m \
	ShortcutHandlingFields.m PassphraseLengthFormatter.m AppDelegate.m main.m

app_RESOURCES = Icon.icns
app_XIBS = Main.xib Pinentry.xib
app_STRINGS = de.strings en.strings

app_NIBS = $(app_XIBS:xib=nib)
app_LPROJS = $(app_STRINGS:strings=lproj)
app_RES = $(app_NIBS) $(app_LPROJS) $(app_RESOURCES)

CLEANFILES = $(app_NIBS)

temp_app_dir = pinentry-mac.app_temp
contents_dir = $(temp_app_dir)/Contents


.xib.nib:
	ibtool --compile $@ $<

.strings.lproj:
	rm -rf $@
	mkdir $@
	cp $< $@/Localizable.strings

clean-local:
	[[ -z "$(app_LPROJS)" ]] || rm -rf $(app_LPROJS)
	[[ -z "$(temp_app_dir)" ]] || rm -rf $(temp_app_dir)

clean-libexecPROGRAMS:
	[[ -z "$(libexec_PROGRAMS)" ]] || rm -rf $(libexec_PROGRAMS)

pinentry-mac.app$(EXEEXT): pinentry-mac $(app_RES) Info.plist
	@rm -rf pinentry-mac.app $(temp_app_dir)
	mkdir -p $(contents_dir)/MacOS $(contents_dir)/Resources
	cp -R $(app_RES) $(contents_dir)/Resources/
	./copyInfoPlist.sh Info.plist $(contents_dir)
	cp pinentry-mac $(contents_dir)/MacOS/
	mv $(temp_app_dir) pinentry-mac.app
