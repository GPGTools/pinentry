/* pinentrydialog.h - A secure KDE dialog for PIN entry.
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

#ifndef __PINENTRYDIALOG_H__
#define __PINENTRYDIALOG_H__

#include <qdialog.h>
#include "pinentry.h"

class QLabel;
class QPushButton;
class QProgressBar;
class SecQLineEdit;
class SecQString;

class PinEntryDialog : public QDialog {
  Q_OBJECT

  Q_PROPERTY( QString description READ description WRITE setDescription )
  Q_PROPERTY( QString error READ error WRITE setError )
    //  Q_PROPERTY( SecQString text READ text WRITE setText )
  Q_PROPERTY( QString prompt READ prompt WRITE setPrompt )
public:
  friend class PinEntryController; // TODO: remove when assuan lets me use Qt eventloop.
  PinEntryDialog( QWidget* parent = 0, const char* name = 0, 
                  bool modal = false, bool enable_quality_bar = false );

  void setDescription( const QString& );
  QString description() const;

  void setError( const QString& );
  QString error() const;

  void setText( const SecQString& );
  SecQString text() const;

  void setPrompt( const QString& );
  QString prompt() const;
  
  void setOkText( const QString& );
  void setCancelText( const QString& );

  void setQualityBar( const QString& );
  void setQualityBarTT( const QString& );

  void setPinentryInfo (pinentry_t);

public slots:
  void updateQuality(const SecQString &);

signals:
  void accepted();
  void rejected();

protected:
  virtual void keyPressEvent( QKeyEvent *e );
  virtual void hideEvent( QHideEvent* );
  virtual void paintEvent( QPaintEvent* );

private:
  QLabel*    _icon;
  QLabel*    _desc;
  QLabel*    _error;
  QLabel*    _prompt;
  QLabel*    _quality_bar_label;
  QProgressBar* _quality_bar;
  SecQLineEdit* _edit;
  QPushButton* _ok;
  QPushButton* _cancel;  
  bool       _grabbed;
  bool       _have_quality_bar;
  pinentry_t _pinentry_info;
};


#endif // __PINENTRYDIALOG_H__
