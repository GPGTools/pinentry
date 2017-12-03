/****************************************************************************
** $Id$
**
** Implementation of some internal classes
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
** Foundation and appearing in the file COPYING included in the
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

#include "secqinternal_p.h"
#include "ntqwidget.h"
#include "ntqpixmap.h"
#include "ntqpainter.h"
#include "ntqcleanuphandler.h"

static TQPixmap* qdb_shared_pixmap = 0;
static TQPixmap *qdb_force_pixmap = 0;
static SecTQSharedDoubleBuffer* qdb_owner = 0;

static TQCleanupHandler<TQPixmap> qdb_pixmap_cleanup;

#ifdef Q_WS_MACX
bool SecTQSharedDoubleBuffer::dblbufr = FALSE;
#else
bool SecTQSharedDoubleBuffer::dblbufr = TRUE;
#endif


/*
  hardLimitWidth/Height: if >= 0, the maximum number of pixels that
  get double buffered.

  sharedLimitWidth/Height: if >= 0, the maximum number of pixels the
  shared double buffer can keep.

  For x with sharedLimitSize < x <= hardLimitSize, temporary buffers
  are constructed.
 */
static const int hardLimitWidth = -1;
static const int hardLimitHeight = -1;
#if defined( Q_WS_QWS ) || defined( Q_WS_MAC9 )
// Small in TQt/Embedded / Mac9 - 5K on 32bpp
static const int sharedLimitWidth = 64;
static const int sharedLimitHeight = 20;
#else
// 240K on 32bpp
static const int sharedLimitWidth = 640;
static const int sharedLimitHeight = 100;
#endif

// *******************************************************************
// SecTQSharedDoubleBufferCleaner declaration and implementation
// *******************************************************************

/* \internal
   This class is responsible for cleaning up the pixmaps created by the
   SecTQSharedDoubleBuffer class.  When SecTQSharedDoubleBuffer creates a
   pixmap larger than the shared limits, this class deletes it after a
   specified amount of time.

   When the large pixmap is created/used, you must call start(). If the
   large pixmap is ever deleted, you must call stop().  The start()
   method always restarts the timer, so if the large pixmap is
   constantly in use, the timer will never fire, and the pixmap will
   not be constantly created and destroyed.
*/

static const int shared_double_buffer_cleanup_timeout = 30000; // 30 seconds

// declaration

class SecTQSharedDoubleBufferCleaner : public TQObject
{
public:
    SecTQSharedDoubleBufferCleaner( void );

    void start( void );
    void stop( void );

    void doCleanup( void );

    bool event( TQEvent *e );

private:
    int timer_id;
};

// implementation

/* \internal
   Creates a SecTQSharedDoubleBufferCleaner object. The timer is not
   started when creating the object.
*/
SecTQSharedDoubleBufferCleaner::SecTQSharedDoubleBufferCleaner( void )
    : TQObject( 0, "internal shared double buffer cleanup object" ),
      timer_id( -1 )
{
}

/* \internal
   Starts the cleanup timer.  Any previously running timer is stopped.
*/
void SecTQSharedDoubleBufferCleaner::start( void )
{
    stop();
    timer_id = startTimer( shared_double_buffer_cleanup_timeout );
}

/* \internal
   Stops the cleanup timer, if it is running.
*/
void SecTQSharedDoubleBufferCleaner::stop( void )
{
    if ( timer_id != -1 )
	killTimer( timer_id );
    timer_id = -1;
}

/* \internal
 */
void SecTQSharedDoubleBufferCleaner::doCleanup( void )
{
    qdb_pixmap_cleanup.remove( &qdb_force_pixmap );
    delete qdb_force_pixmap;
    qdb_force_pixmap = 0;
}

/* \internal
   Event handler reimplementation.  Calls doCleanup() when the timer
   fires.
*/
bool SecTQSharedDoubleBufferCleaner::event( TQEvent *e )
{
    if ( e->type() != TQEvent::Timer )
	return FALSE;

    TQTimerEvent *event = (TQTimerEvent *) e;
    if ( event->timerId() == timer_id ) {
	doCleanup();
	stop();
    }
#ifdef QT_CHECK_STATE
    else {
	tqWarning( "SecTQSharedDoubleBufferCleaner::event: invalid timer event received." );
	return FALSE;
    }
#endif // QT_CHECK_STATE

    return TRUE;
}

// static instance
static SecTQSharedDoubleBufferCleaner *static_cleaner = 0;
TQSingleCleanupHandler<SecTQSharedDoubleBufferCleaner> cleanup_static_cleaner;

inline static SecTQSharedDoubleBufferCleaner *staticCleaner()
{
    if ( ! static_cleaner ) {
	static_cleaner = new SecTQSharedDoubleBufferCleaner();
	cleanup_static_cleaner.set( &static_cleaner );
    }
    return static_cleaner;
}


// *******************************************************************
// SecTQSharedDoubleBuffer implementation
// *******************************************************************

/* \internal
   \enum DoubleBufferFlags

   \value InitBG initialize the background of the double buffer.

   \value Force disable shared buffer size limits.

   \value Default InitBG and Force are used by default.
*/

/* \internal
   \enum DoubleBufferState

   \value Active indicates that the buffer may be used.

   \value BufferActive indicates that painting with painter() will be
   double buffered.

   \value ExternalPainter indicates that painter() will return a
   painter that was not created by SecTQSharedDoubleBuffer.
*/

/* \internal
   \class SecTQSharedDoubleBuffer

   This class provides a single, reusable double buffer.  This class
   is used internally by TQt widgets that need double buffering, which
   prevents each individual widget form creating a double buffering
   pixmap.

   Using a single pixmap double buffer and sharing it across all
   widgets is nicer on window system resources.
*/

/* \internal
   Creates a SecTQSharedDoubleBuffer with flags \f.

   \sa DoubleBufferFlags
*/
SecTQSharedDoubleBuffer::SecTQSharedDoubleBuffer( DBFlags f )
    : wid( 0 ), rx( 0 ), ry( 0 ), rw( 0 ), rh( 0 ), flags( f ), state( 0 ),
      p( 0 ), external_p( 0 ), pix( 0 )
{
}

/* \internal
   Creates a SecTQSharedDoubleBuffer with flags \f. The \a widget, \a x,
   \a y, \a w and \a h arguments are passed to begin().

   \sa DoubleBufferFlags begin()
*/
SecTQSharedDoubleBuffer::SecTQSharedDoubleBuffer( TQWidget* widget,
					  int x, int y, int w, int h,
					  DBFlags f )
    : wid( 0 ), rx( 0 ), ry( 0 ), rw( 0 ), rh( 0 ), flags( f ), state( 0 ),
      p( 0 ), external_p( 0 ), pix( 0 )
{
    begin( widget, x, y, w, h );
}

/* \internal
   Creates a SecTQSharedDoubleBuffer with flags \f. The \a painter, \a x,
   \a y, \a w and \a h arguments are passed to begin().

   \sa DoubleBufferFlags begin()
*/
SecTQSharedDoubleBuffer::SecTQSharedDoubleBuffer( TQPainter* painter,
					  int x, int y, int w, int h,
					  DBFlags f)
    : wid( 0 ), rx( 0 ), ry( 0 ), rw( 0 ), rh( 0 ), flags( f ), state( 0 ),
      p( 0 ), external_p( 0 ), pix( 0 )
{
    begin( painter, x, y, w, h );
}

/* \internal
   Creates a SecTQSharedDoubleBuffer with flags \f. The \a widget and
   \a r arguments are passed to begin().

   \sa DoubleBufferFlags begin()
*/
SecTQSharedDoubleBuffer::SecTQSharedDoubleBuffer( TQWidget *widget, const TQRect &r, DBFlags f )
    : wid( 0 ), rx( 0 ), ry( 0 ), rw( 0 ), rh( 0 ), flags( f ), state( 0 ),
      p( 0 ), external_p( 0 ), pix( 0 )
{
    begin( widget, r );
}

/* \internal
   Creates a SecTQSharedDoubleBuffer with flags \f. The \a painter and
   \a r arguments are passed to begin().

   \sa DoubleBufferFlags begin()
*/
SecTQSharedDoubleBuffer::SecTQSharedDoubleBuffer( TQPainter *painter, const TQRect &r, DBFlags f )
    : wid( 0 ), rx( 0 ), ry( 0 ), rw( 0 ), rh( 0 ), flags( f ), state( 0 ),
      p( 0 ), external_p( 0 ), pix( 0 )
{
    begin( painter, r );
}

/* \internal
   Destructs the SecTQSharedDoubleBuffer and calls end() if the buffer is
   active.

   \sa isActive() end()
*/
SecTQSharedDoubleBuffer::~SecTQSharedDoubleBuffer()
{
    if ( isActive() )
        end();
}

/* \internal
   Starts double buffered painting in the area specified by \a x,
   \a y, \a w and \a h on \a painter.  Painting should be done using the
   TQPainter returned by SecTQSharedDoubleBuffer::painter().

   The double buffered area will be updated when calling end().

   \sa painter() isActive() end()
*/
bool SecTQSharedDoubleBuffer::begin( TQPainter* painter, int x, int y, int w, int h )
{
    if ( isActive() ) {
#if defined(QT_CHECK_STATE)
        tqWarning( "SecTQSharedDoubleBuffer::begin: Buffer is already active."
                  "\n\tYou must end() the buffer before a second begin()" );
#endif // QT_CHECK_STATE
        return FALSE;
    }

    external_p = painter;

    if ( painter->device()->devType() == TQInternal::Widget )
	return begin( (TQWidget *) painter->device(), x, y, w, h );

    state = Active;

    rx = x;
    ry = y;
    rw = w;
    rh = h;

    if ( ( pix = getPixmap() ) ) {
#ifdef Q_WS_X11
	if ( painter->device()->x11Screen() != pix->x11Screen() )
	    pix->x11SetScreen( painter->device()->x11Screen() );
	TQPixmap::x11SetDefaultScreen( pix->x11Screen() );
#endif // Q_WS_X11

	state |= BufferActive;
	p = new TQPainter( pix );
	if ( p->isActive() ) {
	    p->setPen( external_p->pen() );
	    p->setBackgroundColor( external_p->backgroundColor() );
	    p->setFont( external_p->font() );
	}
    } else {
	state |= ExternalPainter;
	p = external_p;
    }

    return TRUE;
}

/* \internal


   Starts double buffered painting in the area specified by \a x,
   \a y, \a w and \a h on \a widget.  Painting should be done using the
   TQPainter returned by SecTQSharedDoubleBuffer::painter().

   The double buffered area will be updated when calling end().

   \sa painter() isActive() end()
*/
bool SecTQSharedDoubleBuffer::begin( TQWidget* widget, int x, int y, int w, int h )
{
    if ( isActive() ) {
#if defined(QT_CHECK_STATE)
        tqWarning( "SecTQSharedDoubleBuffer::begin: Buffer is already active."
                  "\n\tYou must end() the buffer before a second begin()" );
#endif // QT_CHECK_STATE
        return FALSE;
    }

    state = Active;

    wid = widget;
    rx = x;
    ry = y;
    rw = w <= 0 ? wid->width() : w;
    rh = h <= 0 ? wid->height() : h;

    if ( ( pix = getPixmap() ) ) {
#ifdef Q_WS_X11
	if ( wid->x11Screen() != pix->x11Screen() )
	    pix->x11SetScreen( wid->x11Screen() );
	TQPixmap::x11SetDefaultScreen( pix->x11Screen() );
#endif // Q_WS_X11

	state |= BufferActive;
	if ( flags & InitBG ) {
	    pix->fill( wid, rx, ry );
	}
	p = new TQPainter( pix, wid );
	// newly created painters should be translated to the origin
	// of the widget, so that paint methods can draw onto the double
	// buffered painter in widget coordinates.
	p->setBrushOrigin( -rx, -ry );
	p->translate( -rx, -ry );
    } else {
	if ( external_p ) {
	    state |= ExternalPainter;
	    p = external_p;
	} else {
	    p = new TQPainter( wid );
	}

	if ( flags & InitBG ) {
	    wid->erase( rx, ry, rw, rh );
	}
    }
    return TRUE;
}

/* \internal
   Ends double buffered painting.  The contents of the shared double
   buffer pixmap are drawn onto the destination by calling flush(),
   and ownership of the shared double buffer pixmap is released.

   \sa begin() flush()
*/
bool SecTQSharedDoubleBuffer::end()
{
    if ( ! isActive() ) {
#if defined(QT_CHECK_STATE)
	tqWarning( "SecTQSharedDoubleBuffer::end: Buffer is not active."
		  "\n\tYou must call begin() before calling end()." );
#endif // QT_CHECK_STATE
	return FALSE;
    }

    if ( ! ( state & ExternalPainter ) ) {
	p->end();
	delete p;
    }

    flush();

    if ( pix ) {
	releasePixmap();
    }

    wid = 0;
    rx = ry = rw = rh = 0;
    // do not reset flags!
    state = 0;

    p = external_p = 0;
    pix = 0;

    return TRUE;
}

/* \internal
   Paints the contents of the shared double buffer pixmap onto the
   destination.  The destination is determined from the arguments
   based to begin().

   Note: You should not need to call this function, since it is called
   from end().

   \sa begin() end()
*/
void SecTQSharedDoubleBuffer::flush()
{
    if ( ! isActive() || ! ( state & BufferActive ) )
	return;

    if ( external_p )
	external_p->drawPixmap( rx, ry, *pix, 0, 0, rw, rh );
    else if ( wid && wid->isVisible() )
	bitBlt( wid, rx, ry, pix, 0, 0, rw, rh );
}

/* \internal
   Atquire ownership of the shared double buffer pixmap, subject to the
   following conditions:

   \list 1
   \i double buffering is enabled globally.
   \i the shared double buffer pixmap is not in use.
   \i the size specified in begin() is valid, and within limits.
   \endlist

   If all of these conditions are met, then this SecTQSharedDoubleBuffer
   object becomes the owner of the shared double buffer pixmap.  The
   shared double buffer pixmap is resize if necessary, and this
   function returns a pointer to the pixmap.  Ownership must later be
   relinquished by calling releasePixmap().

   If none of the above conditions are met, this function returns
   zero.

   \sa releasePixmap()
*/
TQPixmap *SecTQSharedDoubleBuffer::getPixmap()
{
    if ( isDisabled() ) {
	// double buffering disabled globally
	return 0;
    }

    if ( qdb_owner ) {
	// shared pixmap already in use
	return 0;
    }

    if ( rw <= 0 || rh <= 0 ||
	 ( hardLimitWidth > 0 && rw >= hardLimitWidth ) ||
	 ( hardLimitHeight > 0 && rh >= hardLimitHeight ) ) {
	// invalid size, or hard limit reached
	return 0;
    }

    if ( rw >= sharedLimitWidth || rh >= sharedLimitHeight ) {
	if ( flags & Force ) {
	    rw = TQMIN(rw, 8000);
	    rh = TQMIN(rh, 8000);
	    // need to create a big pixmap and start the cleaner
	    if ( ! qdb_force_pixmap ) {
		qdb_force_pixmap = new TQPixmap( rw, rh );
		qdb_pixmap_cleanup.add( &qdb_force_pixmap );
	    } else if ( qdb_force_pixmap->width () < rw ||
			qdb_force_pixmap->height() < rh ) {
		qdb_force_pixmap->resize( rw, rh );
	    }
	    qdb_owner = this;
	    staticCleaner()->start();
	    return qdb_force_pixmap;
	}

	// size is outside shared limit
	return 0;
    }

    if ( ! qdb_shared_pixmap ) {
	qdb_shared_pixmap = new TQPixmap( rw, rh );
	qdb_pixmap_cleanup.add( &qdb_shared_pixmap );
    } else if ( qdb_shared_pixmap->width() < rw ||
		qdb_shared_pixmap->height() < rh ) {
	qdb_shared_pixmap->resize( rw, rh );
    }
    qdb_owner = this;
    return qdb_shared_pixmap;
}

/* \internal
   Releases ownership of the shared double buffer pixmap.

   \sa getPixmap()
*/
void SecTQSharedDoubleBuffer::releasePixmap()
{
    if ( qdb_owner != this ) {
	// sanity check

#ifdef QT_CHECK_STATE
	tqWarning( "SecTQSharedDoubleBuffer::releasePixmap: internal error."
		  "\n\t%p does not own shared pixmap, %p does.",
		  (void*)this, (void*)qdb_owner );
#endif // QT_CHECK_STATE

	return;
    }

    qdb_owner = 0;
}

/* \internal
   \fn bool SecTQSharedDoubleBuffer::isDisabled()

   Returns TRUE is double buffering is disabled globally, FALSE otherwise.
*/

/* \internal
   \fn void SecTQSharedDoubleBuffer::setDisabled( bool off )

   Disables global double buffering \a off is TRUE, otherwise global
   double buffering is enabled.
*/

/* \internal
   Deletes the shared double buffer pixmap.  You should not need to
   call this function, since it is called from the TQApplication
   destructor.
*/
void SecTQSharedDoubleBuffer::cleanup()
{
    qdb_pixmap_cleanup.remove( &qdb_shared_pixmap );
    qdb_pixmap_cleanup.remove( &qdb_force_pixmap );
    delete qdb_shared_pixmap;
    delete qdb_force_pixmap;
    qdb_shared_pixmap = 0;
    qdb_force_pixmap = 0;
    qdb_owner = 0;
}

/* \internal
   \fn bool SecTQSharedDoubleBuffer::begin( TQWidget *widget, const TQRect &r )
   \overload
*/

/* \internal
   \fn bool SecTQSharedDoubleBuffer::begin( TQPainter *painter, const TQRect &r )
   \overload
*/

/* \internal
   \fn TQPainter *SecTQSharedDoubleBuffer::painter() const

   Returns the active painter on the double buffered area,
   or zero if double buffered painting is not active.
*/

/* \internal
   \fn bool SecTQSharedDoubleBuffer::isActive() const

   Returns TRUE if double buffered painting is active, FALSE otherwise.
*/

/* \internal
   \fn bool SecTQSharedDoubleBuffer::isBuffered() const

   Returns TRUE if painting is double buffered, FALSE otherwise.
*/
