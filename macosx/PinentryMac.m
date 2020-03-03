/* PinentryMac.m - pinentry dialog for Mac OS X
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

#import "PinentryMac.h"

#define localized(x) [[NSBundle mainBundle] localizedStringForKey:x value:nil table:nil]


@interface PinentryMac ()
@property (nonatomic, strong) IBOutlet NSWindow *window;
@property (nonatomic, strong) IBOutlet NSButton *okButton;
@property (nonatomic, strong) IBOutlet NSButton *cancelButton;
@property (nonatomic, strong) IBOutlet NSButton *notokButton;
@property (nonatomic, strong) IBOutlet NSButton *showTypingButton;
@property (nonatomic, strong) IBOutlet NSButton *saveInKeychainButton;
@property (nonatomic, strong) IBOutlet NSTextField *passphraseField;
@property (nonatomic, strong) IBOutlet NSSecureTextField *securePassphraseField;
@property (nonatomic, strong) IBOutlet NSTextField *repeatPassphraseField;
@property (nonatomic, strong) IBOutlet NSSecureTextField *secureRepeatPassphraseField;
@property (nonatomic, strong) IBOutlet NSTextField *errorLabel;
@property (nonatomic, strong) IBOutlet NSLevelIndicator *qualityBar;

@property (nonatomic, readwrite, strong) NSString *pin;
@property (nonatomic, strong) NSString *repeatedPin;
@property (nonatomic, strong) NSString *showTypingText;
@property (nonatomic, strong) NSString *saveInKeychainText;
@property (nonatomic, strong) NSString *repeatText;

@property (nonatomic, readonly) BOOL okEnabled;
@property (nonatomic) NSInteger quality;
@property (nonatomic) NSInteger minQuality;
@property (nonatomic) NSInteger warnQuality;


- (IBAction)buttonClicked:(NSButton *)sender;
@end


@implementation PinentryMac
@synthesize titleText, descriptionText, errorText, promptText, okText, notokText, cancelText, showTypingText, saveInKeychainText, repeatText;
@synthesize grab, oneButton, confirmMode, showTyping;
@synthesize okButton, cancelButton, notokButton, showTypingButton, saveInKeychainButton;
@synthesize icon, timeout, window;
@synthesize passphraseField, securePassphraseField, errorLabel;


PinentryMac *_sharedInstance = nil;

- (id)init {
	self = [super init];
	if (!self) {
		return nil;
	}

	self.showTyping = [[NSUserDefaults standardUserDefaults] boolForKey:@"ShowPassphrase"];
	self.saveInKeychain = [[NSUserDefaults standardUserDefaults] boolForKey:@"UseKeychain"];
	_disableKeychain = [[NSUserDefaults standardUserDefaults] boolForKey:@"DisableKeychain"];


	self.okText = localized(@"OK");
	self.cancelText = localized(@"Cancel");
	self.showTypingText = localized(@"Show typing");
	self.saveInKeychainText = localized(@"Save in Keychain");
	self.repeatText = localized(@"Repeat:");
	self.titleText = @"Pinentry Mac";

	if (NSAppKitVersionNumber < NSAppKitVersionNumber10_8) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
		[NSBundle loadNibNamed:@"Pinentry" owner:self];
#pragma GCC diagnostic pop
	} else {
		NSArray *objects = nil;
		[[NSBundle mainBundle] loadNibNamed:@"Pinentry" owner:self topLevelObjects:&objects];
		topLevelObjects = objects;
	}

	return self;
}

- (NSInteger)runModal {
	if (!self.errorText) {
		[self.errorLabel removeFromSuperviewWithoutNeedingDisplay];
	}
	if (self.confirmMode) {
		[self.passphraseField removeFromSuperviewWithoutNeedingDisplay];
		[self.securePassphraseField removeFromSuperviewWithoutNeedingDisplay];
		[self.showTypingButton removeFromSuperviewWithoutNeedingDisplay];
		[self.saveInKeychainButton removeFromSuperviewWithoutNeedingDisplay];
	}
	if (!self.repeatPassword) {
		[self.repeatPassphraseField removeFromSuperviewWithoutNeedingDisplay];
		[self.secureRepeatPassphraseField removeFromSuperviewWithoutNeedingDisplay];
	}
	if (self.oneButton) {
		[self.notokButton removeFromSuperviewWithoutNeedingDisplay];
		[self.cancelButton removeFromSuperviewWithoutNeedingDisplay];
	} else if (!self.notokText) {
		[self.notokButton removeFromSuperviewWithoutNeedingDisplay];
	}
	if (self.qualityCheck) {
		self.minQuality = 3;
		self.warnQuality = 5;
	} else {
		[self.qualityBar removeFromSuperviewWithoutNeedingDisplay];
	}



	[self.window center];

	[NSApp activateIgnoringOtherApps:YES];
	[self.window makeKeyAndOrderFront:self];

	if (!self.confirmMode) {
		[self.window makeFirstResponder:self.securePassphraseField];
	}

	if (self.timeout > 0) {
		timer = [NSTimer timerWithTimeInterval:self.timeout target:self.window selector:@selector(close) userInfo:nil repeats:NO];
		[[NSRunLoop currentRunLoop] addTimer:timer forMode:NSModalPanelRunLoopMode];
	}

	[NSApp runModalForWindow:self.window];

	return pressedButton;
}


- (void)setPin:(NSString *)pin {
	if (pin != _pin) {
		_pin = pin;
		if (self.qualityCheck) {
			NSInteger value = self.qualityCheck(_pin) / 10;
			if (value < 0) {
				value = -value;
				self.minQuality = MAX(self.minQuality, value);
				self.warnQuality = MAX(self.warnQuality, self.minQuality + 1);
			}
			self.quality = value;
		}
	}
}
- (NSString *)pin {
	return _pin;
}

+ (NSSet *)keyPathsForValuesAffectingOkEnabled {
	return [NSSet setWithObjects:@"pin", @"repeatedPin", nil];
}
- (BOOL)okEnabled {
	if (!self.repeatPassword) {
		return YES;
	}
	if (self.pin.length == 0 && self.repeatedPin.length == 0) {
		return YES;
	}
	return [self.pin isEqualToString:self.repeatedPin];
}



- (void)setValue:(id)value forKey:(NSString *)key {
	if (timer) {
		[timer invalidate];
		timer = nil;
	}
	[super setValue:value forKey:key];
}
- (void)buttonClicked:(NSButton *)sender {
	pressedButton = sender.tag;
	[self.window close];
}

/*
 * The user can only select to save the password in the keychain when
 * gpg-agent sent a chaching-id AND the use of the keychain was NOT disabled.
 */
- (BOOL)canUseKeychain {
	return _canUseKeychain && !_disableKeychain;
}
- (BOOL)saveInKeychain {
	return _saveInKeychain && !_disableKeychain;
}


// Window Controller
- (void)windowWillClose:(NSNotification *)notification {
	[NSApp stopModal];
}
- (void)setWindow:(NSWindow *)newWindow {
	window = newWindow;
	window.level = 30;
}
- (NSWindow *)window {
	return window;
}

@end
