/* pinentrydialog.cpp - A secure KDE dialog for PIN entry.
   Copyright (C) 2002 Klarälvdalens Datakonsult AB
   Copyright (C) 2007 g10 Code GmbH
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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA  */


#include <qlayout.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qmessagebox.h>
#include <qprogressbar.h>
#include <qtooltip.h>

#include "secqlineedit.h"

#include "pinentrydialog.h"
#include "pinentry.h"

PinEntryDialog::PinEntryDialog( QWidget* parent, const char* name, 
                                bool modal, bool enable_quality_bar )
  : QDialog( parent, name, modal, Qt::WStyle_StaysOnTop ), _grabbed( false )
{
  QBoxLayout* top = new QVBoxLayout( this, 6 );
  QBoxLayout* upperLayout = new QHBoxLayout( top );

  _icon = new QLabel( this );
  _icon->setPixmap( QMessageBox::standardIcon( QMessageBox::Information ) );
  upperLayout->addWidget( _icon );

  QBoxLayout* labelLayout = new QVBoxLayout( upperLayout );

  _error = new QLabel( this );
  labelLayout->addWidget( _error );

  _desc = new QLabel( this );
  labelLayout->addWidget( _desc );

  QGridLayout* grid = new QGridLayout( labelLayout );

  _prompt = new QLabel( this );
  _prompt->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  grid->addWidget( _prompt, 0, 0 );
  _edit = new SecQLineEdit( this );
  _edit->setMaxLength( 256 );
  _edit->setEchoMode( SecQLineEdit::Password );
  grid->addWidget( _edit, 0, 1 );

  if (enable_quality_bar)
    {
      _quality_bar_label = new QLabel( this );
      _quality_bar_label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
      grid->addWidget ( _quality_bar_label, 1, 0 );
      _quality_bar = new QProgressBar( this );
      _quality_bar->setCenterIndicator( true );
      grid->addWidget( _quality_bar, 1, 1 );
      _have_quality_bar = true;
    }
  else
    _have_quality_bar = false;

  QBoxLayout* l = new QHBoxLayout( top );

  _ok = new QPushButton( tr("OK"), this );
  _cancel = new QPushButton( tr("Cancel"), this );

  l->addWidget( _ok );
  l->addStretch();
  l->addWidget( _cancel );

  _ok->setDefault(true);

  connect( _ok, SIGNAL( clicked() ),
	   this, SIGNAL( accepted() ) );
  connect( _cancel, SIGNAL( clicked() ),
	   this, SIGNAL( rejected() ) );
  connect( _edit, SIGNAL( textModified(const SecQString&) ),
	   this, SLOT( updateQuality(const SecQString&) ) );
  connect (this, SIGNAL (accepted ()),
	   this, SLOT (accept ()));
  connect (this, SIGNAL (rejected ()),
	   this, SLOT (reject ()));
  _edit->setFocus();
}

void PinEntryDialog::paintEvent( QPaintEvent* ev )
{
  // Grab keyboard when widget is mapped to screen
  // It might be a little weird to do it here, but it works!
  if( !_grabbed ) {
    _edit->grabKeyboard();
    _grabbed = true;
  }
  QDialog::paintEvent( ev );
}

void PinEntryDialog::hideEvent( QHideEvent* ev )
{
  _edit->releaseKeyboard();
  _grabbed = false;
  QDialog::hideEvent( ev );
}

void PinEntryDialog::keyPressEvent( QKeyEvent* e ) 
{
  if ( e->state() == 0 && e->key() == Key_Escape ) {
    emit rejected();
    return;
  }
  QDialog::keyPressEvent( e );
}


void PinEntryDialog::updateQuality( const SecQString & txt )
{
  char *pin;
  int length;
  int percent;
  QPalette pal;

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
          pal.setColor (QColorGroup::Highlight, QColor("red"));
          percent = -percent;
        }
      else
        {
          pal.setColor (QColorGroup::Highlight, QColor("green"));
        }
      _quality_bar->setPalette (pal);
      _quality_bar->setProgress (percent);
    }
}


void PinEntryDialog::setDescription( const QString& txt ) 
{
  _desc->setText( txt );
  _icon->setPixmap( QMessageBox::standardIcon( QMessageBox::Information ) );
  setError( QString::null );
}

QString PinEntryDialog::description() const 
{
  return _desc->text();
}

void PinEntryDialog::setError( const QString& txt ) 
{
  if ( !txt.isNull() )
    _icon->setPixmap( QMessageBox::standardIcon( QMessageBox::Critical ) );
  _error->setText( txt );
}

QString PinEntryDialog::error() const 
{
  return _error->text();
}

void PinEntryDialog::setText( const SecQString& txt ) 
{
  _edit->setText( txt );
}

SecQString PinEntryDialog::text() const 
{
  return _edit->text();
}

void PinEntryDialog::setPrompt( const QString& txt ) 
{
  _prompt->setText( txt );
}

QString PinEntryDialog::prompt() const 
{
  return _prompt->text();
}

void PinEntryDialog::setOkText( const QString& txt )
{
  _ok->setText( txt );
}

void PinEntryDialog::setCancelText( const QString& txt )
{
  _cancel->setText( txt );
}

void PinEntryDialog::setQualityBar( const QString& txt )
{
  if (_have_quality_bar)
    _quality_bar_label->setText( txt );
}

void PinEntryDialog::setQualityBarTT( const QString& txt )
{
  if (_have_quality_bar)
    QToolTip::add ( _quality_bar, txt );
}

void PinEntryDialog::setPinentryInfo (pinentry_t peinfo )
{
  _pinentry_info = peinfo;
}
