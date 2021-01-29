/* pinentrydialog.h - A secure KDE dialog for PIN entry.
 * Copyright (C) 2002 Klarälvdalens Datakonsult AB
 * Written by Steffen Hansen <steffen@klaralvdalens-datakonsult.se>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef __PINENTRYDIALOG_H__
#define __PINENTRYDIALOG_H__

#include <ntqdialog.h>
#include "pinentry.h"

class TQLabel;
class TQPushButton;
class TQProgressBar;
class SecTQLineEdit;
class SecTQString;

class PinEntryDialog : public TQDialog {
  TQ_OBJECT

  TQ_PROPERTY( TQString description READ description WRITE setDescription )
  TQ_PROPERTY( TQString error READ error WRITE setError )
    //  TQ_PROPERTY( SecTQString text READ text WRITE setText )
  TQ_PROPERTY( TQString prompt READ prompt WRITE setPrompt )
public:
  friend class PinEntryController; // TODO: remove when assuan lets me use TQt eventloop.
  PinEntryDialog( TQWidget* parent = 0, const char* name = 0,
                  bool modal = false, bool enable_quality_bar = false );

  void setDescription( const TQString& );
  TQString description() const;

  void setError( const TQString& );
  TQString error() const;

  void setText( const SecTQString& );
  SecTQString text() const;

  void setPrompt( const TQString& );
  TQString prompt() const;

  void setOkText( const TQString& );
  void setCancelText( const TQString& );

  void setQualityBar( const TQString& );
  void setQualityBarTT( const TQString& );

  void setPinentryInfo (pinentry_t);

public slots:
  void updateQuality(const SecTQString &);
  void onBackspace();

signals:
  void accepted();
  void rejected();

protected:
  virtual void keyPressEvent( TQKeyEvent *e );
  virtual void hideEvent( TQHideEvent* );
  virtual void paintEvent( TQPaintEvent* );

private:
  TQLabel*    _icon;
  TQLabel*    _desc;
  TQLabel*    _error;
  TQLabel*    _prompt;
  TQLabel*    _quality_bar_label;
  TQProgressBar* _quality_bar;
  SecTQLineEdit* _edit;
  TQPushButton* _ok;
  TQPushButton* _cancel;
  bool       _grabbed;
  bool       _have_quality_bar;
  pinentry_t _pinentry_info;
  bool       _disable_echo_allowed;
};


#endif // __PINENTRYDIALOG_H__
