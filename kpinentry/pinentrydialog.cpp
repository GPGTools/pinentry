#include <qlayout.h>
#include "pinentrydialog.h"

#include <X11/Xlib.h>

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
    XGrabKeyboard( x11Display(), winId(), 
		   TRUE, GrabModeAsync, GrabModeAsync, CurrentTime );
    _grabbed = true;
  }
  QDialog::paintEvent( ev );
}

void PinEntryDialog::hideEvent( QHideEvent* ev )
{
  XUngrabKeyboard( x11Display(), CurrentTime );
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
