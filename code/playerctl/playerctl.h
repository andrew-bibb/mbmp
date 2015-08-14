/**************************** playerctl.h ******************************
Main window which also handles all the controls.

Copyright (C) 2014-2015
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
# ifndef PLAYER_CONTROL
# define PLAYER_CONTROL

# include <QDialog>
# include <QKeyEvent>
# include <QCommandLineParser>
# include <QContextMenuEvent>
# include <QList>
# include <QKeySequence>
# include <QActionGroup>
# include <QAction>
# include <QCursor>
# include <QFile>
# include <QMenu>
# include <QMessageBox>
# include <QTimer>

# include "ui_playerctl.h"

# include "./code/settings/settings.h"
# include "./code/gstiface/gstiface.h"
# include "./code/keymap/keymap.h"
# include "./code/playlist/playlist.h"
# include "./code/videowidget/videowidget.h"
# include "./code/scrollbox/scrollbox.h"
# include "./code/notify/notify.h"

class PlayerControl : public QDialog
{
	Q_OBJECT
	
	public:
    PlayerControl(const QCommandLineParser&, QWidget* parent = 0);
    QList<QKeySequence> getShortcuts(const QString&);
    
  public slots:
		void changeVolume(int);
		void initializeCD();
		void initializeDVD();
		void playMedia(QAction* act = 0);
		void stopPlaying();
		void seekToPosition(QAction*);
		void dvdNavigationCommand(QAction*);
		void playPause();
		void toggleFullScreen();
		void togglePlaylist();
		void toggleGUI();
		void toggleCheatsheet();
		void toggleSettingsDialog();
		void showAbout();
		void aboutMBMP();
		void aboutNuvola();
		void showLicense();
		void showChangeLog();
		
	private slots:
		void processGstifaceMessages(int, QString);
		void changeVolumeDialStep(QAction*);
		void popupVisualizerMenu();
		void popupOptionsMenu();
		void changeVisualizer(QAction*);
		void changeOptions(QAction*);
    void cleanUp();
    void connectNotifyClient();
    void setDurationWidgets(int, bool seek_enabled = false);
    void setPositionWidgets();

	protected:
		void contextMenuEvent(QContextMenuEvent*);		
		bool eventFilter(QObject*, QEvent*);
	
  private:
  // mbmp members 
    Ui::PlayerControl ui;
    Settings* diag_settings;
    GST_Interface* gstiface;
    KeyMap* keymap;
    Playlist* playlist;
    VideoWidget* videowidget;
    ScrollBox* chtsht;
    NotifyClient* notifyclient;
    QTimer* pos_timer;
    bool b_logtofile;
 
  // plain members 
		QActionGroup* playlist_group;    
		QActionGroup* volume_group;
		QActionGroup* seek_group;
		QActionGroup* dvd_group;
		QActionGroup* vis_group;
		QMenu* control_menu;
		QMenu* vis_menu;
		QMenu* advanced_menu;
		QMenu* options_menu;
		QCursor ncurs;
		QFile logfile;
		int loglevel;
		QAction* action_vis;
		QAction* action_sub;
		QAction* action_sbuf;
		QAction* action_dbuf;
		int hiatus_resume;
		
  
  // functions
		QString readTextFile(const char*);

};

# endif
