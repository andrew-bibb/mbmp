/**************************** settings.h *****************************

Dialog to select program settings

Copyright (C) 2015-2017
by: Andrew J. Bibb
License: MIT 

Permission is hereby granted, free of charge, to any person obtaining a copy 
of this software and associated documentation files (the "Software"),to deal 
in the Software without restriction, including without limitation the rights 
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
copies of the Software, and to permit persons to whom the Software is 
furnished to do so, subject to the following conditions: 

The above copyright notice and this permission notice shall be included 
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
DEALINGS IN THE SOFTWARE.
***********************************************************************/ 

# ifndef SETTINGS_H
# define SETTINGS_H

# include <QDialog>
# include <QSettings>
# include <QString>
# include <QSize>
# include <QPoint>
# include <QVariant>
# include <QStringList>
# include <QButtonGroup>
# include <QByteArray>

# include "ui_settings.h"


//	The main program class based on a QDialog
class Settings : public QDialog
{
  Q_OBJECT

  public:
    Settings(QWidget*);
    inline bool useStartOptions() {return ui.checkBox_usestartoptions->isChecked();}
    inline bool useState() {return ui.checkBox_retainstate->isChecked();}
    inline bool usePlaylist() {return ui.checkBox_retainplaylist->isChecked();}
    inline bool useNotifications() {return (ui.checkBox_notifydaemon->isEnabled() && ui.checkBox_notifydaemon->isChecked() );}
    inline bool useDisableTT() {return ui.checkBox_disabletooltips->isChecked();}
    inline bool useDisableXSS() {return (ui.checkBox_disablexscreensaver->isEnabled() && ui.checkBox_disablexscreensaver->isChecked() );}
    inline bool useDisableDPMS() {return ui.checkBox_disabledpms->isChecked();}
    inline bool useDisableInternet() {return ui.checkBox_disableinternet->isChecked();}
    inline bool useYouTubeDL() {return ui.checkBox_useyoutubedl->isChecked();}
    
  	void saveElementGeometry(const QString&, const bool&, const QSize&, const QPoint&);
  	void restoreElementGeometry(const QString&, QWidget*);
  	QStringList getPlaylist();
  	QVariant getSetting(const QString&, const QString&);
  	void setNotificationsTrying(const QString&); 
  	void setNotificationsConnected(const QString&);
  	void setNotificationsFailed(); 	    
  	
  public slots:
  void writeSettings();
  	
  private:
  // members 
    Ui::Settings ui;    
    QSettings* settings;
    QButtonGroup* bg01;
    QString editor_string;
    
  private slots:
		void openEditor(QAbstractButton*);
		void callColorDialog();
		void iconColorChanged(const QString&);
    bool isProcessRunning(const QByteArray&);
    bool isProgramAvailable(const QString&);
};

#endif

