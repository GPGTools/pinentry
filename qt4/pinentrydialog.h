/* 
   pinentrydialog.h - A (not yet) secure Qt 4 dialog for PIN entry.

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

#ifndef __PINENTRYDIALOG_H__
#define __PINENTRYDIALOG_H__

#include <QDialog>

class QLabel;
class QPushButton;
#ifdef WITH_SECURE_QSTRING
class SecQLineEdit;
class SecQString;
#else
class QLineEdit;
class QString;
#define SecQLineEdit QLineEdit
#define SecQString QString
#endif

class PinEntryDialog : public QDialog {
  Q_OBJECT

  Q_PROPERTY( QString description READ description WRITE setDescription )
  Q_PROPERTY( QString error READ error WRITE setError )
    //  Q_PROPERTY( SecQString text READ text WRITE setText )
  Q_PROPERTY( QString prompt READ prompt WRITE setPrompt )
public:
  friend class PinEntryController; // TODO: remove when assuan lets me use Qt eventloop.
  PinEntryDialog( QWidget* parent = 0, const char* name = 0, bool modal = false );

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
  SecQLineEdit* _edit;
  QPushButton* _ok;
  QPushButton* _cancel;  
  bool       _grabbed;
};

#endif // __PINENTRYDIALOG_H__
