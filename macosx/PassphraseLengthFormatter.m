/* PassphraseLengthFormatter.m - Formatter to limit the number of chars.
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

#import "PassphraseLengthFormatter.h"

#define MAX_PASSPHRASE_LENGTH 300

@implementation PassphraseLengthFormatter

- (NSString *)stringForObjectValue:(id)object {
	if (![object isKindOfClass:[NSString class]]) {
        return nil;
	}
    return [NSString stringWithString:object];
}

- (BOOL)getObjectValue:(id *)object forString:(NSString *)string errorDescription:(NSString **)error {
	// Generate a new string here, otherwise bindings won't work properly.
    *object = [NSString stringWithString:string];
	return YES;
}

- (BOOL)isPartialStringValid:(NSString **)partialStringPtr
	   proposedSelectedRange:(NSRangePointer)proposedSelRangePtr
			  originalString:(NSString *)origString
	   originalSelectedRange:(NSRange)origSelRange
			errorDescription:(NSString **)error {
    // Code found on http://stackoverflow.com/a/19635242 which seems to work properly and as expected.

    NSString *proposedString = *partialStringPtr;
	if (proposedString.length <= MAX_PASSPHRASE_LENGTH) {
        return YES;
	}

    *partialStringPtr = [NSString stringWithString:[proposedString substringToIndex:MAX_PASSPHRASE_LENGTH]];
    return NO;
}

@end
