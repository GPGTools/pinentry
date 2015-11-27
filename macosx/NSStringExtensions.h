/* NSStringExtensions.h
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

@interface NSString (BetweenExtension)
- (NSString *)stringBetweenString:(NSString *)start andString:(NSString *)end needEnd:(BOOL)endNeeded;
@end

@interface NSString (gpgString)
+ (NSString *)gpgStringWithCString:(const char *)cString;
@end
