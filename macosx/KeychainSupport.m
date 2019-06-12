/* KeychainSupport.m - Support for the Mac OS X keychain.
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

#import <Security/Security.h>
#import "KeychainSupport.h"

#define GPG_SERVICE_NAME "GnuPG"


BOOL storePassphraseInKeychain(NSString *fingerprint, NSString *passphrase, NSString *label) {
	OSStatus status;
	SecKeychainItemRef itemRef = nil;
	SecKeychainRef keychainRef = nil;

    NSString *keychainPath = [[NSUserDefaults standardUserDefaults] valueForKey:@"KeychainPath"];
    const char *path = keychainPath.UTF8String;


    if (keychainPath.length) {
        if (SecKeychainOpen(path, &keychainRef) != 0) {
            return NO;
        }
    } else if (SecKeychainCopyDefault(&keychainRef) != 0) {
        return NO;
    }

	if (!label) {
		label = @GPG_SERVICE_NAME;
	}

	NSData *encodedPassphrase = [passphrase dataUsingEncoding:NSUTF8StringEncoding];


	NSDictionary *queryDict = @{(NSString *)kSecClass: (NSString *)kSecClassGenericPassword,
									   (NSString *)kSecAttrService: @GPG_SERVICE_NAME,
									   (NSString *)kSecAttrAccount: fingerprint,
									   (NSString *)kSecReturnRef: @YES,
									   (NSString *)kSecUseKeychain: (__bridge id)keychainRef};
	CFDictionaryRef query = (__bridge CFDictionaryRef)queryDict;


	if (encodedPassphrase) {
		NSDictionary *attributesDict = @{(NSString *)kSecClass: (NSString *)kSecClassGenericPassword,
										 (NSString *)kSecAttrService: @GPG_SERVICE_NAME,
										 (NSString *)kSecAttrAccount: fingerprint,
										 (NSString *)kSecValueData: encodedPassphrase,
										 (NSString *)kSecAttrLabel: label,
										 (NSString *)kSecUseKeychain: (__bridge id)keychainRef};
		CFDictionaryRef attributes = (__bridge CFDictionaryRef)attributesDict;


		status = SecItemUpdate(query, attributes);
		if (status == errSecItemNotFound) {
			status = SecItemAdd(attributes, nil);
		}
	} else {
		status = SecItemCopyMatching(query, (CFTypeRef *)&itemRef);
		if (status == errSecSuccess) {
			status = SecKeychainItemDelete(itemRef);
			CFRelease(itemRef);
		}
	}

	CFRelease(keychainRef);

	return status == errSecSuccess;
}

NSString *getPassphraseFromKeychain(NSString *fingerprint, BOOL *keychainUnusable) {
	SecKeychainRef keychainRef = nil;

	NSString *keychainPath = [[NSUserDefaults standardUserDefaults] valueForKey:@"KeychainPath"];
	const char *path = keychainPath.UTF8String;

    if (keychainPath.length && SecKeychainOpen(path, &keychainRef) != 0) {
		return nil;
    }

	NSDictionary *attributes = [NSDictionary dictionaryWithObjectsAndKeys:
								kSecClassGenericPassword, kSecClass,
								@GPG_SERVICE_NAME, kSecAttrService,
								fingerprint, kSecAttrAccount,
								kCFBooleanFalse, kSecReturnData,
								keychainRef, kSecUseKeychain,
								nil];

	int status1 = SecItemCopyMatching((__bridge CFDictionaryRef)attributes, nil);



	attributes = [NSDictionary dictionaryWithObjectsAndKeys:
								kSecClassGenericPassword, kSecClass,
								@GPG_SERVICE_NAME, kSecAttrService,
								fingerprint, kSecAttrAccount,
								kCFBooleanTrue, kSecReturnData,
								keychainRef, kSecUseKeychain,
								nil];
	CFTypeRef passphraseData = nil;

	int status2 = SecItemCopyMatching((__bridge CFDictionaryRef)attributes, &passphraseData);

	if (status1 == errSecSuccess) {
		if (status2 == errSecAuthFailed) {
			// The keychain is unusable because of the Apple bug radar://50789571
			// Do not try to use the keychain in any form.
			if (keychainUnusable) {
				*keychainUnusable = YES;
			}
		} else if (status2 == errSecUserCanceled) {
			// The user did not allow pinentry to use the keychain.
			// Do not use the keychain, do prevent removing or overwriting of the correct passphrase.
			if (keychainUnusable) {
				*keychainUnusable = YES;
			}
		}
	}

	if (keychainRef) {
		CFRelease(keychainRef);
	}
	if (status2 != 0) {
		return nil;
	}

	NSString *passphrase = [[NSString alloc] initWithData:(__bridge NSData *)passphraseData encoding:NSUTF8StringEncoding];

	CFRelease(passphraseData);

	return passphrase;
}
