
# include "playerctl.h"

# include <QtCore/QDebug>
# include <QLocale>
# include <QTimer>
# include <QFile>
# include <QTextStream>
# include <QFile>
# include <QRegExp>
# include <QTime>

# include "./code/playerctl/playerctl.h"	
# include "./code/resource.h"

PlayerControl::PlayerControl(const QCommandLineParser& parser, QWidget* parent) 
	: QDialog(parent)
{
	// set the Locale
	QLocale::setDefault(QLocale(QLocale::L_LANG, QLocale::L_COUNTRY));	
	
	// set the window title
	this->setWindowTitle(WINDOW_TITLE);
	
  // setup the user interface
  ui.setupUi(this);	
	ui.widget_control->setVisible(parser.isSet("gui"));
	if (parser.isSet("fullscreen") ) this->showFullScreen();
		
	// members
	keymap = new KeyMap(this);
	playlist = new Playlist(this); 
	playlist->hide();
	p_gstiface = new GST_Interface(this);
	ncurs = this->cursor();
		
	// setup the logfile and the logging level
	logfile.setFileName("/tmp/mbmp.log");	// we don't provide an opportunity to change this
	if (logfile.exists() ) logfile.remove();	
	bool ok;
	loglevel = parser.value("loglevel").toInt(&ok,10);
	if (! ok) loglevel = 1;
	
	// enable the visualizer if requested. 
	p_gstiface->setPlayFlag(GST_PLAY_FLAG_VIS, (parser.isSet("visualizer")) );
	
	// enable subtitles if requested
	p_gstiface->setPlayFlag(GST_PLAY_FLAG_TEXT, (parser.isSet("subtitles")) ); 
		
	// hide the buffering progress bar
	ui.progressBar_buffering->hide();
	
	// setup the connection speed
	qint64 cnxnspeed = parser.value("connection-speed").toInt(&ok,10);
	if (ok) p_gstiface->changeConnectionSpeed(cnxnspeed);

	// setup the cheatsheet message box
	chtsht = new QMessageBox(this);
	chtsht->setWindowTitle(tr("Key Bindings"));
	chtsht->setText(keymap->getCheatSheet());
	chtsht->setWindowModality(Qt::NonModal);	
	
	// seed the playlist with the positional arguments from the command line
	playlist->seedPlaylist(parser.positionalArguments() );
	
	/////////////// for testing if we want something to play
	//playlist->seedPlaylist(QStringList("/mnt/p-stor/sambashare/Media/Video/Pacific Rim Elbow Rocket Clip.webm"));
	
  // Assign actions defined in the UI to toolbuttons.  This also has the
  // effect of adding actions to this dialog so shortcuts work provided
  // the toolbutton is visible.  Since we can hide GUI in this widget
  // we need to both add the action and set the toolbar defaultActions.
  // If there is no toolbutton for the action then just add the action.
  this->addAction(ui.actionTogglePlaylist);	
	this->ui.toolButton_toggleplaylist->setDefaultAction(ui.actionTogglePlaylist);
	this->addAction(ui.actionToggleStreamInfo);
	this->addAction(ui.actionQuit);
	this->ui.toolButton_quit->setDefaultAction(ui.actionQuit);
	this->addAction(ui.actionToggleGUI);
	this->ui.toolButton_togglegui->setDefaultAction(ui.actionToggleGUI);
	this->addAction(ui.actionToggleFullscreen);
	this->ui.toolButton_fullscreen->setDefaultAction(ui.actionToggleFullscreen);
	this->addAction(ui.actionShowCheatsheet);
	this->addAction(ui.actionAbout);
	this->addAction(ui.actionAboutMBMP);
	this->addAction(ui.actionAboutNuvola);
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
	
	// create the visualizer menu. It is tearoff enabled.
	vis_menu = new QMenu(this);
	vis_menu->setTitle(ui.actionVisualizer->text());
	vis_menu->setIcon(ui.actionVisualizer->icon());
	vis_menu->setTearOffEnabled(true);
	QActionGroup* vis_group = new QActionGroup(this);	
	QList<QString> vislist = p_gstiface->getVisualizerList();
	for (int i = 0; i < vislist.size(); ++i) {
		QAction* act = vis_menu->addAction(vislist.at(i));
		act->setCheckable(true);
		vis_group->addAction(act);
		if (act->text().contains(QRegExp("^GOOM: what a GOOM!$")) ) act->setChecked(true);
	}
	vis_group->setEnabled(parser.isSet("visualizer"));
	
	// create the advanced menu
	advanced_menu = new QMenu(this);
	advanced_menu->setTitle(ui.actionAdvancedMenu->text());
	advanced_menu->setIcon(ui.actionAdvancedMenu->icon());
	advanced_menu->addAction(ui.actionAVSync);
	advanced_menu->addAction(ui.actionColorBalance);
	advanced_menu->setDisabled(true);
			
	// create the control_menu
	control_menu = new QMenu(this);
	control_menu->addAction(ui.actionPlayCD);
	control_menu->addAction(ui.actionTogglePlaylist);
	control_menu->addAction(ui.actionAddMedia);
	control_menu->addMenu(vis_menu);
	control_menu->addAction(ui.actionToggleStreamInfo);
	control_menu->addSeparator();
	control_menu->addAction(ui.actionToggleGUI);
	control_menu->addAction(ui.actionToggleFullscreen);
	control_menu->addAction(ui.actionShowCheatsheet);
	control_menu->addMenu(advanced_menu);
	control_menu->addSeparator();	
	control_menu->addAction(ui.actionPlayPause);
	control_menu->addAction(ui.actionPlayerStop);
	control_menu->addSeparator();	
	control_menu->addAction(ui.actionSeekBack10);
	control_menu->addAction(ui.actionSeekFrwd10);
	control_menu->addAction(ui.actionSeekBack60);
	control_menu->addAction(ui.actionSeekFrwd60);	
	control_menu->addAction(ui.actionSeekBack600);
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
	control_menu->addAction(ui.actionAbout);
	control_menu->addAction(ui.actionQuit);
	
	// now assign the shortcuts to each action
	ui.actionTogglePlaylist->setShortcuts(getShortcuts("cmd_playlist"));
	ui.actionToggleStreamInfo->setShortcuts(getShortcuts("cmd_streaminfo"));
	ui.actionQuit->setShortcuts(getShortcuts("cmd_quit"));
	ui.actionToggleGUI->setShortcuts(getShortcuts("cmd_gui"));
	ui.actionToggleFullscreen->setShortcuts(getShortcuts("cmd_fullscreen"));
	ui.actionShowCheatsheet->setShortcuts(getShortcuts("cmd_cheatsheet"));
	ui.actionAbout->setShortcuts(getShortcuts("cmd_about"));
	ui.actionAboutMBMP->setShortcuts(getShortcuts("cmd_aboutmbmp"));
	ui.actionAboutNuvola->setShortcuts(getShortcuts("cmd_aboutnuvola"));
	ui.actionAboutQT->setShortcuts(getShortcuts("cmd_aboutqt"));
	ui.actionShowLicense->setShortcuts(getShortcuts("cmd_showlicense"));
	ui.actionShowChangeLog->setShortcuts(getShortcuts("cmd_showchangelog"));
	ui.actionPlayPause->setShortcuts(getShortcuts("cmd_playpause"));
	ui.actionPlaylistFirst->setShortcuts(getShortcuts("cmd_playlistfirst"));
	ui.actionPlaylistBack->setShortcuts(getShortcuts("cmd_playlistprev"));	
	ui.actionPlaylistNext->setShortcuts(getShortcuts("cmd_playlistnext"));
	ui.actionPlaylistLast->setShortcuts(getShortcuts("cmd_playlistlast"));
	ui.actionAddMedia->setShortcuts(getShortcuts("cmd_addmedia"));
	ui.actionToggleMute->setShortcuts(getShortcuts("cmd_togglemute"));
	ui.actionVolumeDecreaseStep->setShortcuts(getShortcuts("cmd_voldec"));
	ui.actionVolumeIncreaseStep->setShortcuts(getShortcuts("cmd_volinc"));
	ui.actionVisualizer->setShortcuts(getShortcuts("cmd_visualizer"));
	ui.actionPlayerStop->setShortcuts(getShortcuts("cmd_playerstop"));
	ui.actionSeekBack10->setShortcuts(getShortcuts("cmd_seek_back_10"));
	ui.actionSeekFrwd10->setShortcuts(getShortcuts("cmd_seek_frwd_10"));
	ui.actionSeekBack60->setShortcuts(getShortcuts("cmd_seek_back_60"));
	ui.actionSeekFrwd60->setShortcuts(getShortcuts("cmd_seek_frwd_60"));
	ui.actionSeekBack600->setShortcuts(getShortcuts("cmd_seek_back_600"));
	ui.actionSeekFrwd600->setShortcuts(getShortcuts("cmd_seek_frwd_600"));
	ui.actionAdvancedMenu->setShortcuts(getShortcuts("cmd_advanced_menu"));
	ui.actionAVSync->setShortcuts(getShortcuts("cmd_av_sync"));
	ui.actionColorBalance->setShortcuts(getShortcuts("cmd_color_bal"));
	
  // connect signals to slots 
  connect (ui.actionTogglePlaylist, SIGNAL (triggered()), this, SLOT(togglePlaylist()));	
  connect (ui.actionToggleStreamInfo, SIGNAL (triggered()), p_gstiface, SLOT(toggleStreamInfo()));
	connect (ui.actionQuit, SIGNAL (triggered()), qApp, SLOT(quit()));
	connect (ui.actionToggleGUI, SIGNAL (triggered()), this, SLOT(toggleGUI()));
	connect (ui.actionToggleFullscreen, SIGNAL (triggered()), this, SLOT(toggleFullScreen()));
	connect (ui.actionShowCheatsheet, SIGNAL (triggered()), this->chtsht, SLOT(show()));
	connect (ui.actionAbout, SIGNAL (triggered()), this, SLOT(showAbout()));
	connect (ui.actionAboutMBMP, SIGNAL (triggered()), this, SLOT(aboutMBMP()));
	connect (ui.actionAboutNuvola, SIGNAL (triggered()), this, SLOT(aboutNuvola()));
	connect (ui.actionAboutQT, SIGNAL (triggered()), qApp, SLOT(aboutQt()));
	connect (ui.actionShowLicense, SIGNAL (triggered()), this, SLOT(showLicense())); 
	connect (ui.actionShowChangeLog, SIGNAL (triggered()), this, SLOT(showChangeLog()));
	connect (ui.actionPlayPause, SIGNAL (triggered()), this, SLOT(playPause()));
	connect (playlist_group, SIGNAL(triggered(QAction*)), this, SLOT(playMedia(QAction*)));
	connect (p_gstiface, SIGNAL(busMessage(int, QString)), this, SLOT(processBusMessages(int, QString)));
	connect (ui.actionAddMedia, SIGNAL (triggered()), playlist, SLOT(addMedia()));
	connect (ui.actionToggleMute, SIGNAL (triggered()), p_gstiface, SLOT(toggleMute())); 
	connect (volume_group, SIGNAL(triggered(QAction*)), this, SLOT(changeVolumeDialStep(QAction*)));
	connect (ui.dial_volume, SIGNAL(valueChanged(int)), this, SLOT(changeVolume(int)));	
	connect (ui.actionVisualizer, SIGNAL(triggered()), this, SLOT(popupVisualizerMenu()));
	connect (vis_menu, SIGNAL(triggered(QAction*)), this, SLOT(changeVisualizer(QAction*)));
	connect (ui.actionPlayerStop, SIGNAL(triggered()), p_gstiface, SLOT(playerStop()));
	connect (ui.actionPlayerStop, SIGNAL(triggered(bool)), ui.toolButton_playpause, SLOT(setChecked(bool)));
	connect (ui.horizontalSlider_position, SIGNAL(sliderMoved(int)), p_gstiface, SLOT(seekToPosition(int)));
	connect (seek_group, SIGNAL(triggered(QAction*)), this, SLOT(seekToPosition(QAction*)));
	connect (ui.actionPlayCD, SIGNAL (triggered()), p_gstiface, SLOT(playCD()));
	
	// create actions to accept a selected few playlist shortcuts
	QAction* pl_Act01 = new QAction(this);
	this->addAction(pl_Act01);
	pl_Act01->setShortcuts(getShortcuts("cmd_addaudio") );
	connect (pl_Act01, SIGNAL(triggered()), playlist, SLOT(triggerAddAudio()));
	
	QAction* pl_Act02 = new QAction(this);
	this->addAction(pl_Act02);
	pl_Act02->setShortcuts(getShortcuts("cmd_addvideo") );
	connect (pl_Act02, SIGNAL(triggered()), playlist, SLOT(triggerAddVideo()));
	
	QAction* pl_Act03 = new QAction(this);
	this->addAction(pl_Act03);
	pl_Act03->setShortcuts(getShortcuts("cmd_addfile") );
	connect (pl_Act03, SIGNAL(triggered()), playlist, SLOT(triggerAddFiles()));
	
	QAction* pl_Act04 = new QAction(this);
	this->addAction(pl_Act04);
	pl_Act04->setShortcuts(getShortcuts("cmd_addurl") );
	connect (pl_Act04, SIGNAL(triggered()), playlist, SLOT(addURL()));	
	
	QAction* pl_Act05 = new QAction(this);
	this->addAction(pl_Act05);
	pl_Act05->setShortcuts(getShortcuts("cmd_cycleaudio") );
	connect (pl_Act05, SIGNAL(triggered()), p_gstiface, SLOT(cycleAudioStream()));		
		
	QAction* pl_Act06 = new QAction(this);
	this->addAction(pl_Act06);
	pl_Act06->setShortcuts(getShortcuts("cmd_cyclevideo") );
	
	
	QAction* pl_Act07 = new QAction(this);
	this->addAction(pl_Act07);
	pl_Act07->setShortcuts(getShortcuts("cmd_cyclesubtitle") );
	connect (pl_Act07, SIGNAL(triggered()), p_gstiface, SLOT(cycleTextStream()));		
		
	// wait 10ms (basically give the constructor time to end) and then
	// start the media playback
	QTimer::singleShot(10, this, SLOT(playMedia()));
 }


////////////////////////////// Public Functions ////////////////////////////
// 
// Function to get a list of shortcuts from the keymap.  Needed only
// for access from other classes that have this as a parent (for
// instance class "playlist".  Otherwise we could just call keymap->getKeySequence
// from here. 
QList<QKeySequence> PlayerControl::getShortcuts(const QString& cmd)
{
	return keymap->getKeySequence(cmd);
}

//
// Function to set the stream duration stream label.  Called from p_gstiface
// when a new stream duration is found.  Int value sent as the argument
// is the stream duration in seconds.
void PlayerControl:: setDurationWidgets(int duration, bool seek_enabled)
{
	// duration is zero or positive
	if (duration >= 0 ) {
		QTime t(0,0,0);
		t = t.addSecs(duration);
		ui.label_duration->setText(t.toString("HH:mm:ss") );
		ui.horizontalSlider_position->setMaximum(duration);
		ui.horizontalSlider_position->setEnabled(seek_enabled);
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

//
// Function to set the stream position.  This will set both the text
// label and the slider. Called from p_gstiface at the beginning of
// pollGstBus before any other bus message is processed. Position
// is the stream position in seconds.
void PlayerControl::setPositionWidgets(int position)
{
	// position is zero or positive
	if (position >= 0 ) {
		QTime t(0,0,0);
		t = t.addSecs(position);
		ui.label_position->setText(t.toString("HH:mm:ss") );
		ui.horizontalSlider_position->setSliderPosition(position);
	}
	// position is negative
	else {
		ui.label_position->setText("00:00:00");
	}
	
	return;
}

////////////////////////////// Public Slots ////////////////////////////
//
//	Slot to change the volume in p_gstiface in response to the volume dial being changed
void PlayerControl::changeVolume(int vol)
{
	// Gstreamer volume ranges are doubles in the range of 0.0 to 10.0
	// 0.0 = mute, 1.0 = 100%, so 10.0 must be really loud.  The volume
	// scale is linear, and the default is 1.0  Our dial uses integers
	// and is set up on a range of 0-20, with 10 being 100%. The conversions:
	//	Vol%		GStreamer MBMP
	//	  0%		0.0				0
	//	100%		1.0				10
	//	200%		2.0				20  This is the maximum we want to go
	
	double d_vol = 0.0;
	d_vol = (static_cast<double>(vol) / 10.0);
	
	p_gstiface->changeVolume(d_vol);
	
	return;		  
}


// Slot to query the playlist for the next media item, and then send
// it to p_gstiface to play it.
void PlayerControl::playMedia(QAction* act)
{
	// Figure out which way we want to go in the playlist
	int direction = MBMP::Current;
	if (act == ui.actionPlaylistFirst)	direction = MBMP::First;
	else if (act == ui.actionPlaylistBack )	direction = MBMP::Previous;
		else if (act == ui.actionPlaylistNext ) 	direction = MBMP::Next;
			else if (act == ui.actionPlaylistLast )	direction = MBMP::Last;
	
	// Get the media from the playlist
	QString media = playlist->getItem(direction);
	
	// Return if there is no media to play
	if (media.isEmpty() ) return;
	
	// Get the window ID to render the media on and the next item in 
	// the playlist, then send both to p_gstiface to play the media
	WId winId = ui.frame_player->winId();
	p_gstiface->playMedia(winId, media);

	// Set the stream volume to agree with the dial
	changeVolume(ui.dial_volume->value()); 
	
	return;
}

//
// Slot to jump to a stream position, called when a QAction is triggered
void PlayerControl::seekToPosition(QAction* act)
{
	int pos = 0;
	if (act == ui.actionSeekBack10) pos = ui.horizontalSlider_position->sliderPosition() - 10; 
	else if (act == ui.actionSeekFrwd10) pos = ui.horizontalSlider_position->sliderPosition() + 10;
		else if (act == ui.actionSeekBack60) pos = ui.horizontalSlider_position->sliderPosition() - 60; 
			else if (act == ui.actionSeekFrwd60) pos = ui.horizontalSlider_position->sliderPosition() + 60;
				else if (act == ui.actionSeekBack600) pos = ui.horizontalSlider_position->sliderPosition() - 600; 
					else if (act == ui.actionSeekFrwd600) pos = ui.horizontalSlider_position->sliderPosition() + 600;
	
	if (pos < 0 ) pos = 0;
	if (pos > ui.horizontalSlider_position->maximum() ) pos = ui.horizontalSlider_position->maximum();
	
	p_gstiface->seekToPosition(pos);
	
	return;
}

// Slot to toggle pause and play on the media
void PlayerControl::playPause()
{
	if (p_gstiface->getState() == GST_STATE_NULL || p_gstiface->getState() == GST_STATE_READY )
		playMedia();
		
	else {
		if (p_gstiface->getState() == GST_STATE_PLAYING) seek_group->setEnabled(false);
		p_gstiface->playPause();
	}
		
}

//
//	Slot to toggle full screen on and off
void PlayerControl::toggleFullScreen()
{
	static bool playlistup = false;
	static bool cheatsheetup = false;
	
	if (this->isFullScreen() ) {
		if (playlistup) playlist->show();
		if (cheatsheetup) chtsht->show();
		this->showNormal();
		this->activateWindow();
		this->setCursor(ncurs);
		}
	else {
		playlistup = playlist->isVisible();
		cheatsheetup = chtsht->isVisible();
		if (playlistup) playlist->hide();
		if (cheatsheetup) chtsht->hide();
		if (vis_menu->isTearOffMenuVisible() ) vis_menu->hideTearOffMenu();	
		this->showFullScreen();
		this->setCursor(Qt::BlankCursor);
	}
	
	return;
}

//
//	Slot to toggle the playlist up and down
void PlayerControl::togglePlaylist()
{
	playlist->isVisible() ? playlist->hide() : playlist->show();
	
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
//	Slot to enter the About dialogs
void PlayerControl::showAbout()
{
	QMenu about_menu(this);

	about_menu.addAction(ui.actionAboutMBMP);
	about_menu.addAction(ui.actionAboutNuvola);
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
 QMessageBox::about(this, tr("About %1").arg(PROGRAM_NAME),
      tr("<center>%1 is a GStreamer (1.0 series) media player."
		  "<br><center>Version <b>%2</b>"
                  "<center>Release date: %3"                        
                  "<center>Copyright c %4\n<center>by\n"
                  "<center>Andrew J. Bibb\n"
                  "<center>Vermont, USA").arg(PROGRAM_NAME).arg(VERSION).arg(RELEASE_DATE).arg(COPYRIGHT_DATE) );
}

//
// slot to display an about box for the icons we used
void PlayerControl::aboutNuvola()
{
 QMessageBox::about(this, tr("About Nuvola"),
       tr("<center>This program uses the version 1.0 <b>Nuvola</b> icon set"                       
                  "<br>copyright c 2004<br>by"
                  "<br>David Vignoni"
                  "<br>Released under the"
		  "<br>GNU Lesser General Public License, version 2.1"
                  ) );
}

//
//	slot to display the program license
void PlayerControl::showLicense()
{
	QString s = readTextFile(":/text/text/license.txt");
  if ( s.isEmpty() ) s.append(tr("%1 license is the MIT license.").arg(PROGRAM_NAME));
	
	QMessageBox::about(this, tr("License"), s);
}

//
//	slot to display the change log of theprogram
void PlayerControl::showChangeLog()
{
	QString s = readTextFile(":/text/text/changelog.txt");
  if ( s.isEmpty() ) s.append(tr("%1 change log is not available.").arg(PROGRAM_NAME));
	
	QMessageBox::about(this, tr("Change Log"), s);
}

////////////////////////////// Private Slots ////////////////////////////
//
//	Slot to process the output from the gstreamer bus
// 	mtype should be an MBMP enum from the gstiface.h file
//	msg is an optional qstring to display or send to a file
void PlayerControl::processBusMessages(int mtype, QString msg)
{
	const bool logtofile = logfile.open(QIODevice::Append | QIODevice::Text);
	QTextStream stream1 (stdout);
	QTextStream stream2 (&logfile);
	
	switch (mtype) {
		case MBMP::State: 
			switch (loglevel) {
				case 0:		// case 0 - surpress state change output
					break;	
				case 	1:	// case 1 - only output player state changes 
					if (msg.contains(PLAYER_NAME, Qt::CaseSensitive)) {
						stream1 << msg << endl;
						if (logtofile) stream2 << msg << endl;
					}	// if PLAYER_NAME	
					break;
				default: // default (case 2 and above) - output everything	
					stream1 << msg << endl;
					if (logtofile) stream2 << msg << endl;			
			}	// loglevel switch
			break;
						
		case MBMP::EOS:
			switch (loglevel) {
				case 0:		// case 0 supress EOS output
					break;	
				default: // otherwise print EOS output
					stream1 << msg << endl;
					if (logtofile) stream2 << msg << endl;
				}	// loglevel switch
			ui.actionPlaylistNext->trigger();
			break;
			
		case MBMP::SOS:
			switch (loglevel) {
				case 0:		// case 0 supress SOS output
					break;	
				default: // otherwise print SOS output
					stream1 << msg << endl;
					if (logtofile) stream2 << msg << endl;
				}	// loglevel switch
			break;
			
		case MBMP::Error:	// all errors printed regardless of loglevel
			stream1 << msg << endl;
			if (logtofile) stream2 << msg << endl;
			break;

		case MBMP::Warning: //all warnings printed regardless of loglevel
			stream1 << msg << endl;
			if (logtofile) stream2 << msg << endl;
			break;
		
		case MBMP::Info:
			switch (loglevel) {
				case 0:		// case 0 supress Info output
					break;	
				default: // otherwise print Info output
					stream1 << msg << endl;
					if (logtofile) stream2 << msg << endl;
			}	// loglevel switch
			break;
		
		case MBMP::ClockLost: // message printed regardless of loglevel
			stream1 << msg << endl;
			if (logtofile) stream2 << msg << endl;
			break;
			
		case MBMP::Application:	// a message we posted to the bus
			switch (loglevel) {
				case 0:		// case 0 supress output
					break;	
				default: // otherwise print output
					stream1 << msg << endl;
					if (logtofile) stream2 << msg << endl;
			}	// loglevel switch
			break;
		
		case MBMP::Buffering: // buffering messages
			static bool b_finished = true;
			
			switch (loglevel) {
				case 0:		// no text output
					break;
				case 1:		// normally show one buffering message
					if (b_finished) {
						stream1 << "Buffering....." << endl;
						if (logtofile) stream2 << "Buffering" << endl;
					}
					break;
				default:	// case 2 and above, show every message
					stream1 << "Buffering " << msg << "%" << endl;
					if (logtofile) stream2 << "Buffering " << msg << "%" << endl;
			}	// loglevel switch
				
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
			
		case MBMP::Unhandled: // a GstBus message we didn't handle (should never get here)
				switch (loglevel) {
					case 0:		// case 0 supress output
					break;	
				default: // otherwise print output
					stream1 << msg << endl;
					if (logtofile) stream2 << msg << endl;
				}	// loglevel switch
			break;	// Unhandled GstBus message		
		
		case MBMP::Duration:	// a new stream duration message 
			switch (loglevel) {
				case 0:		// case 0 supress output
					break;	
				default: // otherwise print output
					stream1 << msg << endl;
					if (logtofile) stream2 << msg << endl;
				}	// loglevel switch
			break;				
			
		default:	// should never be here so if we are we had best see the message
			stream1 << msg << endl;
			if (logtofile) stream2 << msg << endl;
			break;
		
		}	// mtype switch
			
	if (logtofile) logfile.close();
	
	return;
}

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
// Slot to select or change the audio visualizer. Extract the visualizer
// name from the QAction and send it to p_gstiface. 
void PlayerControl::changeVisualizer(QAction* act)
{
		QString vis = act->text();
		p_gstiface->changeVisualizer(vis);
	
	return;
}


////////////////////////////// Protected Functions ////////////////////////////
//
//	Create a context menu	
void PlayerControl::contextMenuEvent(QContextMenuEvent* e)
{
	control_menu->popup(e->globalPos());
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

