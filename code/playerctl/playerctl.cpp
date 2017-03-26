/**************************** playerctl.cpp ****************************
Main window which also handles all the controls.

Copyright (C) 2014-2017
by: Andrew J. Bibb
License: MIT \

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
# include <QtCore/QDebug>
# include <QLocale>
# include <QFile>
# include <QTextStream>
# include <QFile>
# include <QRegExp>
# include <QTime>
# include <QFileInfo>
# include <QProcess>

# include "./code/scman/scman.h"
# include "./code/iconman/iconman.h"
# include "./code/playerctl/playerctl.h"	
# include "./code/resource.h"


PlayerControl::PlayerControl(const QCommandLineParser& parser, QWidget* parent) 
	: QDialog(parent)
{
	// Set the Locale (probably not necessary since the default is the system one anyway)
  QLocale::setDefault(QLocale::system() );	

	// Set the window title
	this->setWindowTitle(LONG_NAME);

	// setup the settings dialog (and read settings)
	diag_settings = new Settings(this);
			  
  // Set icon theme if provided on the command line or in the settings
  if (parser.isSet("icon-theme") )
		QIcon::setThemeName(parser.value("icon-theme"));
	else
		if (diag_settings->useStartOptions() && diag_settings->getSetting("StartOptions", "use_icon_theme").toBool() )
			QIcon::setThemeName(diag_settings->getSetting("StartOptions", "icon_theme_name").toString() );
		else QIcon::setThemeName(INTERNAL_THEME);
			
	// data members
	stackedwidget = new QStackedWidget(this);
	videowidget = new VideoWidget(this);
	playlist = new Playlist(this); 
	gstiface = new GST_Interface(this);
	ncurs = this->cursor();
	hiatus_resume = -1;
	notifyclient = NULL;
	mpris2 = new Mpris2(this);
	pos_timer = new QTimer(this);
	albumart = new ArtWidget(this);
	
	// Create the notifyclient, make four tries; first immediately in constructor, then
  // at 1/2 second, 2 seconds and finally at 8 seconds
  notifyclient = new NotifyClient(this);
  this->connectNotifyClient();
  QTimer::singleShot(500, this, SLOT(connectNotifyClient()));
  QTimer::singleShot(2 * 1000, this, SLOT(connectNotifyClient()));
  QTimer::singleShot(8 * 1000, this, SLOT(connectNotifyClient()));
  
  // setup the user interface
  ui.setupUi(this);	
  ui.gridLayout->addWidget(stackedwidget, 0, 0);
  ui.gridLayout->setRowStretch(0,1);
  stackedwidget->addWidget(videowidget);
  stackedwidget->addWidget(playlist);
  stackedwidget->addWidget(albumart);

	// Set the style
	QString styl = diag_settings->getSetting("Preferences", "style").toString();
	styl.prepend(":/stylesheets/stylesheets/");
	styl.append(".qss");
	QFile f0(styl);
	if (f0.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QString qss = QString(f0.readAll());
		f0.close();
		qApp->setStyleSheet(qss);
	}
      	
	// Setup the icon manager and give it a color
	IconManager iconman(this);
	iconman.setIconColor(QColor(diag_settings->getSetting("Preferences", "colorize_icons").toString()) );
	
	// Set the window icon
	setWindowIcon(iconman.getIcon("mbmp") );
			
	// assign icons to actions
	ui.actionPlaylistNext->setIcon(iconman.getIcon("playlist_next"));
	ui.actionPlaylistBack->setIcon(iconman.getIcon("playlist_back"));
	ui.actionTogglePlaylist->setIcon(iconman.getIcon("toggle_playlist"));
	ui.actionPlaylistFirst->setIcon(iconman.getIcon("playlist_first"));
	ui.actionPlaylistLast->setIcon(iconman.getIcon("playlist_last"));	
	ui.actionAbout->setIcon(iconman.getIcon("help_about"));
	ui.actionVisualizer->setIcon(iconman.getIcon("visualizer"));
	ui.actionToggleStreamInfo->setIcon(iconman.getIcon("stream_info"));
	ui.actionAudioCD->setIcon(iconman.getIcon("cd_drive"));
	ui.actionDVD->setIcon(iconman.getIcon("dvd_drive"));
	ui.actionSeekBack10->setIcon(iconman.getIcon("back_10"));
	ui.actionSeekFrwd10->setIcon(iconman.getIcon("forward_10"));
	ui.actionSeekBack60->setIcon(iconman.getIcon("back_60"));
	ui.actionSeekFrwd60->setIcon(iconman.getIcon("forward_60"));
	ui.actionSeekBack600->setIcon(iconman.getIcon("back_600"));
	ui.actionSeekFrwd600->setIcon(iconman.getIcon("forward_600"));
	ui.actionPlayerStop->setIcon(iconman.getIcon("player_stop"));
	ui.actionVolumeDecreaseStep->setIcon(iconman.getIcon("volume_step_down"));
	ui.actionVolumeIncreaseStep->setIcon(iconman.getIcon("volume_step_up"));
	ui.actionQuit->setIcon(iconman.getIcon("quit"));
	ui.actionToggleGUI->setIcon(iconman.getIcon("toggle_gui"));
	ui.actionToggleShade->setIcon(iconman.getIcon("toggle_shade"));
	ui.actionToggleFullscreen->setIcon(iconman.getIcon("toggle_fullscreen"));
	ui.actionOptions->setIcon(iconman.getIcon("options"));
	ui.actionShowSettingsDialog->setIcon(iconman.getIcon("settings"));	
	ui.actionAddMedia->setIcon(iconman.getIcon("add_media"));
	ui.actionAVSync->setIcon(iconman.getIcon("av_sync"));
	ui.actionColorBalance->setIcon(iconman.getIcon("color_balance"));
	ui.actionPlayPause->setIcon(iconman.getIcon("playpause"));
	ui.actionToggleMute->setIcon(iconman.getIcon("mute"));
	
	// hide the buffering progress bar
	ui.progressBar_buffering->hide();	
	
	// set up an event filter 
	QList<QWidget*> childlist = ui.widget_control->findChildren<QWidget*>();
	childlist += playlist->findChildren<QWidget*>();
	for (int i = 0; i < childlist.count(); ++i) {
		childlist.at(i)->installEventFilter(this);
	}	
	
	// find the the optical drives, or at least the first five
	for (int i = 0; i < 5; ++i) {
		QFileInfo fi(QString("/dev/sr%1").arg(i));
		if (fi.exists() ) {
			ui.comboBox_audiocd->addItem(QString("/dev/sr%1").arg(i));
			ui.comboBox_dvd->addItem(QString("/dev/sr%1").arg(i));
		}
		else break;
	}	// for
	
	// set the CD combo box
	if (ui.comboBox_audiocd->count() == 0) { 
		ui.comboBox_audiocd->addItem(tr("None"));
		ui.comboBox_audiocd->setDisabled(true);
	}	// if
	else {
		QString s_cd = QString();
		if (parser.isSet("CD")) 
			s_cd = parser.value("CD");
		else if (diag_settings->useStartOptions() )
			s_cd = diag_settings->getSetting("StartOptions", "audio_cd_drive").toString();
		int idx = ui.comboBox_audiocd->findText(s_cd);
		ui.comboBox_audiocd->setCurrentIndex(idx > 0 ? idx : 0);
	}	// else
	
	//set the DVD combo box	
	if (ui.comboBox_dvd->count() == 0) { 
		ui.comboBox_dvd->addItem(tr("None"));
		ui.comboBox_dvd->setDisabled(true);
	}	// if
	else {
		QString s_dvd = QString();
		if (parser.isSet("DVD"))
			s_dvd = parser.value("DVD");
		else if (diag_settings->useStartOptions() )
			s_dvd = diag_settings->getSetting("StartOptions", "dvd_drive").toString();
		int idx = ui.comboBox_dvd->findText(s_dvd);
		ui.comboBox_dvd->setCurrentIndex(idx > 0 ? idx : 0);
	}	// else	
				
	// setup the logfile and the logging level
	logfile.setFileName("/tmp/mbmp.log");	// we don't provide an opportunity to change this
	if (logfile.exists() ) logfile.remove();	
	bool ok;
	loglevel = parser.value("loglevel").toInt(&ok,10);	// default is 1
	if (! ok) loglevel = 1;
	if (! parser.isSet("loglevel") && diag_settings->useStartOptions() )
		loglevel = diag_settings->getSetting("StartOptions", "log_level").toInt();
	if (loglevel < 0 ) loglevel = 0;
	if (loglevel > 4 ) loglevel = 4; 	
	b_logtofile = logfile.open(QIODevice::Append | QIODevice::Text);
					
	// setup the connection speed - note that gstreamer takes a guint64, but
	// the spinbox in the UI maxes out at quite a bit lower number.  Not sure
	// exaxtly what the units are supposed to be.  The one example I can find
	// on the internet uses a value of 56 which looks like an old dialup modem
	// to me.  If that is the case it seems kind of excessive to send a guint64.  
	quint64 cnxnspeed = 0;
	ok = false;
	if (parser.isSet("connection-speed") ) {
		cnxnspeed = parser.value("connection-speed").toInt(&ok,10); // default is 0				
	}
	else if (diag_settings->useStartOptions() ) {
		cnxnspeed = diag_settings->getSetting("StartOptions", "connection_speed").toInt();
		ok = true;
	}			
	if (ok) gstiface->changeConnectionSpeed(cnxnspeed);		
	
  // Assign actions defined in the UI to toolbuttons.  This also has the
  // effect of adding actions to this dialog so shortcuts work provided
  // the toolbutton is visible.  Since we can hide GUI in this widget
  // we need to both add the action and set the toolbar defaultActions.
  // If there is no toolbutton for the action then just add the action.
  this->addAction(ui.actionTogglePlaylist);	
	this->ui.toolButton_toggleplaylist->setDefaultAction(ui.actionTogglePlaylist);
	this->addAction(ui.actionToggleVideoWindow);
	this->addAction(ui.actionToggleAlbumArt);
	this->addAction(ui.actionToggleStreamInfo);
	this->addAction(ui.actionQuit);
	this->ui.toolButton_quit->setDefaultAction(ui.actionQuit);
	this->addAction(ui.actionToggleGUI);
	this->ui.toolButton_togglegui->setDefaultAction(ui.actionToggleGUI);
	this->addAction(ui.actionToggleShade);
	this->ui.toolButton_toggleshade->setDefaultAction(ui.actionToggleShade);
	this->addAction(ui.actionToggleFullscreen);
	this->ui.toolButton_fullscreen->setDefaultAction(ui.actionToggleFullscreen);
	this->addAction(ui.actionShowCheatsheet);
	this->addAction(ui.actionShowSettingsDialog);
	this->ui.toolButton_settings->setDefaultAction(ui.actionShowSettingsDialog);
	this->addAction(ui.actionAbout);
	this->addAction(ui.actionAboutMBMP);
	this->addAction(ui.actionAboutIconSet);
	this->addAction(ui.actionAboutQT);
	this->addAction(ui.actionShowLicense);
	this->addAction(ui.actionShowChangeLog);
	this->addAction(ui.actionPlayPause);
	this->ui.toolButton_playpause->setDefaultAction(ui.actionPlayPause);
	this->addAction(ui.actionPlaylistFirst);
	this->ui.toolButton_playlistfirst->setDefaultAction(ui.actionPlaylistFirst);
	this->addAction(ui.actionPlaylistBack);
	this->ui.toolButton_playlistback->setDefaultAction(ui.actionPlaylistBack);
	this->addAction(ui.actionPlaylistNext);
	this->ui.toolButton_playlistnext->setDefaultAction(ui.actionPlaylistNext);
	this->addAction(ui.actionPlaylistLast);
	this->ui.toolButton_playlistlast->setDefaultAction(ui.actionPlaylistLast);
	this->addAction(ui.actionAddMedia);
	this->addAction(ui.actionToggleMute);
	this->ui.toolButton_mute->setDefaultAction(ui.actionToggleMute);
	this->addAction(ui.actionVolumeDecreaseStep);
	this->addAction(ui.actionVolumeIncreaseStep);
	this->addAction(ui.actionVisualizer);
	this->addAction(ui.actionPlayerStop);
	this->ui.toolButton_stop->setDefaultAction(ui.actionPlayerStop);
	this->addAction(ui.actionSeekBack10);
	this->ui.toolButton_back10->setDefaultAction(ui.actionSeekBack10);
	this->addAction(ui.actionSeekFrwd10);
	this->ui.toolButton_frwd10->setDefaultAction(ui.actionSeekFrwd10);
	this->addAction(ui.actionSeekBack60);
	this->ui.toolButton_back60->setDefaultAction(ui.actionSeekBack60);
	this->addAction(ui.actionSeekFrwd60);
	this->ui.toolButton_frwd60->setDefaultAction(ui.actionSeekFrwd60);
	this->addAction(ui.actionSeekBack600);
	this->ui.toolButton_back600->setDefaultAction(ui.actionSeekBack600);	
	this->addAction(ui.actionSeekFrwd600);
	this->ui.toolButton_frwd600->setDefaultAction(ui.actionSeekFrwd600);
	this->addAction(ui.actionAdvancedMenu);
	this->addAction(ui.actionAVSync);
	this->addAction(ui.actionColorBalance);
	this->addAction(ui.actionAudioCD);
	this->ui.toolButton_cd->setDefaultAction(ui.actionAudioCD);
	this->addAction(ui.actionDVD);
	this->ui.toolButton_dvd->setDefaultAction(ui.actionDVD);
	this->addAction(ui.actionDVDBackOneMenu);
	this->addAction(ui.actionDVDTitleMenu);
	this->addAction(ui.actionDVDRootMenu);
	this->addAction(ui.actionDVDSubpictureMenu);
	this->addAction(ui.actionDVDAudioMenu);
	this->addAction(ui.actionDVDAngleMenu);
	this->addAction(ui.actionDVDChapterMenu);
	this->addAction(ui.actionDVDLeft);
	this->addAction(ui.actionDVDRight);
	this->addAction(ui.actionDVDUp);
	this->addAction(ui.actionDVDDown);
	this->addAction(ui.actionDVDActivate);
	this->addAction(ui.actionOptions);
	this->ui.toolButton_options->setDefaultAction(ui.actionOptions);
	
	// add actions to action groups
	playlist_group = new QActionGroup(this);	
	playlist_group->addAction(ui.actionPlaylistFirst);
	playlist_group->addAction(ui.actionPlaylistBack);
	playlist_group->addAction(ui.actionPlaylistNext);
	playlist_group->addAction(ui.actionPlaylistLast);	
	
	volume_group = new QActionGroup(this);
	volume_group->addAction(ui.actionVolumeDecreaseStep);
	volume_group->addAction(ui.actionVolumeIncreaseStep);
	
	seek_group = new QActionGroup(this);
	seek_group->addAction(ui.actionSeekBack10);
	seek_group->addAction(ui.actionSeekFrwd10);
	seek_group->addAction(ui.actionSeekBack60);
	seek_group->addAction(ui.actionSeekFrwd60);
	seek_group->addAction(ui.actionSeekBack600);
	seek_group->addAction(ui.actionSeekFrwd600);
	seek_group->setEnabled(false);
	
	dvd_group = new QActionGroup(this);
	dvd_group->addAction(ui.actionDVDBackOneMenu);
	dvd_group->addAction(ui.actionDVDTitleMenu);
	dvd_group->addAction(ui.actionDVDRootMenu);
	dvd_group->addAction(ui.actionDVDSubpictureMenu);
	dvd_group->addAction(ui.actionDVDAudioMenu);
	dvd_group->addAction(ui.actionDVDAngleMenu);
	dvd_group->addAction(ui.actionDVDChapterMenu);
	dvd_group->addAction(ui.actionDVDLeft);
	dvd_group->addAction(ui.actionDVDRight);
	dvd_group->addAction(ui.actionDVDUp);
	dvd_group->addAction(ui.actionDVDDown);
	dvd_group->addAction(ui.actionDVDActivate);
	
	stackedwidget_group = new QActionGroup(this);
	stackedwidget_group->addAction(ui.actionTogglePlaylist);
	stackedwidget_group->addAction(ui.actionToggleVideoWindow);
	stackedwidget_group->addAction(ui.actionToggleAlbumArt);
		
	// create the visualizer menu. It is tearoff enabled.
	vis_menu = new QMenu(this);
	vis_menu->setTitle(ui.actionVisualizer->text());
	vis_menu->setIcon(ui.actionVisualizer->icon());
	vis_menu->setTearOffEnabled(true);
	vis_group = new QActionGroup(this);	
	QList<QString> vislist = gstiface->getVisualizerList();
	for (int i = 0; i < vislist.size(); ++i) {
		QAction* act = vis_menu->addAction(vislist.at(i));
		act->setCheckable(true);
		vis_group->addAction(act);
		if (i == 0 ) act->setChecked(true);
		else if (act->text().contains(QRegExp("^GOOM: what a GOOM!$")) ) act->setChecked(true);
	}
	
	// create the advanced menu
	advanced_menu = new QMenu(this);
	advanced_menu->setTitle(ui.actionAdvancedMenu->text());
	advanced_menu->setIcon(ui.actionAdvancedMenu->icon());
	advanced_menu->addAction(ui.actionAVSync);
	advanced_menu->addAction(ui.actionColorBalance);
	advanced_menu->setDisabled(true);
	
	// create the options menu.  Actions are class members because we
	// need to access them in the slot changeOptions, but they are not
	// directly accessable from the ui (so they are not defined from there)
	bool b_01 = false;
	options_menu = new QMenu(this);
	options_menu->setTitle(ui.actionOptions->text());
	options_menu->setIcon(ui.actionOptions->icon());
	
	action_vis = options_menu->addAction(tr("Visualizer"));
	action_vis->setCheckable(true);		
	if (parser.isSet("visualizer") ) b_01 = true;
	else
		if (diag_settings->useStartOptions() && diag_settings->getSetting("StartOptions", "start_visualizer").toBool() ) b_01 = true;
		else b_01 = false;
	gstiface->setPlayFlag(GST_PLAY_FLAG_VIS, b_01 );
	action_vis->setChecked(b_01);
	vis_group->setEnabled(b_01);

	action_sub = options_menu->addAction(tr("Subtitles"));
	action_sub->setCheckable(true);
	if (parser.isSet("subtitles") ) b_01 = true;
	else
		if (diag_settings->useStartOptions() && diag_settings->getSetting("StartOptions", "start_subtitles").toBool() ) b_01 = true;
		else b_01 = false;		
	gstiface->setPlayFlag(GST_PLAY_FLAG_TEXT, b_01);
	action_sub->setChecked(b_01);
	
	action_sbuf = options_menu->addAction(tr("Stream Buffering"));
	action_sbuf->setCheckable(true);
	if (parser.isSet("stream-buffering") ) b_01 = true;
	else
		if (diag_settings->useStartOptions() && diag_settings->getSetting("StartOptions", "use_stream_buffering").toBool() ) b_01 = true;
		else b_01 = false;		
	gstiface->setPlayFlag(GST_PLAY_FLAG_BUFFERING, b_01);
	action_sbuf->setChecked(b_01);
		
	action_dbuf = options_menu->addAction(tr("Download Buffering"));
	action_dbuf->setCheckable(true);
	if (parser.isSet("download-buffering") ) b_01 = true;
	else
		if (diag_settings->useStartOptions() && diag_settings->getSetting("StartOptions", "use_download_buffering").toBool() ) b_01 = true;
		else b_01 = false;		
	gstiface->setPlayFlag(GST_PLAY_FLAG_DOWNLOAD, b_01);
	action_dbuf->setChecked(b_01);	

	// create the control_menu
	control_menu = new QMenu(this);
	control_menu->addAction(ui.actionTogglePlaylist);
	control_menu->addAction(ui.actionToggleVideoWindow);
	control_menu->addAction(ui.actionToggleAlbumArt);
	control_menu->addAction(ui.actionAddMedia);
	control_menu->addAction(ui.actionAudioCD);
	control_menu->addAction(ui.actionDVD);
	control_menu->addSeparator();
	control_menu->addMenu(vis_menu);
	control_menu->addAction(ui.actionToggleStreamInfo);
	control_menu->addSeparator();
	control_menu->addAction(ui.actionToggleGUI);
	control_menu->addAction(ui.actionToggleShade);
	control_menu->addAction(ui.actionToggleFullscreen);
	control_menu->addAction(ui.actionShowCheatsheet);
	control_menu->addMenu(advanced_menu);
	control_menu->addSeparator();	
	control_menu->addAction(ui.actionPlayPause);
	control_menu->addAction(ui.actionPlayerStop);
	control_menu->addSeparator();	
	control_menu->addAction(ui.actionSeekBack10);
	control_menu->addAction(ui.actionSeekBack60);
	control_menu->addAction(ui.actionSeekBack600);
	control_menu->addSeparator();	
	control_menu->addAction(ui.actionSeekFrwd10);
	control_menu->addAction(ui.actionSeekFrwd60);	
	control_menu->addAction(ui.actionSeekFrwd600);
	control_menu->addSeparator();	
	control_menu->addAction(ui.actionPlaylistFirst);
	control_menu->addAction(ui.actionPlaylistBack);	
	control_menu->addAction(ui.actionPlaylistNext);
	control_menu->addAction(ui.actionPlaylistLast);
	control_menu->addSeparator();	
	control_menu->addAction(ui.actionVolumeIncreaseStep);
	control_menu->addAction(ui.actionVolumeDecreaseStep);
	control_menu->addAction(ui.actionToggleMute);
	control_menu->addSeparator();
	control_menu->addMenu(options_menu);
	control_menu->addAction(ui.actionShowSettingsDialog);
	control_menu->addAction(ui.actionAbout);
	control_menu->addAction(ui.actionQuit);	
	
	// now assign the shortcuts to each action
	ShortCutManager scman(this);
	ui.actionTogglePlaylist->setShortcuts(scman.getKeySequence("cmd_playlist"));
	ui.actionToggleVideoWindow->setShortcuts(scman.getKeySequence("cmd_videowindow"));
	ui.actionToggleAlbumArt->setShortcuts(scman.getKeySequence("cmd_albumart"));
	ui.actionToggleStreamInfo->setShortcuts(scman.getKeySequence("cmd_streaminfo"));
	ui.actionQuit->setShortcuts(scman.getKeySequence("cmd_quit"));
	ui.actionToggleGUI->setShortcuts(scman.getKeySequence("cmd_gui"));
	ui.actionToggleShade->setShortcuts(scman.getKeySequence("cmd_shade"));
	ui.actionToggleFullscreen->setShortcuts(scman.getKeySequence("cmd_fullscreen"));
	ui.actionShowCheatsheet->setShortcuts(scman.getKeySequence("cmd_cheatsheet"));
	ui.actionAbout->setShortcuts(scman.getKeySequence("cmd_about"));
	ui.actionAboutMBMP->setShortcuts(scman.getKeySequence("cmd_aboutmbmp"));
	ui.actionAboutIconSet->setShortcuts(scman.getKeySequence("cmd_abouticonset"));
	ui.actionAboutQT->setShortcuts(scman.getKeySequence("cmd_aboutqt"));
	ui.actionShowLicense->setShortcuts(scman.getKeySequence("cmd_showlicense"));
	ui.actionShowChangeLog->setShortcuts(scman.getKeySequence("cmd_showchangelog"));
	ui.actionPlayPause->setShortcuts(scman.getKeySequence("cmd_playpause"));
	ui.actionPlaylistFirst->setShortcuts(scman.getKeySequence("cmd_playlistfirst"));
	ui.actionPlaylistBack->setShortcuts(scman.getKeySequence("cmd_playlistprev"));	
	ui.actionPlaylistNext->setShortcuts(scman.getKeySequence("cmd_playlistnext"));
	ui.actionPlaylistLast->setShortcuts(scman.getKeySequence("cmd_playlistlast"));
	ui.actionAddMedia->setShortcuts(scman.getKeySequence("cmd_addmedia"));
	ui.actionToggleMute->setShortcuts(scman.getKeySequence("cmd_togglemute"));
	ui.actionVolumeDecreaseStep->setShortcuts(scman.getKeySequence("cmd_voldec"));
	ui.actionVolumeIncreaseStep->setShortcuts(scman.getKeySequence("cmd_volinc"));
	ui.actionVisualizer->setShortcuts(scman.getKeySequence("cmd_visualizer"));
	ui.actionPlayerStop->setShortcuts(scman.getKeySequence("cmd_playerstop"));
	ui.actionSeekBack10->setShortcuts(scman.getKeySequence("cmd_seek_back_10"));
	ui.actionSeekFrwd10->setShortcuts(scman.getKeySequence("cmd_seek_frwd_10"));
	ui.actionSeekBack60->setShortcuts(scman.getKeySequence("cmd_seek_back_60"));
	ui.actionSeekFrwd60->setShortcuts(scman.getKeySequence("cmd_seek_frwd_60"));
	ui.actionSeekBack600->setShortcuts(scman.getKeySequence("cmd_seek_back_600"));
	ui.actionSeekFrwd600->setShortcuts(scman.getKeySequence("cmd_seek_frwd_600"));
	ui.actionAdvancedMenu->setShortcuts(scman.getKeySequence("cmd_advanced_menu"));
	ui.actionAVSync->setShortcuts(scman.getKeySequence("cmd_av_sync"));
	ui.actionColorBalance->setShortcuts(scman.getKeySequence("cmd_color_bal"));
	ui.actionAudioCD->setShortcuts(scman.getKeySequence("cmd_playaudiocd"));
	ui.actionDVD->setShortcuts(scman.getKeySequence("cmd_playdvd"));
	ui.actionDVDBackOneMenu->setShortcuts(scman.getKeySequence("cmd_dvd_back_one_menu"));
	ui.actionDVDTitleMenu->setShortcuts(scman.getKeySequence("cmd_dvd_title_menu"));
	ui.actionDVDRootMenu->setShortcuts(scman.getKeySequence("cmd_dvd_root_menu"));
	ui.actionDVDSubpictureMenu->setShortcuts(scman.getKeySequence("cmd_dvd_subpicture_menu"));
	ui.actionDVDAudioMenu->setShortcuts(scman.getKeySequence("cmd_dvd_audio_menu"));
	ui.actionDVDAngleMenu->setShortcuts(scman.getKeySequence("cmd_dvd_angle_menu"));
	ui.actionDVDChapterMenu->setShortcuts(scman.getKeySequence("cmd_dvd_chapter_menu"));
	ui.actionDVDLeft->setShortcuts(scman.getKeySequence("cmd_dvd_left"));
	ui.actionDVDRight->setShortcuts(scman.getKeySequence("cmd_dvd_right"));
	ui.actionDVDUp->setShortcuts(scman.getKeySequence("cmd_dvd_up"));
	ui.actionDVDDown->setShortcuts(scman.getKeySequence("cmd_dvd_down"));
	ui.actionDVDActivate->setShortcuts(scman.getKeySequence("cmd_dvd_activate"));
	ui.actionOptions->setShortcuts(scman.getKeySequence("cmd_options"));
	
	// radio button text and tooltips
	ui.radioButton_video->setText(scman.getKeySequence("cmd_videowindow").at(0).toString() );
	ui.radioButton_playlist->setText(scman.getKeySequence("cmd_playlist").at(0).toString() );
	ui.radioButton_albumart->setText(scman.getKeySequence("cmd_albumart").at(0).toString() );
	ui.radioButton_video->setToolTip(ui.actionToggleVideoWindow->toolTip() );
	ui.radioButton_playlist->setToolTip(ui.actionTogglePlaylist->toolTip() );
	ui.radioButton_albumart->setToolTip(ui.actionToggleAlbumArt->toolTip() );
	connect (ui.radioButton_video, SIGNAL (clicked()), ui.actionToggleVideoWindow, SLOT (trigger()));
	connect (ui.radioButton_playlist, SIGNAL (clicked()), ui.actionTogglePlaylist, SLOT (trigger()));
	connect (ui.radioButton_albumart, SIGNAL (clicked()), ui.actionToggleAlbumArt, SLOT (trigger()));
	ui.radioButton_video->setChecked(true);

  // setup the cheatsheet message box
	chtsht = new ScrollBox(this);
	chtsht->setWindowTitle(tr("Key Bindings"));
	chtsht->setDisplayText(scman.getCheatSheet());
	chtsht->setWindowModality(Qt::NonModal);	
		
  // connect signals to slots 
  connect (stackedwidget_group, SIGNAL (triggered(QAction*)), this, SLOT(advanceStackedWidget(QAction*)));	
  connect (ui.actionToggleStreamInfo, SIGNAL (triggered()), gstiface, SLOT(toggleStreamInfo()));
	connect (ui.actionQuit, SIGNAL (triggered()), qApp, SLOT(quit()));
	connect (ui.actionToggleGUI, SIGNAL (triggered()), this, SLOT(toggleGUI()));
	connect (ui.actionToggleShade, SIGNAL (triggered()), this, SLOT(toggleShadeMode()));
	connect (ui.actionToggleFullscreen, SIGNAL (triggered()), this, SLOT(toggleFullScreen()));
	connect (ui.actionShowCheatsheet, SIGNAL (triggered()), this, SLOT(toggleCheatsheet()));
	connect (ui.actionShowSettingsDialog, SIGNAL (triggered()), this, SLOT(toggleSettingsDialog()));
	connect (ui.actionAbout, SIGNAL (triggered()), this, SLOT(showAbout()));
	connect (ui.actionAboutMBMP, SIGNAL (triggered()), this, SLOT(aboutMBMP()));
	connect (ui.actionAboutIconSet, SIGNAL (triggered()), this, SLOT(aboutIconSet()));
	connect (ui.actionAboutQT, SIGNAL (triggered()), qApp, SLOT(aboutQt()));
	connect (ui.actionShowLicense, SIGNAL (triggered()), this, SLOT(showLicense())); 
	connect (ui.actionShowChangeLog, SIGNAL (triggered()), this, SLOT(showChangeLog()));
	connect (ui.actionPlayPause, SIGNAL (triggered()), this, SLOT(playPause()));
	connect (ui.toolButton_playpause, SIGNAL(toggled(bool)), options_menu, SLOT(setDisabled(bool)));
	connect (playlist_group, SIGNAL(triggered(QAction*)), this, SLOT(playMedia(QAction*)));
	connect (gstiface, SIGNAL(signalMessage(int, QString)), this, SLOT(processGstifaceMessages(int, QString)));
	connect (ui.actionAddMedia, SIGNAL (triggered()), playlist, SLOT(addMedia()));
	connect (ui.actionToggleMute, SIGNAL (triggered()), gstiface, SLOT(toggleMute())); 
	connect (volume_group, SIGNAL(triggered(QAction*)), this, SLOT(changeVolumeDialStep(QAction*)));
	connect (ui.dial_volume, SIGNAL(valueChanged(int)), this, SLOT(changeVolume(int)));
	connect (mpris2, SIGNAL(volumeChanged(int)), ui.dial_volume, SLOT(setValue(int)));	
	connect (ui.actionVisualizer, SIGNAL(triggered()), this, SLOT(popupVisualizerMenu()));
	connect (ui.actionOptions, SIGNAL(triggered()), this, SLOT(popupOptionsMenu()));
	connect (vis_menu, SIGNAL(triggered(QAction*)), this, SLOT(changeVisualizer(QAction*)));
	connect (ui.actionPlayerStop, SIGNAL(triggered()), this, SLOT(stopPlaying()));
	connect (seek_group, SIGNAL(triggered(QAction*)), this, SLOT(seekToPosition(QAction*)));
	connect (dvd_group, SIGNAL(triggered(QAction*)), this, SLOT(dvdNavigationCommand(QAction*)));
	connect (ui.actionAudioCD, SIGNAL (triggered()), this, SLOT(initializeCD()));
	connect (ui.actionDVD, SIGNAL (triggered()), this, SLOT(initializeDVD()));
	connect (videowidget, SIGNAL(navsignal(QString,int,int,int)), gstiface, SLOT(mouseNavEvent(QString,int,int,int)));
	connect (options_menu, SIGNAL(triggered(QAction*)), this, SLOT(changeOptions(QAction*)));
	connect (qApp, SIGNAL(aboutToQuit()), this, SLOT(cleanUp()));
	connect (pos_timer, SIGNAL(timeout()), this, SLOT(setPositionWidgets()));
	connect (playlist, SIGNAL(artworkRetrieved()), this, SLOT(artworkRetrieved()));
	
	connect (mpris2, SIGNAL(applicationStop()), qApp, SLOT(quit()));
	connect (mpris2, SIGNAL(controlStop()), ui.actionPlayerStop, SLOT(trigger()));
	connect (mpris2, SIGNAL(controlPlay()), this, SLOT(playMedia()));
	connect (mpris2, SIGNAL(controlPlayPause()), ui.actionPlayPause, SLOT(trigger()));
	connect (mpris2, SIGNAL(controlPause()), this, SLOT(mpris2Pause()));
	connect (mpris2, SIGNAL(controlSeek(qlonglong)), this, SLOT(mpris2Seek(qlonglong)));
	connect (mpris2, SIGNAL(playlistNext()), ui.actionPlaylistNext, SLOT(trigger()));
	connect (mpris2, SIGNAL(playlistBack()), ui.actionPlaylistBack, SLOT(trigger()));
	connect (playlist, SIGNAL(wrapModeChanged(bool)), mpris2, SLOT(setLoopStatus(bool)));
	connect (mpris2, SIGNAL(loopStatusChanged(bool)), playlist, SLOT(setWrapMode(bool)));
	connect (playlist, SIGNAL(randomModeChanged(bool)), mpris2, SLOT(setShuffle(bool)));
	connect (mpris2, SIGNAL(shuffleChanged(bool)), playlist, SLOT(setRandomMode(bool)));
	connect (mpris2, SIGNAL(controlToggleConsume()), playlist, SLOT(toggleConsumeMode()));
	connect (mpris2, SIGNAL(controlToggleWrap()), playlist, SLOT(toggleWrapMode()));
	connect (mpris2, SIGNAL(controlToggleRandom()), playlist, SLOT(toggleRandomMode()));
	connect (mpris2, SIGNAL(controlToggleDetail()), playlist, SLOT(toggleDetailMode()));
	connect (mpris2, SIGNAL(controlOpenUri(QString)), this, SLOT(mpris2OpenUri(QString)));
	
	// create actions to accept a selected few playlist and gstiface shortcuts
	QAction* pl_Act01 = new QAction(this);
	this->addAction(pl_Act01);
	pl_Act01->setShortcuts(scman.getKeySequence("cmd_addaudio") );
	connect (pl_Act01, SIGNAL(triggered()), playlist, SLOT(triggerAddAudio()));
	
	QAction* pl_Act02 = new QAction(this);
	this->addAction(pl_Act02);
	pl_Act02->setShortcuts(scman.getKeySequence("cmd_addvideo") );
	connect (pl_Act02, SIGNAL(triggered()), playlist, SLOT(triggerAddVideo()));
	
	QAction* pl_Act03 = new QAction(this);
	this->addAction(pl_Act03);
	pl_Act03->setShortcuts(scman.getKeySequence("cmd_addplaylist") );
	connect (pl_Act03, SIGNAL(triggered()), playlist, SLOT(triggerAddPlaylist()));
	
	QAction* pl_Act04 = new QAction(this);
	this->addAction(pl_Act04);
	pl_Act04->setShortcuts(scman.getKeySequence("cmd_addfile") );
	connect (pl_Act04, SIGNAL(triggered()), playlist, SLOT(triggerAddFiles()));	
	
	QAction* pl_Act05 = new QAction(this);
	this->addAction(pl_Act05);
	pl_Act05->setShortcuts(scman.getKeySequence("cmd_addurl") );
	connect (pl_Act05, SIGNAL(triggered()), playlist, SLOT(addURL()));	
	
	QAction* pl_Act06 = new QAction(this);
	this->addAction(pl_Act06);
	pl_Act06->setShortcuts(scman.getKeySequence("cmd_togglewrap") );
	connect (pl_Act06, SIGNAL(triggered()), playlist, SLOT(toggleWrapMode()));	
	
	QAction* pl_Act07 = new QAction(this);
	this->addAction(pl_Act07);
	pl_Act07->setShortcuts(scman.getKeySequence("cmd_toggleconsume") );
	connect (pl_Act07, SIGNAL(triggered()), playlist, SLOT(toggleConsumeMode()));	
	
	QAction* pl_Act08 = new QAction(this);
	this->addAction(pl_Act08);
	pl_Act08->setShortcuts(scman.getKeySequence("cmd_togglerandom") );
	connect (pl_Act08, SIGNAL(triggered()), playlist, SLOT(toggleRandomMode()));	
	
	QAction* pl_Act09 = new QAction(this);
	this->addAction(pl_Act09);
	pl_Act09->setShortcuts(scman.getKeySequence("cmd_toggledetail") );
	connect (pl_Act09, SIGNAL(triggered()), playlist, SLOT(toggleDetailMode()));	
	
	QAction* si_Act01 = new QAction(this);
	this->addAction(si_Act01);
	si_Act01->setShortcuts(scman.getKeySequence("cmd_cycleaudio") );
	connect (si_Act01, SIGNAL(triggered()), gstiface, SLOT(cycleAudioStream()));		
		
	QAction* si_Act02 = new QAction(this);
	this->addAction(si_Act02);
	si_Act02->setShortcuts(scman.getKeySequence("cmd_cyclevideo") );
	connect (si_Act02, SIGNAL(triggered()), gstiface, SLOT(cycleVideoStream()));
		
	QAction* si_Act03 = new QAction(this);
	this->addAction(si_Act03);
	si_Act03->setShortcuts(scman.getKeySequence("cmd_cyclesubtitle") );
	connect (si_Act03, SIGNAL(triggered()), gstiface, SLOT(cycleTextStream()));		

	//restore GUI elements
	if (diag_settings->useState()) {
		diag_settings->restoreElementGeometry("playerctl", this);
		ui.widget_control->setVisible(diag_settings->getSetting("State", "playerctl_gui").toBool() );
		
		int idx = diag_settings->getSetting("State", "playerctl_stackedwidget").toInt();
		stackedwidget->setCurrentIndex(idx);
		// ui.radioButton_video is already checked
		if (idx == stackedwidget->indexOf(playlist) ) ui.radioButton_playlist->setChecked(true);	
		if (idx == stackedwidget->indexOf(albumart) ) ui.radioButton_albumart->setChecked(true);
	}	// if useState
		
	// Options to start fullscreen, shade and gui. If both fullscreen and shade
	// are set then fullscreen takes precedence
	// Settings ->useState takes precedence over both
	else {
		ui.widget_control->setVisible(false);
		if (parser.isSet("gui") ) ui.widget_control->setVisible(true);
		else if (diag_settings->useStartOptions() && diag_settings->getSetting("StartOptions", "start_gui").toBool() ) ui.widget_control->setVisible(true);
		
		if (parser.isSet("fullscreen") ) this->toggleFullScreen();
			else if (parser.isSet("shademode") ) this->toggleShadeMode();
				else if (diag_settings->useStartOptions() && diag_settings->getSetting("StartOptions", "start_fullscreen").toBool() ) this->toggleFullScreen();
					else if (diag_settings->useStartOptions() && diag_settings->getSetting("StartOptions", "start_shademode").toBool() ) this->toggleShadeMode();
	}	// else not useState()
	
	// seed the playlist with the positional arguments from the command line
	if (parser.positionalArguments().count() > 0 )
		playlist->seedPlaylist(parser.positionalArguments() );
	else if (diag_settings->usePlaylist() ) {
		playlist->seedPlaylist(diag_settings->getPlaylist() );
		playlist->setCurrentRow(diag_settings->getSetting("Playlist", "current").toInt() );
		hiatus_resume = diag_settings->getSetting("Playlist", "position").toInt();
	}
	
	// adjust element ranks
	//gstiface->rankElement("avdec_mp3float", true);
	QString el;
	if (parser.isSet("promote") ) el = parser.value("promote");
  else if (diag_settings->useStartOptions() ) el = diag_settings->getSetting("StartOptions", "promoted-elements").toString();
	if (! el.isEmpty() ) {		// undocumented feature, we accept separator
		el.replace(',', ' ');		// characters of , ; or space.  We
    el.replace(';', ' ');		// only advertise the comma.
    el = el.simplified();
    el.replace(' ', ',');
    QStringList el_list = el.split(',');
    for (int i = 0; i < el_list.count(); ++i) {
			gstiface->rankElement(el_list.at(i), true);
		}
	}	// if
	
	el.clear();
	if (parser.isSet("blacklist") ) el = parser.value("blacklist");
  else if (diag_settings->useStartOptions() ) el = diag_settings->getSetting("StartOptions", "blacklisted-elements").toString();
	if (! el.isEmpty() ) {		// undocumented feature, we accept separator
		el.replace(',', ' ');		// characters of , ; or space.  We
    el.replace(';', ' ');		// only advertise the comma.
    el = el.simplified();
    el.replace(' ', ',');
    QStringList el_list = el.split(',');
    for (int i = 0; i < el_list.count(); ++i) {
			gstiface->rankElement(el_list.at(i), false);
		}
	}	// if	
	
	// wait 10ms (basically give the constructor time to end) and then
	// start the media playback
	QTimer::singleShot(10, this, SLOT(playMedia()));
	
}


////////////////////////////// Public Functions ////////////////////////////
//
// Function to set the stream position widgets. This will set both the text
// label and the slider. Called from this->pos_timer. Position
// is the stream position in seconds.
//
// Added feature, send a xscreensaver deactive command if the screensaver
// the option is selected in settings. 

void PlayerControl::setPositionWidgets()
{
	// return if we are not playing
	if (gstiface->getState() != GST_STATE_PLAYING) return;
	
	// variables 
	// gst stream position in nanoseconds
	// nano * 1000 = micro
	// micro * 1000 = milli
	// mili * 1000 = full seconds
	qint64 position = gstiface->queryStreamPosition();
	
	// position is zero or positive
	if (position >= 0 ) {
		QTime t(0,0,0);
		int pos = position / (1000 * 1000 * 1000);
		t = t.addSecs(pos);
		ui.label_position->setText(t.toString("HH:mm:ss") );
		ui.horizontalSlider_position->setSliderPosition(pos);	
		mpris2->setPosition(position);
	}
	// position is negative
	else {
		ui.label_position->setText("00:00:00");
		mpris2->setPosition(0);
	}
	
	// deactive xscreensaver, do this each time we process a new position (about twice a second)
	if (diag_settings->useDisableXSS() ) QProcess::execute("xscreensaver-command -deactivate");
	
	return;
}

//
// Slot to set the albumart widget when new artwork is retrieved
// Called from a playlist signal
void PlayerControl::artworkRetrieved()
{
	albumart->setInfo(playlist->getAlbumArt(), playlist->getCurrentTitle(), playlist->getCurrentArtist() );
	
	return;
}

////////////////////////////// Public Slots ////////////////////////////
//
//	Slot to change the volume in gstiface in response to the volume dial being changed
void PlayerControl::changeVolume(int vol)
{
	// Gstreamer volume ranges are doubles in the range of 0.0 to 10.0
	// 0.0 = mute, 1.0 = 100%, so 10.0 must be really loud.  The volume
	// scale is linear, and the default is 1.0  Our dial uses integers
	// and is set up on a range of 0-30, with 10 being 100%. Mpris2 is 
	// based on doubles range from 0.0 to 1.0,  The conversions:
	//	Vol%		GStreamer MBMP	mpris2
	//	  0%		0.0				0			0.00
	//	100%		1.0				10		0.33
	//	300%		3.0				30  	1.00 	This is the maximum we want to go	
	
	gstiface->changeVolume(static_cast<double>(vol) / 10.0);
	
	// let mpris2 know
	mpris2->setVolume(static_cast<double>(vol) / 30.0);
	
	return;
}

// Slot to initialize the Audio CD.  Called when ui.actonAudioCD is triggered
// Check the audio CD and if we can read it start playing.
void PlayerControl::initializeCD()
{	
	switch (gstiface->checkCD(ui.comboBox_audiocd->currentText()) ) {
		case MBMP_GI::NoCDPipe:
			QMessageBox::warning(this, tr("%1 - Warning").arg(APP),
				tr("<center><b>Unable to create a pipeline to read an audio CD.</b></center>"
				"<br>Additional information may be found in the log file.  "                       
				"Audio CD playback will be disabled."
				) );
			return;	
		case MBMP_GI::BadCDRead:
			QMessageBox::warning(this, tr("%1 - Warning").arg(APP),
				tr("<center><b>Unable to read the audio CD.</b></center>"                       
				"<br>Make sure the audio CD is in %1.  It may still be possible to play a different CD or to "                       
				"play this CD in a different drive.").arg(ui.comboBox_audiocd->currentText())
				);
			return;	
	}	// switch			

	// if we are here checkCD passed, start playing the CD. This will
	// send a TOC to the bus which will trigger filling in the tracklist.
	gstiface->playMedia(videowidget->winId(), "cdda://" );
				
	return;
}

// Slot to setup the DVD.  Called when ui.actionDVD is triggered
// For now just play it
void PlayerControl::initializeDVD()
{
	switch (gstiface->checkDVD(ui.comboBox_dvd->currentText()) ) {
		case MBMP_GI::NoDVDPipe:
			QMessageBox::warning(this, tr("%1 - Warning").arg(APP),
				tr("<center><b>Unable to read the DVD.</b></center>"                       
				"<br>Make sure the DVD is in %1 or try the DVD in a different drive").arg(ui.comboBox_dvd->currentText())
				);
			return;
		case MBMP_GI::BadDVDRead:		
			QMessageBox::warning(this, tr("%1 - Warning").arg(APP),
				tr("<center><b>Unable to read the DVD.</b></center>"                       
				"<br>Make sure the disk is in %1.  It may still be possible to play a different DVD or to "                       
				"play this DVD in a different drive.").arg(ui.comboBox_dvd->currentText())
				);
			return;	
	}	// switch
	
	if (! action_sub->isChecked() ) {
		QMessageBox::warning(this, tr("%1 - Warning").arg(APP),
		tr("<center><b>Subtitles not enabled.</b></center>"                       
		"<br>DVD on screen navigation will only show your selection if subtitles are enabled. Subtitles "				
		"are currently not enabled so you will not be able to see your selections, although selections "		
		"can still be made.")
		 );
	}	// if
	
	// if we are here checkDVD passed, disable the addmedia menu and
	gstiface->playMedia(videowidget->winId(), "dvd://" );
	
	return;
}

// Slot to query the playlist for the next media item, and then send
// it to gstiface to play it.  There are two ways into this, primary way
// is when ui.actionPlayPause is toggled.  It is also called from playlist
// when a playlist item is double clicked.  This bypasses the action so
// we need to keep the action in sync with setChecked
//
// Use this slot to trigger notifications which have to happen even if
// there are no tag information in the media stream (for instance changing
// the album art page, setting DPMS, etc) 
void PlayerControl::playMedia(QAction* act)
{
	// Figure out which way we want to go in the playlist
	int direction = MBMP_PL::Current;
	if (act == ui.actionPlaylistFirst)	direction = MBMP_PL::First;
	else if (act == ui.actionPlaylistBack )	direction = MBMP_PL::Previous;
		else if (act == ui.actionPlaylistNext ) 	direction = MBMP_PL::Next;
			else if (act == ui.actionPlaylistLast )	direction = MBMP_PL::Last;
	
	// Return if the playlist item has not changed
	if (! playlist->selectItem(direction) ) {
		ui.actionPlayPause->setChecked(false);
		this->stopPlaying();
		return;
	}
	
	// Make sure playpause is checked, a double click in the playlist will
	// come here directly so handle that case
	ui.actionPlayPause->setChecked(true);
		
	// If we are playing a CD send the track to gstiface.  This is only if
	// we change tracks in the playlist.  Initially playing is started directly
	// from initializeCD
	if (playlist->currentItemType() == MBMP_PL::ACD ) {
		gstiface->playMedia(videowidget->winId(), "cdda://", playlist->getCurrentSeq());
	}	// if we are playing a disk
	
	// If we are playing a DVD send the chapter to gstiface. This is only if
	// we change chapters in the playlist.  Initially playing is started directly
	// from initializeDVD
	else if (playlist->currentItemType() == MBMP_PL::DVD) {
		gstiface->playMedia(videowidget->winId(), "dvd://", playlist->getCurrentSeq());
	}
	
	// If we are playing a URL process it through youtube-dl if requested and if the URL has a default port
	// youtube-dl does not like URL's when the port is specified.
	else if (playlist->currentItemType() == MBMP_PL::Url) {
		if (diag_settings->useYouTubeDL() && QUrl::fromUserInput(playlist->getCurrentUri()).port() == -1 ) {
			QProcess p;
			p.start(QString("youtube-dl -g -f best %1").arg(playlist->getCurrentUri()) );
			if (p.waitForFinished(diag_settings->getYouTubeDLTimeout() * 1000) )	// timeout from settings
				gstiface->playMedia(videowidget->winId(), p.readAll());
			else {
				this->processGstifaceMessages(MBMP_GI::Info, tr("Failed processing %1 through youtube-dl. Skipping URL").arg(playlist->getCurrentUri()) );
				return;
			}	// else
		}	// if useYouTubeDL
		
		else {
			gstiface->playMedia(videowidget->winId(), playlist->getCurrentUri());	
		}	// else
	}	// else if

	else {
		// Get the window ID to render the media on and the next item in 
		// the playlist, then send both to gstiface to play the media
		if (playlist->currentIsPlayable() ) 
			gstiface->playMedia(videowidget->winId(), playlist->getCurrentUri());	
	}	// else
	
	// Set the stream volume to agree with the dial
	changeVolume(ui.dial_volume->value());
	
	// start the position timer
	pos_timer->start(500);
	
	// set the album art page.
	albumart->setInfo(playlist->getAlbumArt(), playlist->getCurrentTitle(), playlist->getCurrentArtist() );
	
	// clear mpris2 metadata
	mpris2->clearMetaData(); 
	
	// Turn of Display Power Message Signaling if requested
	Display* dpy = XOpenDisplay(NULL);
	DPMSInfo(dpy, &dpms_power_level, &dpms_state); 
	XGetScreenSaver(dpy, &xss_timeout_return, &xss_interval_return, &xss_prefer_blanking_return, &xss_allow_exposures_return);
	if (diag_settings->useDisableDPMS() ) {
		DPMSDisable(dpy);
		XSetScreenSaver(dpy, 0, 0, xss_prefer_blanking_return, xss_allow_exposures_return);
		XFlush(dpy);
	}	// if
					
	return;
}

//
// Slot to set ui elements back to where they need to be when the user
// chooses to stop the playback.  Also let GST_Interface know we want
// to to stop the playback.
void PlayerControl::stopPlaying()
{
	// sync the playpause action
	ui.actionPlayPause->setChecked(false);
	
	// bool if we are playing a disk
	bool b_disk = gstiface->currentIsDisk();
	
	// Let gstiface know about it, this will set the playerstate to NULL
	gstiface->playerStop();	
	
	// If we were playing a disk
	if (b_disk) {
		playlist->clearPlaylist();
		playlist->lockControls(false);
	}
		
	// Set window title and duration labels to zero, will also disable seek ui elements
	this->setDurationWidgets(-1);
	mpris2->setPosition(0);
	this->setWindowTitle(LONG_NAME);
	this->pos_timer->stop();
	
	// Reset and hide the buffering bar
	ui.progressBar_buffering->hide();
	ui.progressBar_buffering->setValue(0);
	
	// Restore Display Power Message Signaling
	if (playlist->getCurrentRow() < 0) return;
	Display* dpy = XOpenDisplay(NULL);
	dpms_state == 0 ? DPMSDisable(dpy) : DPMSEnable(dpy);
	XSetScreenSaver(dpy, xss_timeout_return, xss_interval_return, xss_prefer_blanking_return, xss_allow_exposures_return);
	XFlush(dpy);
	
	return;
}	

//
// Slot to jump to a stream position, called when a QAction is triggered
void PlayerControl::seekToPosition(QAction* act)
{
	// initial slider (and stream) position
	int pos = ui.horizontalSlider_position->sliderPosition();
	
	if (act == ui.actionSeekBack10) pos = pos - 10; 
	else if (act == ui.actionSeekFrwd10) pos = pos + 10;
		else if (act == ui.actionSeekBack60) pos = pos - 60; 
			else if (act == ui.actionSeekFrwd60) pos = pos + 60;
				else if (act == ui.actionSeekBack600) pos = pos - 600; 
					else if (act == ui.actionSeekFrwd600) pos = pos + 600;
	// move the slider to agree with the position				
	if (act != 0) ui.horizontalSlider_position->setSliderPosition(pos);
			
	if (pos < 0 ) pos = 0;
	if (pos > ui.horizontalSlider_position->maximum() ) pos = ui.horizontalSlider_position->maximum();	
	gstiface->seekToPosition(pos);
	
	// let mpris2 know about it
	mpris2->seeked(static_cast<qlonglong>(pos * 1000 * 1000));
	
	return;
}

//
// Slot to change the slider position, called from mpris2 seek
void PlayerControl::mpris2Seek(qlonglong offset)
{
	// offset is microseconds, convert to seconds for slider
	int pos = ui.horizontalSlider_position->sliderPosition() + static_cast<int>(offset / (1000 * 1000));
	
	// move and change stream
	if (pos < 0 ) pos = 0;
	if (pos > ui.horizontalSlider_position->maximum() ) pos = ui.horizontalSlider_position->maximum();
	gstiface->seekToPosition(pos);
	
	// let mpris2 know about it (even though we came here from mpris2)
	mpris2->seeked(static_cast<qlonglong>(pos * 1000 * 1000));
	
	return;
}

//
// Slot to process an mpris2 OpenUri signal
void PlayerControl::mpris2OpenUri(QString uri)
{
	playlist->addURI(uri);
	
	if (gstiface->getState() != GST_STATE_PLAYING)
		QTimer::singleShot(10, this, SLOT(playMedia()));
		
	return;
	}

//
// Slot to send a DVD navigation command to the stream.  Called when
// one of the actions assigned to dvd_group action group are triggered
void PlayerControl::dvdNavigationCommand(QAction* act)
{
	GstNavigationCommand cmd = GST_NAVIGATION_COMMAND_INVALID;
	if (act == ui.actionDVDBackOneMenu) cmd = GST_NAVIGATION_COMMAND_DVD_MENU;
	else if (act == ui.actionDVDTitleMenu) cmd = GST_NAVIGATION_COMMAND_DVD_TITLE_MENU;
		else if (act == ui.actionDVDRootMenu) cmd = GST_NAVIGATION_COMMAND_DVD_ROOT_MENU;
			else if (act == ui.actionDVDSubpictureMenu) cmd = GST_NAVIGATION_COMMAND_DVD_SUBPICTURE_MENU;
				else if (act == ui.actionDVDAudioMenu) cmd = GST_NAVIGATION_COMMAND_DVD_AUDIO_MENU;
					else if (act == ui.actionDVDAngleMenu) cmd = GST_NAVIGATION_COMMAND_DVD_ANGLE_MENU;
						else if (act == ui.actionDVDChapterMenu) cmd = GST_NAVIGATION_COMMAND_DVD_CHAPTER_MENU;
							else if (act == ui.actionDVDLeft) cmd = GST_NAVIGATION_COMMAND_LEFT;
								else if (act == ui.actionDVDRight) cmd = GST_NAVIGATION_COMMAND_RIGHT;
									else if (act == ui.actionDVDUp) cmd = GST_NAVIGATION_COMMAND_UP;
										else if (act == ui.actionDVDDown) cmd = GST_NAVIGATION_COMMAND_DOWN;
											else if (act == ui.actionDVDActivate) cmd = GST_NAVIGATION_COMMAND_ACTIVATE;
	
	gstiface->keyNavEvent(cmd);
	return;
}

//
// Slot to toggle pause and play on the media
void PlayerControl::playPause()
{
	if (gstiface->getState() == GST_STATE_NULL || gstiface->getState() == GST_STATE_READY )
		playMedia();
				
	else {
		if (gstiface->getState() == GST_STATE_PLAYING) seek_group->setEnabled(false);
		gstiface->playPause();
	}
	
	return;	
}

//
// Slot to pause the playback.  Only used from the mpris2 interface, which
// means it will probably never be used.  
void PlayerControl::mpris2Pause()
{
	if (gstiface->getState() == GST_STATE_PLAYING) {
			ui.actionPlayPause->trigger();
	}	
		
	return;
}

//
// Slot to advance the stacked widget to the specified page.  If the
// stackedwidget is already displaying the requested page toggle back
// to the previous page
// Called when something in the stackedWidgetGroup is triggered 
void PlayerControl::advanceStackedWidget(QAction* act)
{
	// store previous index to use for toggling
	static int previdx;
	
	// find the target index
	int targetidx = -1;
	if (act == ui.actionToggleVideoWindow) targetidx = stackedwidget->indexOf(videowidget);
		else if (act == ui.actionTogglePlaylist) targetidx = stackedwidget->indexOf(playlist);
			else if (act == ui.actionToggleAlbumArt) targetidx = stackedwidget->indexOf(albumart);
				else targetidx = stackedwidget->currentIndex();

	// toggle back to previous if act pointed to the current page
	if (targetidx == stackedwidget->currentIndex() ) {
		if (previdx >= 0) { 
			stackedwidget->setCurrentIndex(previdx);
			previdx = targetidx;
		}
	}	// if
	
	// else go to the requested page	
	else {
		previdx = stackedwidget->currentIndex();
		if (act == ui.actionTogglePlaylist)
			stackedwidget->setCurrentWidget(playlist);
			else if (act == ui.actionToggleAlbumArt)
				stackedwidget->setCurrentWidget(albumart);
				else if (act == ui.actionToggleVideoWindow)
					stackedwidget->setCurrentWidget(videowidget);
					else
						stackedwidget->setCurrentWidget(videowidget);			
		}	// else

	// sync the radio buttons 
	if (stackedwidget->currentIndex() == stackedwidget->indexOf(videowidget) )
			ui.radioButton_video->setChecked(true);
	else if (stackedwidget->currentIndex() == stackedwidget->indexOf(playlist) )	
			ui.radioButton_playlist->setChecked(true);
	else if (stackedwidget->currentIndex() == stackedwidget->indexOf(albumart) )
			ui.radioButton_albumart->setChecked(true); 

	return;
}

//
//	Slot to toggle full screen on and off
void PlayerControl::toggleFullScreen()
{
	static bool cheatsheetup = false;
	static QSize savedsize;
	static QPoint savedpoint;
	
	// If we're in shademode bring the stackedwidget back
	if (! stackedwidget->isVisible() ) this->toggleShadeMode();
	
	// Now do the toggle
	if (this->isFullScreen() ) {
		if (cheatsheetup) chtsht->show();
		this->showNormal();
		this->resize(savedsize);
		this->move(savedpoint);
		this->activateWindow();
		this->setCursor(ncurs);
		}
	else {
		cheatsheetup = chtsht->isVisible();
		savedsize = this->size();
		savedpoint = this->pos();
		if (cheatsheetup) chtsht->hide();
		if (vis_menu->isTearOffMenuVisible() ) vis_menu->hideTearOffMenu();	
		this->showFullScreen();
		this->setCursor(Qt::BlankCursor);
	}
	
	return;
}

//
//	Slot to toggle the GUI interface on and off
void PlayerControl::toggleGUI()
{
	ui.widget_control->isVisible() ? ui.widget_control->hide() : ui.widget_control->show();
	
	return;
}

//
// Slot to toggle shademode on and off
void PlayerControl::toggleShadeMode()
{
	static QSize savedsize;
	static QPoint savedpoint;
	
	// If we're in fullscreen mode go to normal mode
	if (this->isFullScreen() ) this->toggleFullScreen();
	
	// Now do the toggle
	if (stackedwidget->isVisible() ) {
		savedsize = this->size();
		savedpoint = this->pos();
		stackedwidget->setVisible(false);
		this->resize(this->sizeHint().width(), ui.widget_control->height());
	}
	else {
		stackedwidget->setVisible(true);
		this->resize(savedsize);
		this->move(savedpoint);
	}
		
	return;
}

//
// Slot to toggle the cheatsheet up or down
void PlayerControl::toggleCheatsheet()
{
	chtsht->isVisible() ? chtsht->hide() : chtsht->show();

	return;
}

//
// Slot to toggle the settings dialog up or down
void PlayerControl::toggleSettingsDialog()
{
	diag_settings->isVisible() ? diag_settings->hide() : diag_settings->show();

	return;
}

//
//	Slot to enter the About dialogs
void PlayerControl::showAbout()
{
	QMenu about_menu(this);

	about_menu.addAction(ui.actionAboutMBMP);
	about_menu.addAction(ui.actionAboutIconSet);
	about_menu.addAction(ui.actionAboutQT);
	about_menu.addSeparator();	
	about_menu.addAction(ui.actionShowLicense);	
	about_menu.addAction(ui.actionShowChangeLog);
	
	about_menu.exec(QCursor::pos() );
}

//
// slot to display an about box for this program
void PlayerControl::aboutMBMP()
{
 QMessageBox::about(this, tr("About %1").arg(APP),
      tr("<center>%1 is a GStreamer (1.0 series) media player."
		  "<br><center>Version <b>%2</b>"
                  "<center>Release date: %3"                        
                  "<center>Copyright c %4\n<center>by\n"
                  "<center>Andrew J. Bibb\n"
                  "<center>Vermont, USA"
									"<br><center><b>Translations:</b>"
                  "<center>Ilya Shestopalov (Russian)"                  
                  ).arg(QString(APP).toUpper()).arg(VERSION).arg(RELEASE_DATE).arg(COPYRIGHT_DATE) );
}

//
// slot to display an about box for the icons we used
void PlayerControl::aboutIconSet()
{
 QMessageBox::about(this, tr("About AwOken"),
       tr("<center>This program uses the <b>AwOken</b> icon set version 2.5"
          "<br><br>Released under the"
          "<br>Creative Commons"
          "<br>Attribution-Share Alike 3.0"
          "<br>Unported License"
          "<br><a href=\"url\">http://creativecommons.org/licenses/by-sa/3.0/legalcode</a>"
                  ) );
}

//
//	slot to display the program license
void PlayerControl::showLicense()
{
	QString s = readTextFile(":/text/text/license.txt");
  if ( s.isEmpty() ) s.append(tr("%1 license is the MIT license.").arg(APP));
	
	QMessageBox::about(this, tr("License"), s);
}

//
//	slot to display the change log of theprogram
void PlayerControl::showChangeLog()
{
	QString s = readTextFile(":/text/text/changelog.txt");
  if ( s.isEmpty() ) s.append(tr("%1 change log is not available.").arg(APP));
	
	ScrollBox::execScrollBox(tr("ChangeLog"), s, this);
}

////////////////////////////// Private Slots ////////////////////////////
//
// Slot to process the output from the gstreamer bus
// mtype should be an MBMP enum from the gstiface.h file
// msg is an optional qstring to display or send to a file
//
// Remember that when this is called the playlist may contain no items
// because CD's and DVD's are started directly.  For these types only 
// after the stream has started do we fill in the playlist items.
void PlayerControl::processGstifaceMessages(int mtype, QString msg)
{
	QTextStream stream1 (stdout);
	QTextStream stream2 (&logfile);

	switch (mtype) {
		case MBMP_GI::State:
		
			// restore stream after hiatus
			if (msg.contains(PLAYER_NAME, Qt::CaseSensitive) && msg.contains("PAUSED to PLAYING", Qt::CaseSensitive) && hiatus_resume >= 0 ) {
				gstiface->seekToPosition(hiatus_resume);
				hiatus_resume = -1;
			}
			// log message
			if (loglevel >= 1 && loglevel <= 2) {
				if (msg.contains(PLAYER_NAME, Qt::CaseSensitive)) {
					stream1 << msg << endl;
					if (b_logtofile) stream2 << msg << endl;
				}	// player name if
			}	// loglevel if	
			else if (loglevel >= 3) {	//output state change all elements	
				stream1 << msg << endl;
				if (b_logtofile) stream2 << msg << endl;				
			}	// loglevel else
			
			// process information and set widgets depending on state
			if (msg.contains(PLAYER_NAME, Qt::CaseSensitive) ) {
				// initialize things based on player state	
				if (msg.contains("PAUSED to PLAYING", Qt::CaseSensitive) ) {
					this->setDurationWidgets(gstiface->queryDuration() / (1000 * 1000 * 1000), gstiface->queryStreamSeek() ); 
				}	// if PAUSED to PLAYING
		
				// let mpris2 know about state changes	
				mpris2->setState(gstiface->getState() );
				mpris2->setCanSeek(gstiface->queryStreamSeek() );	
					
			}	// if PLAYER_NAME								
			
			break;
						
		case MBMP_GI::EOS:	// end of stream
			if (loglevel >= 3) {
				stream1 << msg << endl;
				if (b_logtofile) stream2 << msg << endl;
			}	// loglevel if
			
			ui.actionPlaylistNext->trigger();
			break;
			
		case MBMP_GI::SOS:	// start of stream
			if (loglevel >= 3) {
					stream1 << msg << endl;
					if (b_logtofile) stream2 << msg << endl;
				}	// loglevel if
			break;
			
		case MBMP_GI::Error:	// all errors printed regardless of loglevel
			stream1 << msg << endl;
			if (b_logtofile) stream2 << msg << endl;
			break;

		case MBMP_GI::Warning: 
			if (loglevel >= 1) {
				stream1 << msg << endl;
				if (b_logtofile) stream2 << msg << endl;
			} // loglevel if
			break;
		
		case MBMP_GI::Info:
			if (loglevel >= 2) {
				stream1 << msg << endl;
				if (b_logtofile) stream2 << msg << endl;
			}	// loglevel if
			break;
		
		case MBMP_GI::ClockLost: // message printed regardless of loglevel
			stream1 << msg << endl;
			if (b_logtofile) stream2 << msg << endl;
			break;
			
		case MBMP_GI::Application:	// a message we posted to the bus
			if (loglevel >= 2) {
				stream1 << "MBMP[Application]: " << msg << endl;
				if (b_logtofile) stream2 << "MBMP[Application]: " << msg << endl;
			}	// loglevel if
			break;
		
		case MBMP_GI::Buffering: // buffering messages
			static bool b_finished = true;
			
			if (loglevel >= 1 && loglevel <= 2) {	// only show message at end	
				if (b_finished) {	
					stream1 << "Buffering....." << endl;
					if (b_logtofile) stream2 << "Buffering" << endl;
				}	// if b_finisthed
			}	// loglevel if
			else if (loglevel >= 3) {		//show every message
				stream1 << "Buffering " << msg << "%" << endl;
				if (b_logtofile) stream2 << "Buffering " << msg << "%" << endl;
			}	// loglevel else
				
			// conversion should always work since the QString was created by QString::number
			// check the conversion just in case	
			bool ok;
			int pct;
			pct = msg.toInt(&ok, 10);	
			if (ok) {	
				if (pct < 100 ) {
					if (b_finished) ui.progressBar_buffering->show();
					ui.progressBar_buffering->setValue(pct);
					b_finished = false;
				}	// percent < 100
				else {
					ui.progressBar_buffering->hide();
					b_finished = true;
				}	// else percent is 100 (we're done)
			}	// if ok
			else b_finished = true;
			break;
			
		case MBMP_GI::Unhandled: // a GstBus message we didn't handle
			if (loglevel >= 2) {
				stream1 << msg << endl;
				if (b_logtofile) stream2 << msg << endl;
			}	// loglevel if
			break;	// Unhandled GstBus message		
		
		// only get Duration messages on CD track and DVD chapter changes
		// come through here.  Set file and url durations in the ::State case above
		case MBMP_GI::Duration:	// a new stream duration message 
			if (loglevel >= 3) {
				stream1 << msg << endl;
				if (b_logtofile) stream2 << msg << endl;
			}	// loglevel if
			setDurationWidgets(gstiface->queryDuration() / (1000 * 1000 * 1000), gstiface->queryStreamSeek() );
			break;			
		
		case MBMP_GI::TOC: // A generic TOC
			if (loglevel >= 3) {
				stream1 << msg << endl;
				if (b_logtofile) stream2 << msg << endl;
			}	// loglevel if
			break;
		
		// A TOC with new tracklist. We just log this, tracklist it sent to
		// the playlist in the NewMBID case
		case MBMP_GI::TOCTL: 
			if (loglevel >= 3) {
				stream1 << msg << endl;
				if (b_logtofile) stream2 << msg << endl;
			}	// loglevel if
			break;
			
		case MBMP_GI::Tag:	// a TAG message 				
			if (loglevel >= 3) {
				stream1 << msg << endl;
				if (b_logtofile) stream2 << msg << endl;
			}	// loglevel if					
			break;
		
		case MBMP_GI::NewMBID: // a new musicbrainz CD track id
		if (loglevel > 3) {
			stream1 << msg <<endl;
			if (b_logtofile) stream2 << msg << endl;
		}
		// send the tracklist to the playlist to create playlist entries
		playlist->addTracks(gstiface->getTrackList());	// seed playlist
		playlist->discIDChanged(gstiface->getMBDiscID() ); // update with Musicbrainz data if possible
		if (playlist->isHidden()) playlist->show();
		break;	
					
		case MBMP_GI::TagCL:	// a TAG message indicating a new dvd chapter count
			if (loglevel >= 4) {
				stream1 << msg << endl;
				if (b_logtofile) stream2 << msg << endl;
			}	// loglevel if
			playlist->addChapters(gstiface->getChapterCount() );
			playlist->lockControls(true);
			if (playlist->isHidden()) playlist->show();	
			break;
		
		case MBMP_GI::TagCC:	// a TAG message indicating a new dvd chapter
			if (loglevel >= 4) {
				stream1 << msg << endl;
				if (b_logtofile) stream2 << msg << endl;
			}	// loglevel if
			playlist->setCurrentChapter(gstiface->getCurrentChapter() );
			break;
		
		case MBMP_GI::NewTrack:	// a New Track signal was emitted 
			if (loglevel >= 4) {
				stream1 << msg << endl;
				if (b_logtofile) stream2 << msg << endl;
			}	// loglevel if	
			
			// Set the window title, notifications, and mpris2 data
			if (msg.isEmpty() ) 
				this->setWindowTitle(playlist->getWindowTitle());	
			else   
				this->setWindowTitle(msg);		
			
			this->processMediaInfo(msg);			
			break;		
		
		case MBMP_GI::StreamStatus:	// stream status message
			if (loglevel >= 3) {
				stream1 << msg << endl;
				if (b_logtofile) stream2 << msg << endl;
			}	// loglevel if
			break;
								
		default:	// should never be here so if we are we had best see the message
			stream1 << msg << endl;
			if (b_logtofile) stream2 << msg << endl;
			break;		
			
		}	// mtype switch

	return;
}

//
// Slot to change the the volume dial in response to a QAction being triggered.  
void PlayerControl::changeVolumeDialStep(QAction* act)
{
	if (act == ui.actionVolumeDecreaseStep) ui.dial_volume->triggerAction(QAbstractSlider::SliderSingleStepSub);
	if (act == ui.actionVolumeIncreaseStep) ui.dial_volume->triggerAction(QAbstractSlider::SliderSingleStepAdd);
}

//
// Slot to popup the visualizer menu, called when the visualizer action shortcut
// is triggered.  All menu entries will show disabled unless the program is 
// called with visualizer enabled.
void PlayerControl::popupVisualizerMenu()
{
	if (vis_menu->isTearOffMenuVisible() ) return;
	
	vis_menu->popup(QCursor::pos());
	
	return;
}

//
// Slot to popup the options menu, called when the options action shortcut
// is triggered
void PlayerControl::popupOptionsMenu()
{
	options_menu->popup(QCursor::pos());
	
	return;
}

//
// Slot to select or change the audio visualizer. Extract the visualizer
// name from the QAction and send it to gstiface. 
void PlayerControl::changeVisualizer(QAction* act)
{
		QString vis = act->text();
		gstiface->changeVisualizer(vis);
	
	return;
}

//
// Slot to change the player options.  
void PlayerControl::changeOptions(QAction* act)
{
	QString opt = act->text();
	
	if (act == action_vis) {
		gstiface->setPlayFlag(GST_PLAY_FLAG_VIS, act->isChecked() );
		vis_group->setEnabled(act->isChecked());
	}
	
	else if (act == action_sub) {
		gstiface->setPlayFlag(GST_PLAY_FLAG_TEXT, act->isChecked() );
	}
	
	else if (act == action_sbuf) {
		gstiface->setPlayFlag(GST_PLAY_FLAG_BUFFERING, act->isChecked() );
	}
	
	else if (act == action_dbuf) {
		gstiface->setPlayFlag(GST_PLAY_FLAG_DOWNLOAD, act->isChecked() );
	}	
	
	return;
}

//
// Slot to tidy up the place at close.  Called when the QApplication::aboutToQuit() signal is emitted
void PlayerControl::cleanUp()
{
	// stop playing
	playlist->saveSettings(ui.horizontalSlider_position->sliderPosition() );
	stopPlaying();
	
  // write settings
  diag_settings->saveElementGeometry("playerctl", true, this->size(), this->pos() );
  diag_settings->saveElementState("playerctl", "gui", ui.widget_control->isVisible() );
  diag_settings->saveElementState("playerctl", "stackedwidget", stackedwidget->currentIndex() );
  diag_settings->writeSettings();
  			
  // close b_logtofile			
	logfile.close();

  return;
}

// Slot to connect to the notification client. Called from QTimers to give time for the notification server
// to start up if this program is started automatically at boot.  We make four attempts at finding the
// notification server.  First is in the constructor of NotifyClient, following we call the connectToServer()
// function.
void PlayerControl::connectNotifyClient()
{
   //initialize the counter
   static short count = 0;
   ++count;

  if (count > 1 ) {
    // if we have a valid notifyclient return now
    if (notifyclient->isValid() )
      return;
    // else try to connect again
    else
      notifyclient->connectToServer();
  } // if count > 1

  // setup the notify server label if we were successful in finding and connecting to a server
  if (notifyclient->isValid() ) {
    QString name = notifyclient->getServerName().toLower();
    name = name.replace(0, 1, name.left(1).toUpper() );
    QString vendor = notifyclient->getServerVendor();
    vendor = vendor.replace(0, 1, vendor.left(1).toUpper() );
    QString lab = tr("%1 version %2 by %3 has been detected on this system.<p>This server supports desktop Notification Specification version %4")
      .arg(name)
      .arg(notifyclient->getServerVersion() )
      .arg(vendor)
      .arg(notifyclient->getServerSpecVersion() );
    diag_settings->setNotificationsConnected(lab);
  }
  // not successful, try again or abandon if counter is at limit
  else {
    if (count < 4) {
			diag_settings->setNotificationsTrying(tr("Attempt %1 of 4 looking for notification server.").arg(count) );
    } // try again
    else {
      diag_settings->setNotificationsFailed();
    } // else last time
  } // else we don't have a valid client.

  return;
}

//
// Function to set the stream duration stream label.  Called from gstiface
// when a new stream duration is found.  Int value sent as the argument
// is the stream duration in seconds.
void PlayerControl::setDurationWidgets(int duration, bool seek_enabled)
{	
	// duration is zero or positive
	if (duration >= 0 ) {
		QTime t(0,0,0);
		t = t.addSecs(duration);
		ui.label_duration->setText(t.toString("HH:mm:ss") );
		ui.horizontalSlider_position->setMaximum(duration);
		ui.horizontalSlider_position->setEnabled(seek_enabled);
		ui.horizontalSlider_position->setSingleStep(duration / 100 );
		ui.horizontalSlider_position->setPageStep(duration / 10);
		// we are playing a DVD enable seeking only after we've got a chapter
		if (gstiface->currentIsDVD() ) {
			if (gstiface->getCurrentChapter() > 0 ) seek_group->setEnabled(seek_enabled);
		}
		else 
			seek_group->setEnabled(seek_enabled);
	}
	// duration is negative, for instance we just stopped the stream,
	else {
		ui.label_duration->setText("00:00:00");
		ui.label_position->setText("00:00:00");
		ui.horizontalSlider_position->setMaximum(0);
		ui.horizontalSlider_position->setSliderPosition(0); 
		ui.horizontalSlider_position->setEnabled(false);
		seek_group->setEnabled(false);
	}
		
	return;	
}

////////////////////////////// Protected Functions ////////////////////////////
//
//	Create a context menu	
void PlayerControl::contextMenuEvent(QContextMenuEvent* e)
{
	control_menu->popup(e->globalPos());
}	


// Event filter used to filter out tooltip events if we don't want to see them
// and to catch events to the position slider. In eventFilters returning true
// eats the event, false passes on it.
bool PlayerControl::eventFilter(QObject* watched, QEvent* event)
{
	if (watched == ui.horizontalSlider_position && event->type() == QEvent::MouseButtonPress )
	{
		QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
		if (mouseEvent->button() == Qt::LeftButton) {
			ui.horizontalSlider_position->setValue(QStyle::sliderValueFromPosition(
				ui.horizontalSlider_position->minimum(),
				ui.horizontalSlider_position->maximum(),
				mouseEvent->x(),
				ui.horizontalSlider_position->width()));
				gstiface->seekToPosition(ui.horizontalSlider_position->value() );
			return true;
		}	// if left button
		else 
			return false;
	}	// if
	
	// Disable tooltips on control box Allow playlistitem tooltips, except disable
	// if the checkBox_showInfo is checked
	if (event->type() == QEvent::ToolTip) {

		if (watched->objectName().contains("qt_scrollarea_viewport") ) {
			if (playlist->findChild<QCheckBox*>("checkBox_showinfo")->isChecked() )
				return true;
			else
				return false;
		}

		if (diag_settings->useDisableTT() )
			return true;
		else	
			return false;
	} // event is a tooltip
  
	// not interested, pass it on
	return false;
}

////////////////////////////// Private Functions ////////////////////////////
//
//	Function to read the text contained in a file.  Input is a const char* to 
//	the text file.  The return value is a QString of the text. Since the return
//	value is a single string that implies the source file contains newlines,
//	breaks, and other formatting tags as necessary. 
QString PlayerControl::readTextFile(const char* textfile)
{
	QString rtnstring = QString();
		
	QFile file(textfile);
  if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QTextStream in(&file);
		while (!in.atEnd()) {
			rtnstring.append(in.readLine() );
		}	// while
	}	// if
	
	return rtnstring;
} 

//
// Function to process the media info (tags) from the playlist and
// select various pieces for display, etc.
//
// Called from processGstifaceMessages then a newtrack signal is emitted.
// This signal comes from an actual Gstreamer TAG bus message, and not
// all streams have tags.  Use this only to show information where we 
// think said information would come from a tag.
void PlayerControl::processMediaInfo(const QString& msg)
{
	// Don't show notifications if playing CD's or DVD's
	if (! gstiface->currentIsDisk() ) {
		if(diag_settings->useNotifications() ) {
			// collect some data
			qint16 duration = playlist->getCurrentDuration();
			QTime n(0,0,0);
			QTime t;
			t = n.addSecs(duration);
				
			// build the notification
			notifyclient->init();
			if (! msg.isEmpty() )
				notifyclient->setSummary(msg);
				
			if (playlist->getCurrentTitle().isEmpty() ) {
				if (msg.isEmpty() ) notifyclient->setSummary(playlist->getCurrentUri().section("//", 1, 1));
				else notifyclient->setSummary(msg);
			}
			else {
				notifyclient->setSummary(playlist->getCurrentTitle() );
				QString s;
				if (! playlist->getCurrentArtist().isEmpty() )
					s.append(tr("\nArtist: %1").arg(playlist->getCurrentArtist()) );
				if (duration > 0)
					s.append(tr("\nDuration: %1").arg(duration > (60 * 60) ? t.toString("h:mm:ss") : t.toString("mm:ss")) );
				notifyclient->setBody(s);
			}	// else have title
			notifyclient->setIcon("audio-x-generic");
			notifyclient->sendNotification();
		}	// if useNotifications
	}	// if media type we want notifications for
	
	// Pass information from to mpris2.  
	// Many of the mpris2 fields are lists of strings, while GStreamer tags are just
	// strings.  I'll convert the tags to stringlists, but I'm not going to parse 
	// them trying to break out space or comma separated fields.  Not worth the 
	// effort.
	bool ok = false;
	QVariantMap vmap;
		if (playlist->getCurrentRow() >= 0) {
			vmap["mpris:trackid"] = QVariant::fromValue(QDBusObjectPath(QString("/org/mbmp/Track/%1").arg(playlist->getCurrentRow())) );	
		
		if (playlist->getCurrentDuration() >= 0)
			vmap["mpris:length"] = QVariant::fromValue(static_cast<qlonglong>(playlist->getCurrentDuration() * 1000 * 1000) );	// mpris2 wants microseconds
		
		if (! playlist->getArtURL().isEmpty() )
			vmap["mpris:artUrl"] = QVariant::fromValue(playlist->getArtURL());
		
		if (! playlist->getCurrentTagAsString(GST_TAG_ALBUM).isEmpty() )
			vmap["xesam:album"] = QVariant::fromValue(playlist->getCurrentTagAsString(GST_TAG_ALBUM));
		
		if (! playlist->getCurrentTagAsString(GST_TAG_ALBUM_ARTIST).isEmpty() )
			vmap["xesam:albumArtist"] = QVariant::fromValue(QStringList(playlist->getCurrentTagAsString(GST_TAG_ALBUM_ARTIST)));		
		
		if (! playlist->getCurrentArtist().isEmpty() )
			vmap["xesam:artist"] = QVariant::fromValue(playlist->getCurrentArtist());
		
		if (! playlist->getCurrentTagAsString(GST_TAG_LYRICS).isEmpty() )
			vmap["xesam:asText"] = QVariant::fromValue(playlist->getCurrentTagAsString(GST_TAG_LYRICS));
		
		if (! playlist->getCurrentTagAsString(GST_TAG_BEATS_PER_MINUTE).isEmpty() ) {
			double bpm = (playlist->getCurrentTagAsString(GST_TAG_BEATS_PER_MINUTE)).toDouble(&ok);
			if (ok) {
				vmap["xesam:audioBPM"] = QVariant::fromValue(static_cast<int>(bpm));
			}	// if ok
		} // if bpm tag exists
		
		if (! playlist->getCurrentTagAsString(GST_TAG_COMMENT).isEmpty() )
			vmap["xesam:comment"] = QVariant::fromValue(QStringList(playlist->getCurrentTagAsString(GST_TAG_COMMENT)));	
		
		if (! playlist->getCurrentTagAsString(GST_TAG_COMPOSER).isEmpty() )
			vmap["xesam:composer"] = QVariant::fromValue(QStringList(playlist->getCurrentTagAsString(GST_TAG_COMPOSER)));			
		
		if (! playlist->getCurrentTagAsString(GST_TAG_ALBUM_VOLUME_NUMBER).isEmpty() ) {
			uint vn = (playlist->getCurrentTagAsString(GST_TAG_ALBUM_VOLUME_NUMBER)).toUInt(&ok);
			if (ok) {
				vmap["xesam:discNumber"] = QVariant::fromValue(vn);
			}	// if ok
		}	// if volume number tag exists
		
		if (! playlist->getCurrentTagAsString(GST_TAG_GENRE).isEmpty() )
			vmap["xesam:genre"] = QVariant::fromValue(QStringList(playlist->getCurrentTagAsString(GST_TAG_GENRE)));	
		
		if (! playlist->getCurrentTitle().isEmpty() )
			vmap["xesam:title"] = QVariant::fromValue(playlist->getCurrentTitle());
			
		if (! msg.isEmpty() )
			vmap["mbmp:track"] = QVariant::fromValue(msg);
		
		if (playlist->getCurrentSeq() >= 0)
			vmap["xesam:trackNumber"] = QVariant::fromValue(playlist->getCurrentSeq() );
		
		if (! playlist->getCurrentUri().isEmpty() )
			vmap["xesam:url"] = QVariant::fromValue(playlist->getCurrentUri());
	}	// if current row >= 0
	
	mpris2->setMetadata(vmap);
	
	// update mpris2 on previous, next, play, pause and seekable status
	mpris2->setCanGoNext(playlist->canGoNext());
	mpris2->setCanGoPrevious(playlist->canGoPrevious());
	mpris2->setCanPlay(playlist->currentIsPlayable());
	mpris2->setCanPause(playlist->currentIsPlayable());	
			
	return;
}
