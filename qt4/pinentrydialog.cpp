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

#include "qsecurelineedit.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QKeyEvent>
#include <QLabel>
#include <QMessageBox>

PinEntryDialog::PinEntryDialog( QWidget* parent, const char* name, bool modal,
                                bool enable_quality_bar )
  : QDialog( parent ), _grabbed( false )
{
  if ( modal ) {
    setWindowModality( Qt::ApplicationModal );
  }

  QBoxLayout* top = new QVBoxLayout( this );
  top->setMargin( 6 );
  QBoxLayout* upperLayout = new QHBoxLayout();
  top->addLayout( upperLayout );

  _icon = new QLabel( this );
  _icon->setPixmap( QMessageBox::standardIcon( QMessageBox::Information ) );
  upperLayout->addWidget( _icon );

  QBoxLayout* labelLayout = new QVBoxLayout();
  upperLayout->addLayout( labelLayout );

  _error = new QLabel( this );
  _error->setWordWrap(true);
  labelLayout->addWidget( _error );

  _desc = new QLabel( this );
  _desc->setWordWrap(true);
  labelLayout->addWidget( _desc );

  QGridLayout* grid = new QGridLayout;
  top->addLayout( grid );

  _prompt = new QLabel( this );
  grid->addWidget( _prompt, 0, 0 );
  _edit = new QSecureLineEdit( this );
  _edit->setMaxLength( 256 );
  grid->addWidget( _edit, 0, 1 );

  if (enable_quality_bar)
  {
    _quality_bar_label = new QLabel( this );
    _quality_bar_label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
    grid->addWidget ( _quality_bar_label, 1, 0 );
    _quality_bar = new QProgressBar( this );
    _quality_bar->setAlignment( Qt::AlignCenter );
    grid->addWidget( _quality_bar, 1, 1 );
    _have_quality_bar = true;
  }
  else
    _have_quality_bar = false;

  QBoxLayout* l = new QHBoxLayout();
  top->addLayout( l );

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
  connect( _edit, SIGNAL( textChanged(secqstring) ),
	   this, SLOT( updateQuality(secqstring) ) );
  connect (this, SIGNAL (accepted ()),
	   this, SLOT (accept ()));
  connect (this, SIGNAL (rejected ()),
	   this, SLOT (reject ()));

  _edit->setFocus();
}

void PinEntryDialog::hideEvent( QHideEvent* ev )
{
  if ( !_pinentry_info || _pinentry_info->grab )
    _edit->releaseKeyboard();
  _grabbed = false;
  QDialog::hideEvent( ev );
}

void PinEntryDialog::keyPressEvent( QKeyEvent* e )
{
  if ( e->modifiers() == Qt::NoModifier && e->key() == Qt::Key_Escape ) {
    emit rejected();
    return;
  }
  QDialog::keyPressEvent( e );
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
  if( !txt.isNull() )_icon->setPixmap( QMessageBox::standardIcon( QMessageBox::Critical ) );
  _error->setText( txt );
}

QString PinEntryDialog::error() const
{
  return _error->text();
}

void PinEntryDialog::setPin( const secqstring & txt )
{
    _edit->setText( txt );
}

secqstring PinEntryDialog::pin() const
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
    _quality_bar->setToolTip( txt );
}

void PinEntryDialog::updateQuality(const secqstring & txt )
{
  int length;
  int percent;
  QPalette pal;

  if (!_have_quality_bar || !_pinentry_info)
    return;
  secstring pinStr = toUtf8(txt);
  const char* pin = pinStr.c_str();
  // The Qt3 version called ::secmem_free (pin) here, but from other usage of secstring,
  // it seems like this is not needed anymore - 16 Mar. 2009 13:15 -- Jesper K. Pedersen
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
