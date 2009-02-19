/* 
   secstring.h - typedefs and conversion for secmem-backed std::strings.

   Copyright (C) 2008 Klarälvdalens Datakonsult AB (KDAB)

   Written by Marc Mutz <marc@kdab.com>.
   
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.
 
   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
 
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "secstring.h"

// the following code has been ported from QByteArray to secstring.
// The licence of the original code:

/****************************************************************************
**
** Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License versions 2.0 or 3.0 as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file.  Please review the following information
** to ensure GNU General Public Licensing requirements will be met:
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.  In addition, as a special
** exception, Nokia gives you certain additional rights. These rights
** are described in the Nokia Qt GPL Exception version 1.3, included in
** the file GPL_EXCEPTION.txt in this package.
**
** Qt for Windows(R) Licensees
** As a special exception, Nokia, as the sole copyright holder for Qt
** Designer, grants users of the Qt/Eclipse Integration plug-in the
** right for the Qt/Eclipse Integration to link to functionality
** provided by Qt Designer and its related libraries.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
****************************************************************************/

secstring toUtf8( const secqstring & str )
{
    secstring ba;
    if (const int l = str.size()) {
        int rlen = l*3+1;
        ba.reserve(rlen);
        uchar *cursor = (uchar*)ba.data();
        const QChar *ch = str.data();
        for (int i=0; i < l; i++) {
            uint u = ch->unicode();
            if (u < 0x80) {
                ba.push_back( (uchar)u );
            } else {
                if (u < 0x0800) {
                    ba.push_back( 0xc0 | ((uchar) (u >> 6)) );
                } else {
                    if (ch->isHighSurrogate() && i < l-1) {
                        ushort low = ch[1].unicode();
                        if (ch[1].isLowSurrogate()) {
                            ++ch;
                            ++i;
                            u = QChar::surrogateToUcs4(u,low);
                        }
                    }
                    if (u > 0xffff) {
                        ba.push_back( 0xf0 | ((uchar) (u >> 18)) );
                        ba.push_back( 0x80 | (((uchar) (u >> 12)) & 0x3f) );
                    } else {
                        ba.push_back( 0xe0 | ((uchar) (u >> 12)) );
                    }
                    ba.push_back( 0x80 | (((uchar) (u >> 6)) & 0x3f) );
                }
                ba.push_back( 0x80 | ((uchar) (u&0x3f)) );
            }
            ++ch;
        }
    }
    return ba;
}

