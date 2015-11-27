/* NSStringExtensions.m - Extensions for NSString
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

#import "NSStringExtensions.h"

@implementation NSString (BetweenExtension)
- (NSString *)stringBetweenString:(NSString *)start andString:(NSString *)end needEnd:(BOOL)endNeeded {
	NSRange range = [self rangeOfString:start];
	NSUInteger location;

	if (range.location == NSNotFound) {
		return nil;
	}

	location = range.location + range.length;
	range = [self rangeOfString:end options:NSCaseInsensitiveSearch range:NSMakeRange(location, self.length - location)];

	if (range.location != NSNotFound) {
		range.length = range.location - location;
	} else {
		if (endNeeded) {
			return nil;
		}
		range.length = self.length - location;
	}
	range.location = location;

	return [self substringWithRange:range];
}
@end

@implementation NSString (gpgString)
+ (NSString *)gpgStringWithCString:(const char *)cString {
	if (!cString) {
		return @"";
	}

	unsigned long length = strlen(cString);

	if (length == 0) {
		return @"";
	}

	NSString *retString = [NSString stringWithUTF8String:cString];
	if (retString) {
		return retString;
	}

	// We don't have a valid UTF-8 encoded string.
	// Remove all invalid bytes, so the remaining string is valid UTF-8
	const uint8_t *inText = (uint8_t *)cString;

	NSUInteger i = 0;
	uint8_t *outText = malloc(length + 1);

	if (outText) {
		uint8_t *outPos = outText;
		const uint8_t *startChar = nil;
		int multiByte = 0;

		for (; i < length; i++) {
			if (multiByte && (*inText & 0xC0) == 0x80) { // Continuation of a multi-byte character.
				multiByte--;
				if (multiByte == 0) {
					while (startChar <= inText) {
						*(outPos++) = *(startChar++);
					}
				}
			} else if ((*inText & 0x80) == 0) { // Standard ASCII character.
				*(outPos++) = *inText;
				multiByte = 0;
			} else if ((*inText & 0xC0) == 0xC0) { // Start of a multi-byte character.
				if (multiByte) {
					*(outPos++) = '?';
				}
				if (*inText <= 0xDF && *inText >= 0xC2) {
					multiByte = 1;
					startChar = inText;
				} else if (*inText <= 0xEF && *inText >= 0xE0) {
					multiByte = 2;
					startChar = inText;
				} else if (*inText <= 0xF4 && *inText >= 0xF0) {
					multiByte = 3;
					startChar = inText;
				} else {
					*(outPos++) = '?';
					multiByte = 0;
				}
			} else {
				*(outPos++) = '?';
			}

			inText++;
		}
		*outPos = 0;


		retString = [NSString stringWithUTF8String:(char *)outText];

		free(outText);
		if (retString) {
			return retString;
		}
	}
	// End of the clean-up.



	int encodings[3] = {NSISOLatin1StringEncoding, NSISOLatin2StringEncoding, NSASCIIStringEncoding};
	for(i = 0; i < 3; i++) {
		retString = [NSString stringWithCString:cString encoding:encodings[i]];
		if (retString.length > 0) {
			return retString;
		}
	}

	return retString;
}
@end
