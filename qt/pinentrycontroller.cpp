/* pinentrycontroller.cpp - A secure KDE dialog for PIN entry.
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

#include "pinentrycontroller.h"
#include "pinentrydialog.h"
extern "C"
{
#include "memory.h"
}
#ifdef USE_KDE
# include <kmessagebox.h>
#else
# include <qmessagebox.h>
#endif

PinEntryController::PinEntryController() : _pinentry( 0 )
{
  int fds[2];
  fds[0] = 0;
  fds[1] = 1;
  
  assuan_set_malloc_hooks( secmem_malloc, secmem_realloc, secmem_free );
  int rc = assuan_init_pipe_server( &_ctx, fds );
  if( rc ) {
    qDebug(assuan_strerror( static_cast<AssuanError>(rc) ));
    exit(-1);
  }
  rc = registerCommands();

  assuan_set_pointer( _ctx, this );
}

PinEntryController::~PinEntryController()
{
  assuan_deinit_server( _ctx );
}

void PinEntryController::exec()
{
  while( true ) {
    int rc = assuan_accept( _ctx );
    if( rc == -1 ) {
      qDebug("Assuan terminated");
      break;
    } else if( rc ) {
      qDebug("Assuan accept problem: %s", assuan_strerror( static_cast<AssuanError>(rc) ) );
      break;
    }
    rc = assuan_process( _ctx );
    if( rc ) {
      qDebug("Assuan processing failed: %s", assuan_strerror( static_cast<AssuanError>(rc) ) );
      continue;
    }
  }
}

int PinEntryController::registerCommands()
{
  static struct {
    const char *name;
    int cmd_id;
    int (*handler)(ASSUAN_CONTEXT, char *line);
  } table[] = {
    { "SETDESC",      0,  PinEntryController::assuanDesc },
    { "SETPROMPT",    0,  PinEntryController::assuanPrompt },
    { "SETERROR",     0,  PinEntryController::assuanError },
    { "SETOK",        0,  PinEntryController::assuanOk },
    { "SETCANCEL",    0,  PinEntryController::assuanCancel },
    { "GETPIN",       0,  PinEntryController::assuanGetpin },
    { "CONFIRM",      0,  PinEntryController::assuanConfirm },
    { 0,0,0 }
  };
  int i, j, rc;
  
  for (i=j=0; table[i].name; i++) {
    rc = assuan_register_command (_ctx,
				  table[i].cmd_id? table[i].cmd_id
				  : (ASSUAN_CMD_USER + j++),
				  table[i].name, table[i].handler);
    if (rc) return rc;
  }
  return 0;
}

int PinEntryController::assuanDesc( ASSUAN_CONTEXT ctx, char* line )
{
  //qDebug("PinEntryController::assuanDesc( %s )", line );
  PinEntryController* that =   static_cast<PinEntryController*>(assuan_get_pointer(ctx));
  that->_desc = QString::fromUtf8(line);
  that->_error = QString::null;
  return 0;
}

int PinEntryController::assuanPrompt( ASSUAN_CONTEXT ctx, char* line )
{
  //qDebug("PinEntryController::assuanPrompt( %s )", line );
  PinEntryController* that =   static_cast<PinEntryController*>(assuan_get_pointer(ctx));
  that->_prompt = QString::fromUtf8(line);
  that->_error = QString::null;
  return 0;
}

int PinEntryController::assuanError( ASSUAN_CONTEXT ctx, char* line )
{
  //qDebug("PinEntryController::assuanError( %s )", line );
  PinEntryController* that =   static_cast<PinEntryController*>(assuan_get_pointer(ctx));
  that->_error = QString::fromUtf8(line);
  return 0;
}

int PinEntryController::assuanOk ( ASSUAN_CONTEXT ctx, char* line )
{
  //qDebug("PinEntryController::assuanOk( %s )", line );
  PinEntryController* that =   static_cast<PinEntryController*>(assuan_get_pointer(ctx));
  that->_ok = QString::fromUtf8(line);
  return 0;
}

int PinEntryController::assuanCancel( ASSUAN_CONTEXT ctx, char* line )
{
  //qDebug("PinEntryController::assuanCancel( %s )", line );
  PinEntryController* that =   static_cast<PinEntryController*>(assuan_get_pointer(ctx));
  that->_cancel = QString::fromUtf8(line);
  return 0;
}

int PinEntryController::assuanGetpin( ASSUAN_CONTEXT ctx, char* line )
{
  //qDebug("PinEntryController::assuanGetpin( %s )", line );  
  PinEntryController* that =   static_cast<PinEntryController*>(assuan_get_pointer(ctx));
  return that->getPin( line );
}

int PinEntryController::getPin( char* line ) {
  if( _pinentry == 0 ) {
    _pinentry = new PinEntryDialog(0,0,true);
  }
  _pinentry->setPrompt( _prompt );
  _pinentry->setDescription( _desc );
  _pinentry->setText(QString::null);
  if( !_error.isNull() ) _pinentry->setError( _error );
  connect( _pinentry, SIGNAL( accepted() ),
	   this, SLOT( slotAccepted() ) );
  connect( _pinentry, SIGNAL( rejected() ),
	   this, SLOT( slotRejected() ) );
  bool ret = _pinentry->exec();  
  FILE* fp = assuan_get_data_fp( _ctx );
  if( ret ) {
    fputs( static_cast<const char*>(_pinentry->text().utf8()), fp );
    return 0;
  } else {
    assuan_set_error( _ctx, ASSUAN_Canceled, "Dialog cancelled by user" );
    return ASSUAN_Canceled;
  }
}

int PinEntryController::assuanConfirm( ASSUAN_CONTEXT ctx, char* line )
{
  //qDebug("PinEntryController::assuanConfirm( %s )", line );  
  PinEntryController* that =   static_cast<PinEntryController*>(assuan_get_pointer(ctx));
  return that->confirm( line );  
}

int PinEntryController::confirm( char* line )
{
  int ret;
#ifdef USE_KDE
  if( !_error.isNull() ) {
    ret = KMessageBox::questionYesNo( 0, _error );
  } else {
    ret = KMessageBox::questionYesNo( 0, _desc );    
  }
  FILE* fp = assuan_get_data_fp( _ctx );
  if( ret == KMessageBox::Yes ) {
#else
  if( !_error.isNull() ) {
    ret = QMessageBox::critical( 0, "", _error, QMessageBox::Yes, QMessageBox::No );
  } else {
    ret = QMessageBox::information( 0, "", _desc, QMessageBox::Yes, QMessageBox::No );
  }    
  FILE* fp = assuan_get_data_fp( _ctx );
  if( ret == 0 ) {
#endif // USE_KDE
    fputs( "YES", fp );    
  } else {
    fputs( "NO", fp );
  }
  return 0;
}

void PinEntryController::slotAccepted()
{
  //qDebug("PinEntryController::slotAccepted() NYI");
  _pinentry->accept();
}

void PinEntryController::slotRejected()
{
  //qDebug("PinEntryController::slotRejected() NYI");
  _pinentry->reject();
}
