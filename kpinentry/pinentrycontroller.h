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
