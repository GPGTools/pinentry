/* pinentrycontroller.h - A secure KDE dialog for PIN entry.
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

#ifndef __PINENTRYCONTROLLER_H__
#define __PINENTRYCONTROLLER_H__

#include <cstdio>
#include <assuan.h>

#include <qobject.h>

class PinEntryDialog;

class PinEntryController : public QObject {
  Q_OBJECT
public:
  PinEntryController();
  virtual ~PinEntryController();

  static int assuanDesc( ASSUAN_CONTEXT, char* );
  static int assuanPrompt( ASSUAN_CONTEXT, char* );
  static int assuanError( ASSUAN_CONTEXT, char* );
  static int assuanGetpin( ASSUAN_CONTEXT, char* );
  static int assuanConfirm( ASSUAN_CONTEXT, char* );

public slots:
  void slotAccepted();
  void slotRejected();

  virtual void exec();
private:
  int getPin( char* );
  int confirm( char* );

  int registerCommands();

  ASSUAN_CONTEXT _ctx;

  PinEntryDialog* _pinentry;
  QString _desc;
  QString _prompt;
  QString _error;
};

#endif // __PINENTRYCONTROLLER_H__
