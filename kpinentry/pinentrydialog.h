#ifndef __PINENTRYDIALOG_H__
#define __PINENTRYDIALOG_H__

#include <qdialog.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qlabel.h>

class PinEntryDialog : public QDialog {
  Q_OBJECT

  Q_PROPERTY( QString description READ description WRITE setDescription )
  Q_PROPERTY( QString error READ error WRITE setError )
  Q_PROPERTY( QString text READ text WRITE setText )
  Q_PROPERTY( QString prompt READ prompt WRITE setPrompt )
public:
  friend class PinEntryController; // TODO: remove when assuan lets me use Qt eventloop.
  PinEntryDialog( QWidget* parent = 0, const char* name = 0, bool modal = false );

  void setDescription( const QString& );
  QString description() const;

  void setError( const QString& );
  QString error() const;

  void setText( const QString& );
  QString text() const;

  void setPrompt( const QString& );
  QString prompt() const;
  
signals:
  void accepted();
  void rejected();

protected:
  virtual void keyPressEvent( QKeyEvent *e );
  virtual void hideEvent( QHideEvent* );
  virtual void paintEvent( QPaintEvent* );

private:
  QLabel*    _desc;
  QLabel*    _error;
  QLabel*    _prompt;
  QLineEdit* _edit;
  bool       _grabbed;
};


#endif // __PINENTRYDIALOG_H__
