/* main.m - Initializing and option parsing.
 Copyright (C) 2002 Klar<E4>lvdalens Datakonsult AB
 Copyright (C) 2003 g10 Code GmbH
 Copyright (c) 2006 Benjamin Donnachie.
 Copyright (C) 2010 Roman Zechmeister
 Written by Steffen Hansen <steffen@klaralvdalens-datakonsult.se>.
 Modified by Marcus Brinkmann <marcus@g10code.de>.
 Adapted / rewritten by Benjamin Donnachie <benjamin@py-soft.co.uk> for MacOS X.
 Modified by Roman Zechmeister

 Code Signature verification added by Lukas Pitschl <lukele@leftandleaving.com>

 This file is part of pinentry-mac.

 pinentry-mac is free software; you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 pinentry-mac is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 02111-1307, USA
*/



#import <Cocoa/Cocoa.h>
#import "pinentry.h"
#import "config.h"


extern pinentry_cmd_handler_t pinentry_cmd_handler;

#ifdef CODE_SIGN_CHECK
#define MACRO_TO_STRING(m) #m
#define CODE_SIGN_CERT MACRO_TO_STRING(CODE_SIGN_CHECK)
BOOL isBundleValidSigned(NSBundle *bundle) {
	SecRequirementRef requirement = nil;
    SecStaticCodeRef staticCode = nil;

    SecStaticCodeCreateWithPath((__bridge CFURLRef)[bundle bundleURL], 0, &staticCode);

	SecRequirementCreateWithString(CFSTR("anchor apple generic and cert leaf = H\""CODE_SIGN_CERT"\""), 0, &requirement);

	OSStatus result = SecStaticCodeCheckValidity(staticCode, 0, requirement);

	if (staticCode) {
		CFRelease(staticCode);
	}
	if (requirement) {
		CFRelease(requirement);
	}
    return result == noErr;
}
#endif


#ifdef FALLBACK_CURSES
#import <pinentry-curses.h>

/* On Mac, the DISPLAY environment variable, which is passed from
 a session to gpg2 to gpg-agent to pinentry and which is used
 on other platforms for falling back to curses, is not completely
 reliable, since some Mac users do not use X11.

 It might be valuable to submit patches so that gpg-agent could
 automatically indicate the state of SSH_CONNECTION to pinentry,
 which would be useful for OS X.

 This pinentry-mac handling will recognize USE_CURSES=1 in
 the environment variable PINENTRY_USER_DATA (which is
 automatically passed from session to gpg2 to gpg-agent to
 pinentry) to allow the user to specify that curses should be
 initialized.

 E.g. in .bashrc or .profile:

 if test "$SSH_CONNECTION" != ""
 then
 export PINENTRY_USER_DATA="USE_CURSES=1"
 fi
 */
int pinentry_mac_is_curses_demanded() {
	const char *s;

	s = getenv ("PINENTRY_USER_DATA");
	if (s && *s) {
		return (strstr(s, "USE_CURSES=1") != NULL);
	}
	return 0;
}
#endif




int main(int argc, char *argv[]) {
	@autoreleasepool {
#ifdef CODE_SIGN_CHECK
		/* Check the validity of the code signature. */
		if (!isBundleValidSigned([NSBundle mainBundle])) {
			NSRunAlertPanel(@"Someone tampered with your installation of pinentry-mac!", @"To keep you safe, pinentry-mac will exit now!\n\nPlease download and install the latest version of GPG Suite from https://gpgtools.org to be sure you have an original version from us!", nil, nil, nil);
			return 1;
		}
#endif

		pinentry_init("pinentry-mac");

		/* Consumes all arguments.  */
		pinentry_parse_opts(argc, argv);

#ifdef FALLBACK_CURSES
		if (pinentry_mac_is_curses_demanded()) {
			// Only load the curses interface.
			pinentry_cmd_handler = curses_cmd_handler;
			if (pinentry_loop()) {
				return 1;
			}
			return 0;
		}
#endif

		// Load GUI.
		return NSApplicationMain(argc, (const char **)argv);
	}
}
