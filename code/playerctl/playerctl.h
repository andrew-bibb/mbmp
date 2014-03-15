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

# include "ui_playerctl.h"
# include "./code/gstiface/gstiface.h"
# include "./code/keymap/keymap.h"
# include "./code/playlist/playlist.h"

class PlayerControl : public QDialog
{
	Q_OBJECT
	
	public:
    PlayerControl(const QCommandLineParser&, QWidget* parent = 0);
    QList<QKeySequence> getShortcuts(const QString&);
    void setDurationWidgets(int, bool seek_enabled = false);
    void setPositionWidgets(int);
    
  public slots:
		void changeVolume(int);
		void playMedia(QAction* act = 0);
		void seekToPosition(QAction*);
		void playPause();
		void toggleFullScreen();
		void togglePlaylist();
		void toggleGUI();
		void showAbout();
		void aboutMBMP();
		void aboutNuvola();
		void showLicense();
		void showChangeLog();
		
	private slots:
		void processBusMessages(int, QString);
		void changeVolumeDialStep(QAction*);
		void popupVisualizerMenu();
		void changeVisualizer(QAction*);

	protected:
		void contextMenuEvent(QContextMenuEvent*);		
	
  private:
  // members 
    Ui::PlayerControl ui;
    GST_Interface* p_gstiface;
    KeyMap* keymap;
    Playlist* playlist;
		QActionGroup* playlist_group;    
		QActionGroup* volume_group;
		QActionGroup* seek_group;
		QMenu* control_menu;
		QMenu* vis_menu;
		QMenu* advanced_menu;
		QCursor ncurs;
		QFile logfile;
		int loglevel;
		QMessageBox* chtsht;
  
  // functions
		QString readTextFile(const char*);  

};

# endif
