/* 
   pinentrydialog.cpp - A (not yet) secure Qt 4 dialog for PIN entry.

   Copyright (C) 2002 Klarälvdalens Datakonsult AB
   Copyright 2007 Ingo Klöcker

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

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QKeyEvent>
#include <QLabel>
#include <QMessageBox>

#ifdef WITH_SECURE_QSTRING
#include "secqlineedit.h"
#else
#include <QLineEdit>
#endif

#include "pinentrydialog.h"

PinEntryDialog::PinEntryDialog( QWidget* parent, const char* name, bool modal )
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
  labelLayout->addWidget( _error );

  _desc = new QLabel( this );
  labelLayout->addWidget( _desc );

  QBoxLayout* l = new QHBoxLayout();
  top->addLayout( l );
  _prompt = new QLabel( this );
  l->addWidget( _prompt );
  _edit = new SecQLineEdit( this );
  _edit->setMaxLength( 256 );
  _edit->setEchoMode( SecQLineEdit::Password );
  l->addWidget( _edit );

  l = new QHBoxLayout();
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

#include "pinentrydialog.moc"
