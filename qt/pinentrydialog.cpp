/* pinentrydialog.cpp - A secure KDE dialog for PIN entry.
   Copyright (C) 2002 Klarälvdalens Datakonsult AB
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
#include "pinentrydialog.h"

PinEntryDialog::PinEntryDialog( QWidget* parent, const char* name, bool modal )
  : QDialog( parent, name, modal ), _grabbed( false )
{
  QBoxLayout* top = new QVBoxLayout( this, 6 );
  _error = new QLabel( this );
  top->addWidget( _error );

  _desc = new QLabel( this );
  top->addWidget( _desc );

  QBoxLayout* l = new QHBoxLayout( top );
  _prompt = new QLabel( this );
  l->addWidget( _prompt );
  _edit  = new QLineEdit( this );
  _edit->setMaxLength( 256 );
  _edit->setEchoMode( QLineEdit::Password );
  l->addWidget( _edit );

  l = new QHBoxLayout( top );
  
  QPushButton* okB = new QPushButton( tr("OK"), this );
  QPushButton* cancelB = new QPushButton( tr("Cancel"), this );

  l->addWidget( okB );
  l->addStretch();
  l->addWidget( cancelB );

  okB->setDefault(true);

  connect( okB, SIGNAL( clicked() ),
	   this, SIGNAL( accepted() ) );
  connect( cancelB, SIGNAL( clicked() ),
	   this, SIGNAL( rejected() ) );
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

void PinEntryDialog::setDescription( const QString& txt ) 
{
  _desc->setText( txt );
  setError( QString::null );
}

QString PinEntryDialog::description() const 
{
  return _desc->text();
}

void PinEntryDialog::setError( const QString& txt ) 
{
  _error->setText( txt );
}

QString PinEntryDialog::error() const 
{
  return _error->text();
}

void PinEntryDialog::setText( const QString& txt ) 
{
  _edit->setText( txt );
}

QString PinEntryDialog::text() const 
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
