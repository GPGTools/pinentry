/*
   pinentrydialog.h - A (not yet) secure Qt 4 dialog for PIN entry.

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

#ifndef __PINENTRYDIALOG_H__
#define __PINENTRYDIALOG_H__

#include <QDialog>
#include <QStyle>
#include <QTimer>

#include "secstring.h"
#include "pinentry.h"

class QLabel;
class QPushButton;
class QSecureLineEdit;
class QString;
class QProgressBar;

QPixmap icon( QStyle::StandardPixmap which = QStyle::SP_CustomBase );

void raiseWindow( QWidget* w );

class PinEntryDialog : public QDialog {
  Q_OBJECT

  Q_PROPERTY( QString description READ description WRITE setDescription )
  Q_PROPERTY( QString error READ error WRITE setError )
  Q_PROPERTY( secqstring pin READ pin WRITE setPin )
  Q_PROPERTY( QString prompt READ prompt WRITE setPrompt )
public:
  friend class PinEntryController; // TODO: remove when assuan lets me use Qt eventloop.
  explicit PinEntryDialog( QWidget* parent = 0, const char* name = 0, int timeout = 0, bool modal = false, bool enable_quality_bar = false );

  void setDescription( const QString& );
  QString description() const;

  void setError( const QString& );
  QString error() const;

  void setPin( const secqstring & );
  secqstring pin() const;

  void setPrompt( const QString& );
  QString prompt() const;

  void setOkText( const QString& );
  void setCancelText( const QString& );

  void setQualityBar( const QString& );
  void setQualityBarTT( const QString& );

  void setPinentryInfo (pinentry_t);

public slots:
  void updateQuality(const secqstring&);
  void slotTimeout();

protected:
  /* reimp */ void showEvent( QShowEvent* event );
  /* reimp */ void hideEvent( QHideEvent* );
  /* reimp */ void paintEvent( QPaintEvent* event );

private:
  QLabel*    _icon;
  QLabel*    _desc;
  QLabel*    _error;
  QLabel*    _prompt;
  QLabel*    _quality_bar_label;
  QProgressBar* _quality_bar;
  QSecureLineEdit* _edit;
  QPushButton* _ok;
  QPushButton* _cancel;
  bool       _grabbed;
  bool       _have_quality_bar;
  pinentry_t _pinentry_info;
  QTimer*    _timer;
};

#endif // __PINENTRYDIALOG_H__
