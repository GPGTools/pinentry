/* PinentryMac.h
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

#import <Cocoa/Cocoa.h>


@interface PinentryMac : NSObject <NSWindowDelegate> {
	NSImage *icon;
	NSInteger pressedButton;
	NSTimer *timer;
	NSArray *topLevelObjects;
}

@property (nonatomic, strong) NSString *titleText;
@property (nonatomic, strong) NSString *descriptionText;
@property (nonatomic, strong) NSString *errorText;
@property (nonatomic, strong) NSString *promptText;
@property (nonatomic, strong) NSString *okText;
@property (nonatomic, strong) NSString *notokText;
@property (nonatomic, strong) NSString *cancelText;

@property (nonatomic) BOOL grab;
@property (nonatomic) BOOL oneButton;
@property (nonatomic) BOOL confirmMode;
@property (nonatomic) BOOL saveInKeychain;
@property (nonatomic) BOOL canUseKeychain;
@property (nonatomic) BOOL showTyping;

@property (nonatomic) NSInteger timeout;
@property (nonatomic, strong) NSImage *icon;


@property (nonatomic, readonly) NSString *pin;

- (NSInteger)runModal;


@end
