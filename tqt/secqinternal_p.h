/****************************************************************************
** $Id$
**
** Definition of some shared interal classes
**
** Created : 010427
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the kernel module of the TQt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.TQPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid TQt Enterprise Edition or TQt Professional Edition
** licenses may use this file in accordance with the TQt Commercial License
** Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about TQt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for TQPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
** SPDX-License-Identifier: GPL-2.0 OR QPL-1.0
**
**********************************************************************/

#ifndef SECTQINTERNAL_P_H
#define SECTQINTERNAL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the TQt API.  It exists for the convenience
// of a number of TQt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//
//
#ifndef QT_H
#include "ntqnamespace.h"
#include "ntqrect.h"
#include "ntqptrlist.h"
#include "ntqcstring.h"
#include "ntqiodevice.h"
#endif // QT_H

class TQWidget;
class TQPainter;
class TQPixmap;

class Q_EXPORT SecTQSharedDoubleBuffer
{
public:
    enum DoubleBufferFlags {
	NoFlags         = 0x00,
	InitBG		= 0x01,
	Force		= 0x02,
	Default		= InitBG | Force
    };
    typedef uint DBFlags;

    SecTQSharedDoubleBuffer( DBFlags f = Default );
    SecTQSharedDoubleBuffer( TQWidget* widget,
			 int x = 0, int y = 0, int w = -1, int h = -1,
			 DBFlags f = Default );
    SecTQSharedDoubleBuffer( TQPainter* painter,
			 int x = 0, int y = 0, int w = -1, int h = -1,
			 DBFlags f = Default );
    SecTQSharedDoubleBuffer( TQWidget *widget, const TQRect &r, DBFlags f = Default );
    SecTQSharedDoubleBuffer( TQPainter *painter, const TQRect &r, DBFlags f = Default );
    ~SecTQSharedDoubleBuffer();

    bool begin( TQWidget* widget, int x = 0, int y = 0, int w = -1, int h = -1 );
    bool begin( TQPainter* painter, int x = 0, int y = 0, int w = -1, int h = -1);
    bool begin( TQWidget* widget, const TQRect &r );
    bool begin( TQPainter* painter, const TQRect &r );
    bool end();

    TQPainter* painter() const;

    bool isActive() const;
    bool isBuffered() const;
    void flush();

    static bool isDisabled() { return !dblbufr; }
    static void setDisabled( bool off ) { dblbufr = !off; }

    static void cleanup();

private:
    enum DoubleBufferState {
	Active		= 0x0100,
	BufferActive	= 0x0200,
	ExternalPainter	= 0x0400
    };
    typedef uint DBState;

    TQPixmap *getPixmap();
    void releasePixmap();

    TQWidget *wid;
    int rx, ry, rw, rh;
    DBFlags flags;
    DBState state;

    TQPainter *p, *external_p;
    TQPixmap *pix;

    static bool dblbufr;
};

inline bool SecTQSharedDoubleBuffer::begin( TQWidget* widget, const TQRect &r )
{ return begin( widget, r.x(), r.y(), r.width(), r.height() ); }

inline bool SecTQSharedDoubleBuffer::begin( TQPainter *painter, const TQRect &r )
{ return begin( painter, r.x(), r.y(), r.width(), r.height() ); }

inline TQPainter* SecTQSharedDoubleBuffer::painter() const
{ return p; }

inline bool SecTQSharedDoubleBuffer::isActive() const
{ return ( state & Active ); }

inline bool SecTQSharedDoubleBuffer::isBuffered() const
{ return ( state & BufferActive ); }

#endif // SECTQINTERNAL_P_H
