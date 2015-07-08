/**************************** settings.h *****************************

Dialog to select program settings

Copyright (C) 2015
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

# include "ui_settings.h"


//	The main program class based on a QDialog
class Settings : public QDialog
{
  Q_OBJECT

  public:
    Settings(QWidget*);
    inline bool useStartOptions() {return usestartoptions;}
    inline bool useState() {return usestate;}
    inline bool usePlaylist() {return useplaylist;}
    inline bool useNotifications() {return usenotifications;}
  	void writeSettings();
  	void saveElementGeometry(const QString&, const bool&, const QSize&, const QPoint&);
  	void restoreElementGeometry(const QString&, QWidget*);
  	void savePlaylist(const QStringList&, const int& cur = 0, const int& pos = 0);
  	QStringList getPlaylist();
  	QVariant getSetting(const QString&, const QString&); 	    
    
  private:
  // members 
    Ui::Settings ui;    
    QSettings* settings;
    bool usestartoptions;
    bool usestate;
    bool useplaylist;
    bool usenotifications;

};

#endif

