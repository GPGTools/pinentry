/* AppDelegate.m - pinentry handler.
 Copyright © Roman Zechmeister, 2015

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

#import "AppDelegate.h"
#import "pinentry.h"
#import "PinentryMac.h"
#import "KeychainSupport.h"
#import "NSStringExtensions.h"


@implementation AppDelegate

static int mac_cmd_handler (pinentry_t pe);
pinentry_cmd_handler_t pinentry_cmd_handler = mac_cmd_handler;


- (void)applicationDidFinishLaunching:(NSNotification *)notification {
	[[NSUserDefaults standardUserDefaults] addSuiteNamed:@"org.gpgtools.common"];
	[[NSUserDefaults standardUserDefaults] registerDefaults:@{@"UseKeychain": @YES}];

	if (pinentry_loop()) {
		exit(1);
	}
	exit(0);
}

NSDictionary *parseUserData(pinentry_t pe) {
	NSMutableDictionary *dict = [NSMutableDictionary dictionary];
	NSString *userID = @"", *name = @"", *email = @"", *comment = @"", *keyID = @"";
	NSString *description = nil;


	// Get description from pinentry.
	if (pe->description) {
		description = [[NSString gpgStringWithCString:pe->description] stringByReplacingOccurrencesOfString:@"\\n" withString:@"\n"];
	}

	//Parse description to get UserID.
	NSArray *lines = [description componentsSeparatedByString:@"\n"];
	if (lines.count > 2) {
		NSString *workString = [[lines objectAtIndex:1] stringBetweenString:@"\"" andString:@"\"" needEnd:YES];

		if (workString) {
			userID = workString;
			NSUInteger textLength = workString.length;
			NSRange range;

			// Find e-mail.
			if ([workString hasSuffix:@">"] && (range = [workString rangeOfString:@" <" options:NSBackwardsSearch]).length > 0) {
				range.location += 2;
				range.length = textLength - range.location - 1;

				email = [workString substringWithRange:range];

				workString = [workString substringToIndex:range.location - 2];
				textLength -= (range.length + 3);
			}

			// Find comment.
			range = [workString rangeOfString:@" (" options:NSBackwardsSearch];
			if (range.length > 0 && range.location > 0 && [workString hasSuffix:@")"]) {
				range.location += 2;
				range.length = textLength - range.location - 1;

				comment = [workString substringWithRange:range];

				workString = [workString substringToIndex:range.location - 2];
			}

			// Now, workString only contains the name.
			name = workString;
		}

		workString = [description stringBetweenString:@"ID " andString:@"," needEnd:YES];
		if (workString.length == 8 || workString.length == 16) {
			keyID = workString;
		}
	}


	// PINENTRY_USER_DATA should be comma-seperated.
	const char *cUserData = getenv("PINENTRY_USER_DATA");
	NSString *userData = nil;
	if (cUserData) {
		userData = [NSString gpgStringWithCString:cUserData];
	}

	if (userData) {
		/*
		 DESCRIPTION is percent escaped and additionally can use the following placeholders:
		 %KEYID, %USERID, %EMAIL, %COMMENT, %NAME
		*/
		NSMutableString *descriptionTemplate = [[userData stringBetweenString:@"DESCRIPTION=" andString:@"," needEnd:NO] mutableCopy];

		if (descriptionTemplate) {
			[descriptionTemplate replaceOccurrencesOfString:@"%USERID" withString:userID options:0 range:NSMakeRange(0, descriptionTemplate.length)];
			[descriptionTemplate replaceOccurrencesOfString:@"%EMAIL" withString:email options:0 range:NSMakeRange(0, descriptionTemplate.length)];
			[descriptionTemplate replaceOccurrencesOfString:@"%COMMENT" withString:comment options:0 range:NSMakeRange(0, descriptionTemplate.length)];
			[descriptionTemplate replaceOccurrencesOfString:@"%NAME" withString:name options:0 range:NSMakeRange(0, descriptionTemplate.length)];
			[descriptionTemplate replaceOccurrencesOfString:@"%KEYID" withString:keyID options:0 range:NSMakeRange(0, descriptionTemplate.length)];

			NSString *newDescription = [descriptionTemplate stringByReplacingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
			if (newDescription) {
				description = newDescription;
			}
		}
	}

	if (description) {
		dict[@"description"] = description;
	}

	if (userID.length > 0) {
		// The label for the item in the OS X keychain.
		dict[@"keychainLabel"] = [NSString stringWithFormat:@"%@ <%@> (%@)", name, email, keyID];
	}

	NSString *iconPath = [userData stringBetweenString:@"ICON=" andString:@"," needEnd:NO];
	if (iconPath.length > 0) {
		NSImage *icon = [[NSImage alloc] initWithContentsOfFile:iconPath];
		if (icon) {
			dict[@"icon"] = icon;
		}
	}

	return dict;
}

static int mac_cmd_handler (pinentry_t pe) {
	@autoreleasepool {
		int returnValue = -1;


		// cacheId is used to save the passphrase in the macOS keychain.
		NSString *cacheId = nil;
		if (pe->keyinfo) {
			NSString *keyinfo = [NSString gpgStringWithCString:pe->keyinfo];
			if (keyinfo.length > 2) {
				// keyinfo has the form x/fingerprint. x is the cache mode it's one of u (user), s (ssh) or n (normal).
				// Ignore cache_mode at the moment.
				cacheId = [keyinfo substringFromIndex:2];
			}
		}

		if (cacheId && pe->pin && !pe->error) {
			// Search for a stored password in the macOS keychain.
			const char *passphrase = [getPassphraseFromKeychain(cacheId) UTF8String];
			if (passphrase) { // A password was found.
				int len = strlen(passphrase);
				pinentry_setbufferlen(pe, len + 1);
				if (pe->pin) {
					// Write the password into pe->pin and return its length.
					strcpy(pe->pin, passphrase);
					return len;
				} else {
					// That is bad! Could not allocate enough memory for the password.
					return -1;
				}
			}
		}


		PinentryMac *pinentry = [[PinentryMac alloc] init];

		NSDictionary *userData = parseUserData(pe);
		NSString *description = userData[@"description"];


		pinentry.grab = pe->grab;
		if (userData[@"icon"]) {
			pinentry.icon = userData[@"icon"];
		}
		if (description) {
			pinentry.descriptionText = [description stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
		}
		if (pe->ok) {
			pinentry.okText = [NSString gpgStringWithCString:pe->ok];
		}
		if (pe->cancel) {
			pinentry.cancelText = [NSString gpgStringWithCString:pe->cancel];
		}
		if (pe->title) {
			pinentry.titleText = [NSString gpgStringWithCString:pe->title];
		}
		if (pe->timeout) {
			pinentry.timeout = pe->timeout;
		}

		if (pe->pin) { // A passphrase is requested.
			if (pe->prompt) {
				pinentry.promptText = [NSString gpgStringWithCString:pe->prompt];
			}
			if (pe->error) {
				pinentry.errorText = [NSString gpgStringWithCString:pe->error];
			}
			if (cacheId) {
				pinentry.canUseKeychain = YES;
			}
			if (pe->repeat_passphrase) {
				pinentry.repeatPassword = YES;
			}
			if (pe->quality_bar) {
				pinentry.qualityCheck = ^NSInteger(NSString *password) {
					if (password.length == 0) {
						return 0;
					}
					NSUInteger quality = pinentry_inq_quality(pe, password.UTF8String, [password lengthOfBytesUsingEncoding:NSUTF8StringEncoding]);
					return quality;
				};
			}

			if ([pinentry runModal] == 1) {
				const char *passphrase = [pinentry.pin ? pinentry.pin : @"" UTF8String];
				if (passphrase) {
					int len = strlen(passphrase);
					pinentry_setbufferlen(pe, len + 1);
					if (pe->pin) {
						// Write the password into pe->pin and return its length at the end of this method.
						strcpy(pe->pin, passphrase);

						if (pe->repeat_passphrase) {
							pe->repeat_okay = YES;
						}

						returnValue = len;
					}
				}
			}

			if (cacheId) { // Having a cacheId means, we can use the keychain.
				NSString *keychainLabel = userData[@"keychainLabel"];
				if (pinentry.saveInKeychain) {
					// The user wants the password to be stored, do so.
					storePassphraseInKeychain(cacheId, pinentry.pin, keychainLabel);
				} else if (pe->error) {
					// The last password return from pinentry was wrong.
					// Remove a possible stored wrong password from the keychain.
					storePassphraseInKeychain(cacheId, nil, keychainLabel);
				}
			}

		} else {
			pinentry.confirmMode = YES;
			pinentry.oneButton = pe->one_button;
			if (pe->notok) {
				pinentry.notokText = [NSString gpgStringWithCString:pe->notok];
			}

			switch ([pinentry runModal]) {
				case 1:
					returnValue = 1;
					break;
				case 2:
					returnValue = 0;
					break;
				default:
					pe->canceled = 1;
					returnValue = 0;
					break;
			}
		}


		return returnValue;
	}
}


@end
