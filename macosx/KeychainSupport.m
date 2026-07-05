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
#import <LocalAuthentication/LocalAuthentication.h>
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

	// Biometry-protected items always live in the default data-protection
	// keychain (see the comment below on kSecUseKeychain), which isn't
	// necessarily the same address space kSecUseKeychain searches - so a
	// query scoped to keychainRef alone won't reliably find an item from a
	// previous save, and a ref obtained from this unscoped query isn't
	// compatible with the legacy SecKeychainItemDelete (it fails with
	// errSecInvalidItemRef) - use SecItemDelete directly instead.
	NSDictionary *defaultScopeQueryDict = @{(NSString *)kSecClass: (NSString *)kSecClassGenericPassword,
									   (NSString *)kSecAttrService: @GPG_SERVICE_NAME,
									   (NSString *)kSecAttrAccount: fingerprint};
	CFDictionaryRef defaultScopeQuery = (__bridge CFDictionaryRef)defaultScopeQueryDict;

	if (encodedPassphrase) {
		CFErrorRef accessControlError = NULL;
		SecAccessControlRef accessControl = SecAccessControlCreateWithFlags(
			kCFAllocatorDefault,
			kSecAttrAccessibleWhenUnlockedThisDeviceOnly,
			kSecAccessControlBiometryCurrentSet | kSecAccessControlOr | kSecAccessControlDevicePasscode,
			&accessControlError);

		if (!accessControl) {
			if (accessControlError) {
				CFRelease(accessControlError);
			}
			CFRelease(keychainRef);
			return NO;
		}

		// kSecUseKeychain (a specific file-based keychain) cannot be combined
		// with kSecAttrAccessControl - Security framework rejects it with
		// errSecParam. Biometry-protected items always go to the default
		// data-protection keychain instead.
		NSDictionary *attributesDict = @{(NSString *)kSecClass: (NSString *)kSecClassGenericPassword,
										 (NSString *)kSecAttrService: @GPG_SERVICE_NAME,
										 (NSString *)kSecAttrAccount: fingerprint,
										 (NSString *)kSecValueData: encodedPassphrase,
										 (NSString *)kSecAttrLabel: label,
										 (NSString *)kSecAttrAccessControl: (__bridge id)accessControl};
		CFDictionaryRef attributes = (__bridge CFDictionaryRef)attributesDict;

		// Try adding first, optimistically - a brand new item (the common
		// case) needs no prior authentication at all. Biometry-protected
		// items can't be found by a kSecUseKeychain-scoped query (see
		// defaultScopeQuery above), so only fall back to finding and
		// deleting an existing item - which does require the user to
		// authenticate, since it's touching an existing protected item -
		// when SecItemAdd tells us one is actually in the way. This avoids
		// costing a Touch ID prompt on every save when there's nothing to
		// replace.
		status = SecItemAdd(attributes, nil);
		if (status == errSecDuplicateItem) {
			status = SecItemCopyMatching(query, (CFTypeRef *)&itemRef);
			if (status == errSecSuccess) {
				SecKeychainItemDelete(itemRef);
				CFRelease(itemRef);
				itemRef = nil;
			}
			SecItemDelete(defaultScopeQuery);
			status = SecItemAdd(attributes, nil);
		}

		CFRelease(accessControl);
	} else {
		status = SecItemCopyMatching(query, (CFTypeRef *)&itemRef);
		if (status == errSecSuccess) {
			status = SecKeychainItemDelete(itemRef);
			CFRelease(itemRef);
		}
		OSStatus defaultScopeStatus = SecItemDelete(defaultScopeQuery);
		if (status != errSecSuccess) {
			status = defaultScopeStatus;
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

	// A fresh LAContext per retrieval forces macOS to evaluate biometry again
	// for this fetch, rather than honoring any previously cached "always
	// allow" grant on the keychain item.
	//
	// This has to be exactly one SecItemCopyMatching call: a biometry-
	// protected item challenges on every call that touches it, so a second
	// call (e.g. a separate existence check) means a second prompt. We also
	// tried pre-authenticating via -evaluateAccessControl: with
	// kSecUseAuthenticationUISkip on the follow-up call, since some non-Apple
	// sources describe that as the way to chain calls under one prompt and
	// customize the message - empirically, on this macOS version, it did not
	// suppress the second challenge and regressed to two prompts, so we're
	// intentionally not doing that. This single-call form is the one that's
	// been verified end-to-end to produce exactly one Touch ID prompt.
	LAContext *authContext = [[LAContext alloc] init];

	// Biometry-protected items always live in the default data-protection
	// keychain regardless of a custom KeychainPath (see storePassphraseInKeychain),
	// so look there first. This also avoids passing a possibly-nil keychainRef
	// into a dictionaryWithObjectsAndKeys: varargs list, which would treat
	// the nil as the list terminator and silently drop kSecUseAuthenticationContext
	// (and everything after it) whenever no custom KeychainPath is set.
	NSMutableDictionary *attributes = [@{(NSString *)kSecClass: (NSString *)kSecClassGenericPassword,
								(NSString *)kSecAttrService: @GPG_SERVICE_NAME,
								(NSString *)kSecAttrAccount: fingerprint,
								(NSString *)kSecReturnData: @YES,
								(NSString *)kSecUseAuthenticationContext: authContext} mutableCopy];
	CFTypeRef passphraseData = nil;

	int status = SecItemCopyMatching((__bridge CFDictionaryRef)attributes, &passphraseData);

	if (status == errSecItemNotFound && keychainRef) {
		// Fall back to the explicit custom-KeychainPath keychain, for items
		// stored there by older versions before biometry protection existed.
		// A plain (non-ACL) item found here won't challenge for biometry, so
		// this doesn't risk a second Touch ID prompt for the same item.
		attributes[(NSString *)kSecUseKeychain] = (__bridge id)keychainRef;
		status = SecItemCopyMatching((__bridge CFDictionaryRef)attributes, &passphraseData);
	}

	if (status == errSecAuthFailed) {
		// The keychain is unusable because of the Apple bug radar://50789571
		// Do not try to use the keychain in any form.
		if (keychainUnusable) {
			*keychainUnusable = YES;
		}
	} else if (status == errSecUserCanceled) {
		// The user did not authenticate. Do not use the keychain, do
		// prevent removing or overwriting of the correct passphrase.
		if (keychainUnusable) {
			*keychainUnusable = YES;
		}
	}

	if (keychainRef) {
		CFRelease(keychainRef);
	}
	if (status != errSecSuccess) {
		return nil;
	}

	NSString *passphrase = [[NSString alloc] initWithData:(__bridge NSData *)passphraseData encoding:NSUTF8StringEncoding];

	CFRelease(passphraseData);

	return passphrase;
}
