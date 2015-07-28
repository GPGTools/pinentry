/*
   pinentrydialog.cpp - A (not yet) secure Qt 4 dialog for PIN entry.

   Copyright (C) 2002, 2008 Klar‰lvdalens Datakonsult AB (KDAB)
   Copyright 2007 Ingo Kl√∂cker

   Written by Steffen Hansen <steffen@klaralvdalens-datakonsult.se>.

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

#include "pinentrydialog.h"
#include <QGridLayout>

#include <QProgressBar>
#include <QApplication>
#include <QStyle>
#include <QPainter>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QKeyEvent>
#include <QLabel>
#include <QPalette>
#include <QLineEdit>

#ifdef Q_WS_WIN
#include <windows.h>
#endif

/* I [wk] have no idea for what this code was supposed to do.
   Foregrounding a window is heavily restricted by modern Windows
   versions.  This is the reason why gpg-agent employs its
   AllowSetForegroundWindow callback machinery to ask the supposed to
   be be calling process to allow a pinentry to go into the
   foreground.

   [ah] This is a Hack to workaround the fact that Foregrounding
   a Window is so restricted that it AllowSetForegroundWindow
   does not always work (e.g. when the ForegroundWindow timeout
   has not expired.
   */
#ifdef Q_WS_WIN
WINBOOL SetForegroundWindowEx( HWND hWnd )
{
   //Attach foreground window thread to our thread
   const DWORD ForeGroundID = GetWindowThreadProcessId(::GetForegroundWindow(),NULL);
   const DWORD CurrentID   = GetCurrentThreadId();
   WINBOOL retval;

   AttachThreadInput ( ForeGroundID, CurrentID, TRUE );
   //Do our stuff here
   HWND hLastActivePopupWnd = GetLastActivePopup( hWnd );
   retval = SetForegroundWindow( hLastActivePopupWnd );

   //Detach the attached thread
   AttachThreadInput ( ForeGroundID, CurrentID, FALSE );
   return retval;
}// End SetForegroundWindowEx
#endif

void raiseWindow( QWidget* w )
{
    /* Maybe Qt will become agressive enough one day that
     * this is enough on windows too*/
    w->raise();
#ifdef Q_WS_WIN
    /* In the meantime we do our own attention grabbing */
    if (!SetForegroundWindow (w->winId()) &&
            !SetForegroundWindowEx (w->winId()))  {
        OutputDebugString("SetForegroundWindow (ex) failed");
        /* Yet another fallback which will not work on some
         * versions and is not recommended by msdn */
        if (!ShowWindow (w->winId(), SW_SHOWNORMAL)) {
            OutputDebugString ("ShowWindow failed.");
        }
    }
#endif
}

QPixmap icon( QStyle::StandardPixmap which )
{
    QPixmap pm = qApp->windowIcon().pixmap( 48, 48 );

    if ( which != QStyle::SP_CustomBase ) {
        const QIcon ic = qApp->style()->standardIcon( which );
        QPainter painter( &pm );
        const int emblemSize = 22;
        painter.drawPixmap( pm.width()-emblemSize, 0,
                            ic.pixmap( emblemSize, emblemSize ) );
    }

    return pm;
}

void PinEntryDialog::slotTimeout()
{
    reject();
}

PinEntryDialog::PinEntryDialog( QWidget* parent, const char* name,
	int timeout, bool modal, bool enable_quality_bar )
  : QDialog( parent, Qt::WindowStaysOnTopHint ), _grabbed( false )
{
  setWindowFlags( windowFlags() & ~Qt::WindowContextHelpButtonHint );

  if ( modal ) {
    setWindowModality( Qt::ApplicationModal );
  }

  _icon = new QLabel( this );
  _icon->setPixmap( icon() );

  _error = new QLabel( this );
  _error->setWordWrap(true);
  QPalette pal;
  pal.setColor( QPalette::WindowText, Qt::red );
  _error->setPalette( pal );
  _error->hide();

  _desc = new QLabel( this );
  _desc->setWordWrap(true);
  _desc->hide();

  _prompt = new QLabel( this );
  _prompt->hide();

  _edit = new QLineEdit( this );
  _edit->setMaxLength( 256 );
  _edit->setEchoMode( QLineEdit::Password );

  _prompt->setBuddy( _edit );

  if (enable_quality_bar)
  {
    _quality_bar_label = new QLabel( this );
    _quality_bar_label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
    _quality_bar = new QProgressBar( this );
    _quality_bar->setAlignment( Qt::AlignCenter );
    _have_quality_bar = true;
  }
  else
    _have_quality_bar = false;

  QDialogButtonBox* const buttons = new QDialogButtonBox( this );
  buttons->setStandardButtons( QDialogButtonBox::Ok | QDialogButtonBox::Cancel );
  _ok = buttons->button( QDialogButtonBox::Ok );
  _cancel = buttons->button( QDialogButtonBox::Cancel );

  _ok->setDefault(true);

  if ( style()->styleHint( QStyle::SH_DialogButtonBox_ButtonsHaveIcons ) )
    {
      _ok->setIcon( style()->standardIcon( QStyle::SP_DialogOkButton ) );
      _cancel->setIcon( style()->standardIcon( QStyle::SP_DialogCancelButton ) );
    }

  if (timeout > 0) {
      _timer = new QTimer(this);
      connect(_timer, SIGNAL(timeout()), this, SLOT(slotTimeout()));
      _timer->start(timeout*1000);
  }
  else
    _timer = NULL;

  connect( buttons, SIGNAL(accepted()), this, SLOT(accept()) );
  connect( buttons, SIGNAL(rejected()), this, SLOT(reject()) );
  connect( _edit, SIGNAL( textChanged(QString) ),
	   this, SLOT( updateQuality(QString) ) );

  _edit->setFocus();

  QGridLayout* const grid = new QGridLayout( this );
  grid->addWidget( _icon, 0, 0, 5, 1, Qt::AlignTop|Qt::AlignLeft );
  grid->addWidget( _error, 1, 1, 1, 2 );
  grid->addWidget( _desc,  2, 1, 1, 2 );
  //grid->addItem( new QSpacerItem( 0, _edit->height() / 10, QSizePolicy::Minimum, QSizePolicy::Fixed ), 1, 1 );
  grid->addWidget( _prompt, 3, 1 );
  grid->addWidget( _edit, 3, 2 );
  if( enable_quality_bar )
  {
    grid->addWidget( _quality_bar_label, 4, 1 );
    grid->addWidget( _quality_bar, 4, 2 );
  }
  grid->addWidget( buttons, 5, 0, 1, 3 );

  grid->setSizeConstraint( QLayout::SetFixedSize );
}

void PinEntryDialog::hideEvent( QHideEvent* ev )
{
  if ( !_pinentry_info || _pinentry_info->grab )
    _edit->releaseKeyboard();
  _grabbed = false;
  QDialog::hideEvent( ev );
}

void PinEntryDialog::showEvent( QShowEvent* event )
{
    QDialog::showEvent( event );
    raiseWindow( this );
}

void PinEntryDialog::setDescription( const QString& txt )
{
  _desc->setVisible( !txt.isEmpty() );
  _desc->setText( txt );
#ifndef QT_NO_ACCESSIBILITY
  _desc->setAccessibleDescription ( txt );
#endif
  _icon->setPixmap( icon() );
  setError( QString::null );
}

QString PinEntryDialog::description() const
{
  return _desc->text();
}

void PinEntryDialog::setError( const QString& txt )
{
  if( !txt.isNull() )_icon->setPixmap( icon( QStyle::SP_MessageBoxCritical ) );
  _error->setText( txt );
#ifndef QT_NO_ACCESSIBILITY
  _error->setAccessibleDescription ( txt );
#endif
  _error->setVisible( !txt.isEmpty() );
}

QString PinEntryDialog::error() const
{
  return _error->text();
}

void PinEntryDialog::setPin( const QString & txt )
{
    _edit->setText( txt );
}

QString PinEntryDialog::pin() const
{
    return _edit->text();
}

void PinEntryDialog::setPrompt( const QString& txt )
{
  _prompt->setText( txt );
  _prompt->setVisible( !txt.isEmpty() );
}

QString PinEntryDialog::prompt() const
{
  return _prompt->text();
}

void PinEntryDialog::setOkText( const QString& txt )
{
  _ok->setText( txt );
#ifndef QT_NO_ACCESSIBILITY
  _ok->setAccessibleDescription ( txt );
#endif
  _ok->setVisible( !txt.isEmpty() );
}

void PinEntryDialog::setCancelText( const QString& txt )
{
  _cancel->setText( txt );
#ifndef QT_NO_ACCESSIBILITY
  _cancel->setAccessibleDescription ( txt );
#endif
  _cancel->setVisible( !txt.isEmpty() );
}

void PinEntryDialog::setQualityBar( const QString& txt )
{
  if (_have_quality_bar) {
    _quality_bar_label->setText( txt );
#ifndef QT_NO_ACCESSIBILITY
    _quality_bar_label->setAccessibleDescription ( txt );
#endif
  }
}

void PinEntryDialog::setQualityBarTT( const QString& txt )
{
  if (_have_quality_bar)
    _quality_bar->setToolTip( txt );
}

void PinEntryDialog::updateQuality(const QString & txt )
{
  int length;
  int percent;
  QPalette pal;

  if (_timer)
    _timer->stop();

  if (!_have_quality_bar || !_pinentry_info)
    return;
  const char* pin = txt.toUtf8().constData();
  length = strlen (pin);
  percent = length? pinentry_inq_quality (_pinentry_info, pin, length) : 0;
  if (!length)
    {
      _quality_bar->reset ();
    }
  else
    {
      pal = _quality_bar->palette ();
      if (percent < 0)
        {
          pal.setColor (QPalette::Highlight, QColor("red"));
          percent = -percent;
        }
      else
        {
          pal.setColor (QPalette::Highlight, QColor("green"));
        }
      _quality_bar->setPalette (pal);
      _quality_bar->setValue (percent);
    }
}

void PinEntryDialog::setPinentryInfo(pinentry_t peinfo)
{
    _pinentry_info = peinfo;
}

void PinEntryDialog::paintEvent( QPaintEvent* event )
{
  // Grab keyboard. It might be a little weird to do it here, but it works!
  // Previously this code was in showEvent, but that did not work in Qt4.
  QDialog::paintEvent( event );
  if ( !_grabbed && ( !_pinentry_info || _pinentry_info->grab ) ) {
    _edit->grabKeyboard();
    _grabbed = true;
  }

}

#include "pinentrydialog.moc"
