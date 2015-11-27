/* ShortcutHandlingFields.m - Textfields with shortcut support.

 Copyright © Moritz Ulrich, 2011
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


#import "ShortcutHandlingFields.h"

@implementation ShortcutHandlingTextField

- (BOOL)performKeyEquivalent:(NSEvent *)event {
    if (([event modifierFlags] & NSDeviceIndependentModifierFlagsMask) == NSCommandKeyMask) {
        // The command key is the ONLY modifier key being pressed.

		NSString *eventChars = event.charactersIgnoringModifiers;
		if ([eventChars isEqualToString:@"x"]) {
            return [NSApp sendAction:@selector(cut:) to:[self.window firstResponder] from:self];
        } else if ([eventChars isEqualToString:@"c"]) {
            return [NSApp sendAction:@selector(copy:) to:[self.window firstResponder] from:self];
        } else if ([eventChars isEqualToString:@"v"]) {
            return [NSApp sendAction:@selector(paste:) to:[self.window firstResponder] from:self];
        } else if ([eventChars isEqualToString:@"a"]) {
            return [NSApp sendAction:@selector(selectAll:) to:[self.window firstResponder] from:self];
        }
    }
    return [super performKeyEquivalent:event];
}

@end

@implementation ShortcutHandlingSecureTextField

- (BOOL)performKeyEquivalent:(NSEvent *)event {
    if ((event.modifierFlags & NSDeviceIndependentModifierFlagsMask) == NSCommandKeyMask) {
		// The command key is the ONLY modifier key being pressed.

		NSString *eventChars = event.charactersIgnoringModifiers;
		if ([eventChars isEqualToString:@"v"]) {
            return [NSApp sendAction:@selector(paste:) to:[self.window firstResponder] from:self];
        } else if ([eventChars isEqualToString:@"a"]) {
            return [NSApp sendAction:@selector(selectAll:) to:[self.window firstResponder] from:self];
        }
    }
    return [super performKeyEquivalent:event];
}

@end
