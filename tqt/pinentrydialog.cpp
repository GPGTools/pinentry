/* pinentrydialog.cpp - A secure KDE dialog for PIN entry.
 * Copyright (C) 2002 Klarälvdalens Datakonsult AB
 * Copyright (C) 2007 g10 Code GmbH
 * Written by Steffen Hansen <steffen@klaralvdalens-datakonsult.se>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <ntqlayout.h>
#include <ntqpushbutton.h>
#include <ntqlabel.h>
#include <ntqmessagebox.h>
#include <ntqprogressbar.h>
#include <ntqtooltip.h>

#include "secqlineedit.h"

#include "pinentrydialog.h"
#include "pinentry.h"

PinEntryDialog::PinEntryDialog( TQWidget* parent, const char* name,
                                bool modal, bool enable_quality_bar )
  : TQDialog( parent, name, modal, TQt::WStyle_StaysOnTop ), _grabbed( false ),
    _disable_echo_allowed ( true )
{
  TQBoxLayout* top = new TQVBoxLayout( this, 6 );
  TQBoxLayout* upperLayout = new TQHBoxLayout( top );

  _icon = new TQLabel( this );
  _icon->setPixmap( TQMessageBox::standardIcon( TQMessageBox::Information ) );
  upperLayout->addWidget( _icon );

  TQBoxLayout* labelLayout = new TQVBoxLayout( upperLayout );

  _error = new TQLabel( this );
  labelLayout->addWidget( _error );

  _desc = new TQLabel( this );
  labelLayout->addWidget( _desc );

  TQGridLayout* grid = new TQGridLayout( labelLayout );

  _prompt = new TQLabel( this );
  _prompt->setAlignment( TQt::AlignRight | TQt::AlignVCenter );
  grid->addWidget( _prompt, 0, 0 );
  _edit = new SecTQLineEdit( this );
  _edit->setMaxLength( 256 );
  _edit->setEchoMode( SecTQLineEdit::Password );
  grid->addWidget( _edit, 0, 1 );

  if (enable_quality_bar)
    {
      _quality_bar_label = new TQLabel( this );
      _quality_bar_label->setAlignment( TQt::AlignRight | TQt::AlignVCenter );
      grid->addWidget ( _quality_bar_label, 1, 0 );
      _quality_bar = new TQProgressBar( this );
      _quality_bar->setCenterIndicator( true );
      grid->addWidget( _quality_bar, 1, 1 );
      _have_quality_bar = true;
    }
  else
    _have_quality_bar = false;

  TQBoxLayout* l = new TQHBoxLayout( top );

  _ok = new TQPushButton( tr("OK"), this );
  _cancel = new TQPushButton( tr("Cancel"), this );

  l->addWidget( _ok );
  l->addStretch();
  l->addWidget( _cancel );

  _ok->setDefault(true);

  connect( _ok, SIGNAL( clicked() ),
	   this, SIGNAL( accepted() ) );
  connect( _cancel, SIGNAL( clicked() ),
	   this, SIGNAL( rejected() ) );
  connect( _edit, SIGNAL( textModified(const SecTQString&) ),
	   this, SLOT( updateQuality(const SecTQString&) ) );
  connect (_edit, SIGNAL (backspacePressed()),
	   this, SLOT (onBackspace ()));
  connect (this, SIGNAL (accepted ()),
	   this, SLOT (accept ()));
  connect (this, SIGNAL (rejected ()),
	   this, SLOT (reject ()));
  _edit->setFocus();
}

void PinEntryDialog::paintEvent( TQPaintEvent* ev )
{
  // Grab keyboard when widget is mapped to screen
  // It might be a little weird to do it here, but it works!
  if( !_grabbed ) {
    _edit->grabKeyboard();
    _grabbed = true;
  }
  TQDialog::paintEvent( ev );
}

void PinEntryDialog::hideEvent( TQHideEvent* ev )
{
  _edit->releaseKeyboard();
  _grabbed = false;
  TQDialog::hideEvent( ev );
}

void PinEntryDialog::keyPressEvent( TQKeyEvent* e )
{
  if ( e->state() == 0 && e->key() == Key_Escape ) {
    emit rejected();
    return;
  }
  TQDialog::keyPressEvent( e );
}


void PinEntryDialog::updateQuality( const SecTQString & txt )
{
  char *pin;
  int length;
  int percent;
  TQPalette pal;

  _disable_echo_allowed = false;

  if (!_have_quality_bar || !_pinentry_info)
    return;
  pin = (char*)txt.utf8();
  length = strlen (pin);
  percent = length? pinentry_inq_quality (_pinentry_info, pin, length) : 0;
  ::secmem_free (pin);
  if (!length)
    {
      _quality_bar->reset ();
    }
  else
    {
      pal = _quality_bar->palette ();
      if (percent < 0)
        {
          pal.setColor (TQColorGroup::Highlight, TQColor("red"));
          percent = -percent;
        }
      else
        {
          pal.setColor (TQColorGroup::Highlight, TQColor("green"));
        }
      _quality_bar->setPalette (pal);
      _quality_bar->setProgress (percent);
    }
}


void PinEntryDialog::onBackspace()
{
  if (_disable_echo_allowed)
    _edit->setEchoMode( SecTQLineEdit::NoEcho );
}


void PinEntryDialog::setDescription( const TQString& txt )
{
  _desc->setText( txt );
  _icon->setPixmap( TQMessageBox::standardIcon( TQMessageBox::Information ) );
  setError( TQString::null );
}

TQString PinEntryDialog::description() const
{
  return _desc->text();
}

void PinEntryDialog::setError( const TQString& txt )
{
  if ( !txt.isNull() )
    _icon->setPixmap( TQMessageBox::standardIcon( TQMessageBox::Critical ) );
  _error->setText( txt );
}

TQString PinEntryDialog::error() const
{
  return _error->text();
}

void PinEntryDialog::setText( const SecTQString& txt )
{
  _edit->setText( txt );
}

SecTQString PinEntryDialog::text() const
{
  return _edit->text();
}

void PinEntryDialog::setPrompt( const TQString& txt )
{
  _prompt->setText( txt );
  if (txt.contains("PIN"))
    _disable_echo_allowed = false;
}

TQString PinEntryDialog::prompt() const
{
  return _prompt->text();
}

void PinEntryDialog::setOkText( const TQString& txt )
{
  _ok->setText( txt );
}

void PinEntryDialog::setCancelText( const TQString& txt )
{
  _cancel->setText( txt );
}

void PinEntryDialog::setQualityBar( const TQString& txt )
{
  if (_have_quality_bar)
    _quality_bar_label->setText( txt );
}

void PinEntryDialog::setQualityBarTT( const TQString& txt )
{
  if (_have_quality_bar)
    TQToolTip::add ( _quality_bar, txt );
}

void PinEntryDialog::setPinentryInfo (pinentry_t peinfo )
{
  _pinentry_info = peinfo;
}

#include "pinentrydialog.moc"
