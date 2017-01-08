/**************************** playlist.cpp *****************************

Code to manage the media playlist.

Copyright (C) 2014-2016
by: Andrew J. Bibb
License: MIT 

Permission is hereby granted, free of charge, to any person obtaFining a copy 
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

# include "./code/playlist/playlist.h"
# include "./code/playerctl/playerctl.h"
# include "./code/resource.h"
# include "./code/iconman/iconman.h"
# include "./code/scman/scman.h"

# include <QtCore/QDebug>
# include <QFileDialog>
# include <QInputDialog>
# include <QStringList>
# include <QActionGroup>
# include <QFileInfo>
# include <QTime>
# include <QTableWidgetItem>
# include <QFile>
# include <QTextStream>
# include <QProcessEnvironment>

// Use GStreamer to process media tags
# include <gst/gst.h>
# include <gst/tag/tag.h>

MetaData::MetaData(QObject* parent) : QObject(parent)
{
	return;
}

void MetaData::clear()
{
	title = QString();
	date = QString();
	status = QString();
	label = QString();
	discid = QString();
	releaseid = QString();
	relgrpid = QString();
	tracklist.clear();
	
	return;
}


// NOTES: There are a couple of things to keep in mind if we need to expand
// functionality in the future.  In the function seedPlaylist() we assume
// that a url must start with "http" or "ftp", and we assume that a 
// playlist must end with ".m3u".  Additionally in the function addFile()
// we assume that playlist must end with ".m3u"

Playlist::Playlist(QWidget* parent) : QDialog(parent)
{
  // setup the user interface
  ui.setupUi(this);
  
  // initialize class members
  geometry = QRect();
  ui.listWidget_playlist->clear();
  arturl.clear();
  cangoprevious = false;
  cangonext = false;
  
  // Iconmanager with constructor only scope
  IconManager iconman(this);
  
  // Initialize musicbrainz manager
  mbman = NULL;

  // Class to hold audio CD metadata about the current disk
  cdmetadata = new MetaData(static_cast<QObject*>(this) );
  
  // Process playlist related settings. The geometry and playlist contents
  // are set in Playerctl.
	QSettings* settings = new QSettings(ORG, APP, this);
	settings->beginGroup("Preferences");
  iconman.setIconColor(settings->value("colorize_icons").toString() );
  settings->endGroup();
 
	settings->beginGroup("Playlist_Settings");
	ui.checkBox_wrap->setChecked(settings->value("wrap").toBool() );
	ui.checkBox_consume->setChecked(settings->value("consume").toBool() );
	ui.checkBox_random->setChecked(settings->value("random").toBool() );
	ui.checkBox_showinfo->setChecked(settings->value("details").toBool() );
	settings->endGroup();
  
  // Show or hide the details box. After this show and hide controlled
  // by signals and slots in the UI.
  ui.widget_details->setVisible(ui.checkBox_showinfo->isChecked() );
  
  // Setup the data directories 
  // APP defined in resource.h
  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  QString home = env.value("HOME");
  plist_dir = QDir(QString(env.value("XDG_DATA_HOME", QString(QDir::homePath()) + "/.local/share") + "/%1/playlists").arg(QString(APP).toLower()) );
	if (! plist_dir.exists()) plist_dir.mkpath(plist_dir.absolutePath() );   
	artwork_dir = QDir(QString(env.value("XDG_DATA_HOME", QString(QDir::homePath()) + "/.local/share") + "/%1/artwork").arg(QString(APP).toLower()) );
	if (! artwork_dir.exists()) artwork_dir.mkpath(artwork_dir.absolutePath() ); 
	cdmeta_dir = QDir(QString(env.value("XDG_DATA_HOME", QString(QDir::homePath()) + "/.local/share") + "/%1/cdmeta").arg(QString(APP).toLower()) );
	if (! cdmeta_dir.exists()) cdmeta_dir.mkpath(cdmeta_dir.absolutePath() );
	 
  // assign icons to actions
	ui.actionMoveUp->setIcon(iconman.getIcon("move_up"));
	ui.actionMoveDown->setIcon(iconman.getIcon("move_down"));
	ui.actionAddMedia->setIcon(iconman.getIcon("add_media"));
	ui.actionRemoveItem->setIcon(iconman.getIcon("remove_item"));
	ui.actionHidePlaylist->setIcon(iconman.getIcon("hide_playlist"));
	ui.actionAddFiles->setIcon(iconman.getIcon("add_files"));
	ui.actionAddURL->setIcon(iconman.getIcon("add_url"));
	ui.actionAddAudio->setIcon(iconman.getIcon("add_audio"));
	ui.actionAddVideo->setIcon(iconman.getIcon("add_video"));
	ui.actionRemoveAll->setIcon(iconman.getIcon("remove_all"));
	ui.actionSavePlaylist->setIcon(iconman.getIcon("save_playlist"));
	ui.actionAddPlaylist->setIcon(iconman.getIcon("add_playlist"));
	ui.actionToggleWrap->setIcon(iconman.getIcon("playlist_wrap"));
	ui.actionToggleConsume->setIcon(iconman.getIcon("playlist_consume"));
	ui.actionToggleRandom->setIcon(iconman.getIcon("playlist_random"));
	ui.actionToggleDetail->setIcon(iconman.getIcon("playlist_detail"));
  
  // Assign actions defined in the UI to toolbuttons.  This also has the
  // effect of adding actions to this dialog so shortcuts work.  If there
  // is no toolbutton for the action then just add the action.
	this->ui.toolButton_moveup->setDefaultAction(ui.actionMoveUp);
	this->ui.toolButton_movedown->setDefaultAction(ui.actionMoveDown);
	this->ui.toolButton_add->setDefaultAction(ui.actionAddMedia);
	this->addAction(ui.actionAddAudio);
	this->addAction(ui.actionAddVideo);
	this->addAction(ui.actionAddPlaylist);
	this->addAction(ui.actionAddPlaylist);
	this->addAction(ui.actionAddFiles);
	this->addAction(ui.actionAddURL);
	this->ui.toolButton_remove->setDefaultAction(ui.actionRemoveItem);
	this->ui.toolButton_removeall->setDefaultAction(ui.actionRemoveAll);
	this->addAction(ui.actionHidePlaylist);	
	this->ui.toolButton_exit->setDefaultAction(ui.actionHidePlaylist);
	this->addAction(ui.actionSavePlaylist);
	this->ui.toolButton_save->setDefaultAction(ui.actionSavePlaylist);
	this->addAction(ui.actionToggleWrap);
	this->addAction(ui.actionToggleConsume);
	this->addAction(ui.actionToggleRandom);
	this->addAction(ui.actionToggleDetail);
	
	// add actions to action groups
	media_group = new QActionGroup(this);	
	media_group->addAction(ui.actionAddAudio);
	media_group->addAction(ui.actionAddVideo);
	media_group->addAction(ui.actionAddPlaylist);
	media_group->addAction(ui.actionAddFiles);
	
	// create the media menu
	media_menu = new QMenu(this);
	media_menu->setTitle(ui.actionAddMedia->text());
	media_menu->setIcon(ui.actionAddMedia->icon());
	media_menu->addAction(ui.actionAddAudio);
	media_menu->addAction(ui.actionAddVideo);
	media_menu->addAction(ui.actionAddPlaylist);
	media_menu->addAction(ui.actionAddFiles);
	media_menu->addSeparator();
	media_menu->addAction(ui.actionAddURL);
	
	// create the playlist menu
	playlist_menu = new QMenu(this);
	playlist_menu->addAction(ui.actionMoveUp);
	playlist_menu->addAction(ui.actionMoveDown);
	playlist_menu->addSeparator();
	playlist_menu->addMenu(media_menu);
	playlist_menu->addAction(ui.actionRemoveItem);
	playlist_menu->addAction(ui.actionRemoveAll);
	playlist_menu->addSeparator();
	playlist_menu->addAction(ui.actionSavePlaylist);
	playlist_menu->addSeparator();	
	playlist_menu->addAction(ui.actionHidePlaylist);	
	playlist_menu->addSeparator();	
	playlist_menu->addAction(ui.actionToggleWrap);
	playlist_menu->addAction(ui.actionToggleConsume);
	playlist_menu->addAction(ui.actionToggleRandom);
	playlist_menu->addAction(ui.actionToggleDetail);
		  
  // add the shortcuts defined by the user to the actions
  ShortCutManager scman(this);
  ui.actionMoveUp->setShortcuts(scman.getKeySequence("cmd_moveup") );
  ui.actionMoveDown->setShortcuts(scman.getKeySequence("cmd_movedown") );
  ui.actionAddAudio->setShortcuts(scman.getKeySequence("cmd_addaudio") );
  ui.actionAddVideo->setShortcuts(scman.getKeySequence("cmd_addvideo") );
  ui.actionAddPlaylist->setShortcuts(scman.getKeySequence("cmd_addplaylist") );
  ui.actionAddFiles->setShortcuts(scman.getKeySequence("cmd_addfile") );
  ui.actionAddURL->setShortcuts(scman.getKeySequence("cmd_addurl") );  
  ui.actionRemoveItem->setShortcuts(scman.getKeySequence("cmd_removeitem") );
  ui.actionRemoveAll->setShortcuts(scman.getKeySequence("cmd_removeall") );
  ui.actionHidePlaylist->setShortcuts(scman.getKeySequence("cmd_playlist") );
  ui.actionSavePlaylist->setShortcuts(scman.getKeySequence("cmd_saveplaylist") );
  ui.actionToggleWrap->setShortcuts(scman.getKeySequence("cmd_togglewrap") );
  ui.actionToggleConsume->setShortcuts(scman.getKeySequence("cmd_toggleconsume") );
  ui.actionToggleRandom->setShortcuts(scman.getKeySequence("cmd_togglerandom") );
  ui.actionToggleDetail->setShortcuts(scman.getKeySequence("cmd_toggledetail") );
  
  // connect signals to slots
  connect (ui.actionMoveUp, SIGNAL(triggered()), this, SLOT(moveItemUp()));
  connect (ui.actionMoveDown, SIGNAL(triggered()), this, SLOT(moveItemDown()));  
  connect (ui.actionAddMedia, SIGNAL(triggered()), this, SLOT(addMedia()));
  connect (ui.actionAddURL, SIGNAL(triggered()), this, SLOT(addURL()));
  connect (media_group, SIGNAL(triggered(QAction*)), this, SLOT(addFile(QAction*)));
  connect (ui.actionRemoveItem, SIGNAL(triggered()), this, SLOT(removeItem()));
  connect (ui.actionRemoveAll, SIGNAL(triggered()), this, SLOT(clearPlaylist()));
  connect (ui.actionHidePlaylist, SIGNAL(triggered()), this, SLOT(hide()));
  connect (ui.listWidget_playlist, SIGNAL(itemDoubleClicked(QListWidgetItem*)), qobject_cast<PlayerControl*>(parent), SLOT(playMedia()));
  connect (ui.actionSavePlaylist, SIGNAL(triggered()), this, SLOT(savePlaylist()));
  connect (ui.actionToggleWrap, SIGNAL(triggered()), this, SLOT(toggleWrapMode()));
  connect (ui.actionToggleConsume, SIGNAL(triggered()), this, SLOT(toggleConsumeMode()));
  connect (ui.actionToggleRandom, SIGNAL(triggered()), this, SLOT(toggleRandomMode()));
  connect (ui.actionToggleDetail, SIGNAL(triggered()), this, SLOT(toggleDetailMode()));
  connect (ui.listWidget_playlist, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this, SLOT(currentItemChanged(QListWidgetItem*, QListWidgetItem*)));
  connect (ui.checkBox_wrap, SIGNAL(toggled(bool)), this, SIGNAL(wrapModeChanged(bool)));
  connect (ui.checkBox_random, SIGNAL(toggled(bool)), this, SIGNAL(randomModeChanged(bool)));
  
	settings->deleteLater();
 
 return;
}

////////////////////////////// Public Slots ////////////////////////////
//
// Slot to save the playlist to file.  Called when ui.actionSavePlaylist
// is triggered.
void Playlist::savePlaylist()
{
	// return if there are no items to save
	if (ui.listWidget_playlist->count() <= 0 ) return;
	
	// constants
	const QString playlistfiles = "*.m3u";	
		
	// Get the filename to save as
	QString sfiles = tr("Playlists (%1);;All Files (*.*)").arg(playlistfiles);
	QString filename = QFileDialog::getSaveFileName(
											this,
											tr("Save the playlist"),
                      plist_dir.absolutePath(),
                      sfiles );
  
  // if filename is empty return
  if (filename.isEmpty() ) return;
  
  // create a QFile, and write the data to it
  if (! filename.endsWith(".m3u", Qt::CaseInsensitive)) filename.append(".m3u");
  QFile file(filename);
  if (! file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
  QTextStream out(&file);
  out << "# EXTM3U" << "\n" << "\n";
  for (int i = 0; i < ui.listWidget_playlist->count(); ++i) {
		PlaylistItem* pli = static_cast<PlaylistItem*>(ui.listWidget_playlist->item(i));
		QString line1 = QString("#EXTINF:%1,%2 - %3").arg(pli->getDuration()).arg(pli->getArtist()).arg(pli->getTitle());
		QString line2 = (pli->getUri()).remove("file://");
		out << line1 << "\n";
		out << line2 << "\n" << "\n";
	}
	
	file.close();                    
	return;
}

//
// Slot to select an item the playlist.  Direction tells which
// way to go in the playlist and should be an MBMP namespace enum.
// Return true if the current item is different then when we entered,
// false otherwise.
bool Playlist::selectItem(const short& direction)
{
	// If there is nothing in the playlist return now
	if (ui.listWidget_playlist->count() < 1 ) return false;
		
	// Initialize variables
	if (ui.listWidget_playlist->currentRow() < 0 ) ui.listWidget_playlist->setCurrentRow(0);
	int row = ui.listWidget_playlist->currentRow();
	const int lastrow = ui.listWidget_playlist->count() - 1;
	QListWidgetItem* curitem = ui.listWidget_playlist->currentItem();

	// Set the row based on the direction.  Directions except for Next
	// work as expected if random is checked.  Random only applies to Next 
	switch (direction) {
		case MBMP_PL::First:
			row = 0;
			break;
		case MBMP_PL::Previous:
			if (row == 0 )
				ui.checkBox_wrap->isChecked() ?  row = lastrow : row = 0;
			else	
				--row;	
			break;
		case MBMP_PL::Next:
			// if the random box is checked and there are at least 2 items 
			// calculate a random row number
			if (ui.checkBox_random->isChecked() && ui.listWidget_playlist->count() > 1) {
				// generate a random new row that is not the current row
				while (row == ui.listWidget_playlist->currentRow())  {
					const int low = 0;
					const int high = ui.listWidget_playlist->count() - 1;
					row = qrand() % ((high + 1) - low) + low;
				}	// while
			}	// if
			else {
				if (row == lastrow )
					ui.checkBox_wrap->isChecked() ?  row = 0 : row = lastrow;
				else	
					++row;
				}	// else
			break;
		case MBMP_PL::Last:
			row = lastrow;
			break;	
	}	// switch			
				
	// If current row did not change with all the previous calculations 
	// return an empty string, this won't interupt the current playback
	// If consume is checked remove the item first, then return false
	if (row  == ui.listWidget_playlist->currentRow() && direction != MBMP_PL::Current ) {
		if (ui.checkBox_consume->isChecked() ) 
			delete ui.listWidget_playlist->takeItem(ui.listWidget_playlist->row(curitem));
	return false;
	}
			
	// Set the current row and the item string based on the previous calculations
	ui.listWidget_playlist->setCurrentRow(row);	
	
	// If the consume box is checked and we are not looking for the current item
	// consume the item that was current when we entered this function
	if (ui.checkBox_consume->isChecked() && direction != MBMP_PL::Current ) 
		delete ui.listWidget_playlist->takeItem(ui.listWidget_playlist->row(curitem));

	// We have a new current item, return true.
	return true;
}

//
//	Slot to present a choice of media types to open
void Playlist::addMedia()
{
	// make sure the playlist is visible
	if (this->isHidden() ) this->show();
	
	// popup the media menu
	// align with the center of the ui.toolButton_add widget
	media_menu->popup(ui.toolButton_add->mapToGlobal(QPoint(ui.toolButton_add->width() / 2, ui.toolButton_add->height() / 2)) );
	
	return;
}

//
//	Slot to open a file dialog to select media files
void Playlist::addFile(QAction* a)
{
	// make sure the playlist is visible
	if (this->isHidden() ) this->show();
	
	const QString audio = "*.mp3 *.mp4 *.m4a *.ogg *.oga *.flac";
	const QString video = "*.mp4 *.mkv *.avi *.ogv *.webm *.vob";
	const QString plext = "*.m3u *.pls";
	
	// select the files to be presented
	QString startdir = QDir::homePath();
	QString s_files = tr("Media (%1 %2);;All Files (*.*)").arg(audio).arg(video);
	if (a == ui.actionAddAudio ) s_files = tr("Audio (%1);;All Files (*.*)").arg(audio);
	else if (a == ui.actionAddVideo ) s_files = tr("Video (%1);;All Files (*.*)").arg(video);
		else if (a == ui.actionAddPlaylist) {
			s_files = tr("Playlist (%1);;All Files (*.*)").arg(plext);
			startdir = plist_dir.absolutePath();
		}
	
	// Open a file dialog to select media files
	QStringList sl_files = QFileDialog::getOpenFileNames(
                        this,
                        tr("Select one or more media files to add to the playlist"),
                        startdir,
                         s_files);	
                        
  // If we selected files add them to the playlist.  If the playlist contains device tracks
	// clear them out first.
  if (sl_files.size() > 0) {
		if (ui.listWidget_playlist->count() > 0 ) {
			this->setWindowTitle(tr("Playlist"));
			if ( (ui.listWidget_playlist->item(0)->type() == MBMP_PL::ACD)	| 
					 (ui.listWidget_playlist->item(0)->type() == MBMP_PL::DVD)	)
				ui.listWidget_playlist->clear();
		}
		for (int i = 0; i < sl_files.size(); ++i) {
			if (sl_files.at(i).endsWith(".m3u", Qt::CaseInsensitive) )
				this->processM3U(sl_files.at(i));
			else if (sl_files.at(i).endsWith(".pls", Qt::CaseInsensitive) )
				this->processPLS(sl_files.at(i));
			else
				new PlaylistItem(sl_files.at(i), ui.listWidget_playlist, MBMP_PL::File);
		}	// for
	}	//if        

	// update the summary count
	this->updateSummary();

	return;
}

//
//	Slot to open an input dialog to get a url
void Playlist::addURL()
{
	if (! ui.actionAddURL->isEnabled() ) return;
	
	// make sure the playlist is visible
	if (this->isHidden() ) this->show();
	
	// Open an input dialog to get the url
	QString s = QInputDialog::getText(
							this,
							tr("Enter a URL to add to the playlist"),
							tr("URL: ") );
	
	// If we got a URL add it to the playlist.  If the playlist contains device tracks
	// clear them out first.
	if (! s.isEmpty() ) {
		if (ui.listWidget_playlist->count() > 0 ) {
			this->setWindowTitle(tr("Playlist"));
			if (ui.listWidget_playlist->item(0)->type() && (MBMP_PL::ACD | MBMP_PL::DVD)) ui.listWidget_playlist->clear();
		}	// if
		new PlaylistItem(s, ui.listWidget_playlist, MBMP_PL::Url);	
	}	// if

	// update the summary count
	this->updateSummary();
	
	return;
}

//
// Slot to add a URI. URI's can only be delivered by mpris2 so there is no user interaction we
// we need to coordinate with
void Playlist::addURI(const QString& uri)
{
	QUrl url(uri);
	
	if (url.scheme().contains("file", Qt::CaseSensitive) ) { 
		new PlaylistItem(url.toLocalFile(), ui.listWidget_playlist, MBMP_PL::File);
    this->updateSummary();
	}
		
	else if (url.scheme().contains("http", Qt::CaseSensitive) ) {
		new PlaylistItem(url.toString(), ui.listWidget_playlist, MBMP_PL::Url);	
		this->updateSummary();	
	}
	
	return;
}

//
// Slot to add tracks (for instance from an Audio CD) to the playlist.  Tracks and files/url's
// can't coexist in the playlist, so clear the playlist first.  The <TOCEntry> list comes from 
// GST_InterFace via PlayerControl/
void Playlist::addTracks(QList<TocEntry> tracks)
{
	// return if there is nothing to process
	if (tracks.size() <= 0 ) return;
	
	// clear the tracklist entries, and initialize option boxes
	ui.listWidget_playlist->clear();
	ui.checkBox_wrap->setChecked(false);
	ui.checkBox_consume->setChecked(false);
	ui.checkBox_random->setChecked(false);
	
	// set the title
	this->setWindowTitle(tr("Audio CD - Tracklist"));
	
	// create the tracklist entry
	// Currently set title to Audio CD.  Change in updateTracks if we run
	// that function
	for (int i = 0; i < tracks.size(); ++i) {
			PlaylistItem* pli = 0;
			pli= new PlaylistItem(tr("Track %1").arg(tracks.at(i).track), ui.listWidget_playlist, MBMP_PL::ACD);
			pli->setSequence(tracks.at(i).track);
			if (tracks.at(i).end - tracks.at(i).start >= 0)
				pli->setDuration(tracks.at(i).end - tracks.at(i).start);
			pli->setTitle(pli->text() ); 
			pli->makeDisplayText();
	}	// for
	
	// Make the first entry current
	ui.listWidget_playlist->setCurrentRow(0);
	
	// Disable adding of any other media types.  Don't call lockContols as 
	// we do allow some of the movement controls to be active with audio CD's
	ui.actionAddMedia->setDisabled(true);
	ui.actionAddAudio->setDisabled(true);
	ui.actionAddVideo->setDisabled(true);
	ui.actionAddPlaylist->setDisabled(true);
	ui.actionAddFiles->setDisabled(true);
	ui.actionAddURL->setDisabled(true);	
	ui.actionSavePlaylist->setDisabled(true);

	// update the summary count
	this->updateSummary();
	
	return;
}

//
// Slot to add dvd chapters to the playlist, as above Chapters and files/url's 
// can't coexist in the playlist so clear the playlist first. The count comes
// from GST_Interface via PlayerCtl
void Playlist::addChapters(int count)
{
	// return if count is not at least one chapter
	if (count < 1 ) return;
	
	// clear the tracklist entries, and initialize option boxes
	ui.listWidget_playlist->clear();
	ui.checkBox_wrap->setChecked(false);
	ui.checkBox_consume->setChecked(false);
	ui.checkBox_random->setChecked(false);
	
	// set the title
	this->setWindowTitle(tr("DVD - Chapters"));
	
	// create the entries
	for (int i = 0; i < count; ++i) {
		PlaylistItem* pli = new PlaylistItem(tr("Chapter"), ui.listWidget_playlist, MBMP_PL::DVD);
		pli->setSequence(i + 1);
		pli->setTitle(tr("DVD"));
		pli->makeDisplayText();		
	} // for

	// update the summary count
	this->updateSummary();
		
	return;	
}

//
//	Slot to remove the currently selected playlist item
void Playlist::removeItem()
{
	if (ui.listWidget_playlist->currentRow() >= 0)
		delete ui.listWidget_playlist->takeItem(ui.listWidget_playlist->currentRow() );
		
	// update the summary count
	this->updateSummary();
	
	return;
}

//
//	Slot to move the currently selected playlist item up one row
void Playlist::moveItemUp()
{
	int row = ui.listWidget_playlist->currentRow();
	if (row > 0) {
		ui.listWidget_playlist->insertItem(row - 1, ui.listWidget_playlist->takeItem(row)); 	
		ui.listWidget_playlist->setCurrentRow(row - 1);
	}	// if
	
	return;
}

//
//	Slot to move the currently selected playlist item down one row
void Playlist::moveItemDown()
{
	int row = ui.listWidget_playlist->currentRow();
	if (ui.listWidget_playlist->count() > 1 ) {
		ui.listWidget_playlist->insertItem(row + 1, ui.listWidget_playlist->takeItem(row)); 	
		ui.listWidget_playlist->setCurrentRow(row + 1);
	}	// if
	
	return;	
}

//
// Slot to process playlist entries when a new discid is received.  Called
// as a function from PlayerCtl
void Playlist::discIDChanged(const QString& id)
{
	// See if we can go out on the internet
	QSettings* settings = new QSettings(ORG, APP, this);
	settings->beginGroup("Preferences");
	bool b_disable_internet = settings->value("disable_internet").toBool();
	settings->endGroup();
	settings->deleteLater();
		
	// See if we've got info about the disc stored locally.
	if (readCDMetaFile(id) ) {
		this->updateTracks();
		
		// Now look for album art for the CD 
		QStringList searchtags;
		searchtags << cdmetadata->getReleaseID() << cdmetadata->getTitle();
		QPixmap pm = getLocalAlbumArt(searchtags);
		if (pm.isNull() && ! b_disable_internet  ) {
			if (mbman == NULL) mbman = new MusicBrainzManager(this);
			mbman->retrieveAlbumArt(cdmetadata->getRelGrpID(), cdmetadata->getReleaseID() );
			connect(mbman, SIGNAL(artworkRetrieved()), this, SLOT(albumArtRetrieved()));
		}
		else 
			ui.label_artwork->setPixmap(pm);
		return;
	}
	
	// Nothing local, look on Musicbrainz if allowed
	if (! b_disable_internet) {
		if (mbman == NULL) mbman = new MusicBrainzManager(this);
		mbman->retrieveCDMetaData(id);
		connect (mbman, SIGNAL(metaDataRetrieved(const QString&)), this, SLOT(cdMetaDataRetrieved(const QString&)));
	}	// if
	
	return;
}

//
// Slot to read a newly retrieved CD metadata file.  Only called when discIDChanged() and we've gone out
// on the internet to retrieve the metadata and received a reply
void Playlist::cdMetaDataRetrieved(const QString& albumid)
{
	// only try to read the file if we got an albumid back
	if (! albumid.isEmpty() ) {
		if (readCDMetaFile(albumid)) {
			this->updateTracks();
			
			// get album art if we don't already have it. (we can't be here unlessw mbman already exists
			// so no need to do a new on it).
			QStringList sl;
			sl << cdmetadata->getReleaseID() << cdmetadata->getTitle();
			if (getLocalAlbumArt(sl).isNull() ) {
				mbman->retrieveAlbumArt(cdmetadata->getRelGrpID(), cdmetadata->getReleaseID() );
				connect(mbman, SIGNAL(artworkRetrieved()), this, SLOT(albumArtRetrieved()));			
			}	// if we need to to out to the internet to find album art
		}	// if we can read the metadata file	
	}	// if albumid not empty
	
	// disconnect the signal that got us here.
	disconnect (mbman, SIGNAL(metaDataRetrieved(const QString&)), 0, 0);
	
	return;
}

//
// Slot called when new album art has been retrieved
void Playlist::albumArtRetrieved()
{		
	// tags from the currently playing item
	QStringList searchtags;
	searchtags << static_cast<PlaylistItem*>(ui.listWidget_playlist->currentItem())->getTagAsString("musicbrainz-albumid");
	searchtags << static_cast<PlaylistItem*>(ui.listWidget_playlist->currentItem())->getTagAsString(GST_TAG_ALBUM);
	
	// art should now be local, look for it
	QPixmap pm = getLocalAlbumArt(searchtags);
	if (! pm.isNull() ) ui.label_artwork->setPixmap(pm);
	
	disconnect(mbman, SIGNAL(artworkRetrieved()), 0, 0);	
	return;
}

//
// Slot called when the current item is changed
void Playlist::currentItemChanged(QListWidgetItem* cur, QListWidgetItem* old)
{
	(void) old;
	
	// return now if no current item
	cangonext = false;
	cangoprevious = false;	
	if (cur == NULL) return;
	
	// save the movement properties of current item
	if (ui.checkBox_wrap->isChecked() ) {
		cangonext = true;
		cangoprevious = true;
	}// if wrap checked
	else {
		int pl_row = this->getCurrentRow();
		int pl_size = this->getPlaylistSize();
		if (pl_size <= 1) {cangonext = false; cangoprevious = false;}			
		else if (pl_size - pl_row == 1) {cangonext = false; cangoprevious = true;}
			else if (pl_row == 0) {cangonext = true; cangoprevious = false;}
				else {cangonext = true; cangoprevious = true;}
	}	// else wrap not checked
		
	ui.label_iteminfo->clear();
	ui.label_artwork->clear();
	arturl.clear();
	QStringList searchtags;
	searchtags << static_cast<PlaylistItem*>(cur)->getTagAsString("musicbrainz-albumid");
	searchtags << static_cast<PlaylistItem*>(cur)->getTagAsString(GST_TAG_ALBUM);
	
	// If we're playing a local file
	if (this->currentItemType() == MBMP_PL::File) {
		QString s = static_cast<PlaylistItem*>(cur)->getInfoText();
		if (! s.isEmpty() ) ui.label_iteminfo->setText(s);
		
		// First see if the art is contained in the tag
		if (static_cast<PlaylistItem*>(cur)->hasArtwork())
			ui.label_artwork->setPixmap(static_cast<PlaylistItem*>(cur)->getArtwork() ); 	
		
		// Then search directories in the computer
		else {		
			QFileInfo fi(this->getCurrentUri().remove("file://") );
			QDir d(fi.path());		
			QPixmap pm = getLocalAlbumArt(searchtags, d);
			if (! pm.isNull() ) ui.label_artwork->setPixmap(pm);
			else {
				QSettings* settings = new QSettings(ORG, APP, this);
				settings->beginGroup("Preferences");
				bool b_disable_internet = settings->value("disable_internet").toBool();
				settings->endGroup();
				settings->deleteLater();
				if (! b_disable_internet) {
					if (mbman == NULL) mbman = new MusicBrainzManager(this);
					mbman->retrieveReleaseData(static_cast<PlaylistItem*>(cur)->getTagAsString(GST_TAG_ALBUM), this->getCurrentArtist() );
					connect(mbman, SIGNAL(artworkRetrieved()), this, SLOT(albumArtRetrieved()));
				}	// if allowed to go out on the internet
			}	// else need to go out and look
		}	// else didn't find one in the music directory	
	}	// if playing local file
	
	else if (this->currentItemType() == MBMP_PL::ACD) {
		QString s = static_cast<PlaylistItem*>(cur)->getInfoText();
		if (! s.isEmpty() ) ui.label_iteminfo->setText(s);
		
		QPixmap pm = getLocalAlbumArt(searchtags);
		if (! pm.isNull() ) ui.label_artwork->setPixmap(pm);
	}
		
	return;	
}

////////////////////////////// Public Functions ////////////////////////////
//
//	Function to seed the playlist (called from the playerctl constructor)
void Playlist::seedPlaylist(const QStringList& sl_seed)
{
	// Assume url's are good, for files check to make sure we can find it before we add it
	for (int i = 0; i < sl_seed.size(); ++i) {
		if (sl_seed.at(i).startsWith("ftp", Qt::CaseInsensitive) || sl_seed.at(i).startsWith("http", Qt::CaseInsensitive)) {
			new PlaylistItem(sl_seed.at(i), ui.listWidget_playlist, MBMP_PL::Url);
		}	// if
		
		else {
			QFileInfo fi = sl_seed.at(i);
			if (fi.exists()) {
				if (sl_seed.at(i).endsWith(".m3u", Qt::CaseInsensitive) )
					this->processM3U(sl_seed.at(i));
				else
					new PlaylistItem(fi.canonicalFilePath(), ui.listWidget_playlist, MBMP_PL::File);	
			}	// if fileinfo exists
		}	// else does not start with ftp or http
	}	// for

	// update the summary count
	this->updateSummary();
	
	return;	
}

//
// Function to lock or unlock the playlist controls. Users can nomally 
// drag and drop, move or delete items in the playlist.  For DVD disable
// this since movement is mainly through menus and I have no interest 
// in watching a movie with the chapters all out of order
void Playlist::lockControls(bool b_lock)
{
	ui.checkBox_wrap->setDisabled(b_lock);
	ui.checkBox_consume->setDisabled(b_lock);
	ui.checkBox_random->setDisabled(b_lock);
	ui.actionMoveUp->setDisabled(b_lock);
	ui.actionMoveDown->setDisabled(b_lock);
	ui.actionAddMedia->setDisabled(b_lock);
	ui.actionRemoveItem->setDisabled(b_lock);
	ui.actionRemoveAll->setDisabled(b_lock);
	ui.actionAddAudio->setDisabled(b_lock);
	ui.actionAddVideo->setDisabled(b_lock);
	ui.actionAddPlaylist->setDisabled(b_lock);
	ui.actionAddFiles->setDisabled(b_lock);
	ui.actionAddURL->setDisabled(b_lock);	
	ui.actionSavePlaylist->setDisabled(b_lock);
}

//
// Function to return a QStringList containing the current items in the playlist
QStringList Playlist::getCurrentList()
{
	// create our return value
	QStringList sl;
	sl.clear();
	
	// go through the playlist and get names
  for (int i = 0; i < ui.listWidget_playlist->count(); ++i) {
		PlaylistItem* pli = static_cast<PlaylistItem*>(ui.listWidget_playlist->item(i));
		sl << pli->getUri().remove("file://");
	}
	
	return sl;
}

//
// Function to return a QString containing the title of the item currently playing
QString Playlist::getWindowTitle()
{
	PlaylistItem* pli = static_cast<PlaylistItem*>(ui.listWidget_playlist->currentItem());
	
	if (ui.listWidget_playlist->currentItem()->type() == MBMP_PL::ACD) {
		int seq = -1;
		seq = pli->getSequence();
		if (seq >= 0) 
			return tr("%1 - Track %2").arg(pli->getTitle()).arg(seq);
		else
			return tr("%1").arg(pli->getTitle());
	}	// if CD
	
	// for DVD's playerctl will get title from p_gstiface
	if (ui.listWidget_playlist->currentItem()->type() == MBMP_PL::DVD)  
		return QString();

	// default (not CD or DVD)	
	QString title = "";
	QString artist = "";
	
	title = pli->getTitle();
	artist = pli->getArtist();
	if (! title.isEmpty() ) 
		return (artist.isEmpty() ? title : QString("%1 - %2").arg(artist).arg(title) );
	else return (pli->getUri()).section("//", 1, 1);
}

//
// Function to save playlist settings.  pos is the slider position
// sent to this function from PlayerControl
void Playlist::saveSettings(const int& pos)
{
	QSettings* settings = new QSettings(ORG, APP, this);	
	settings->beginGroup("Playlist");
	settings->setValue("entries", this->getCurrentList());
	settings->setValue("current", this->getCurrentRow());
	settings->setValue("position", pos);
	settings->endGroup();
	
	settings->beginGroup("Playlist_Settings");
	settings->setValue("wrap", ui.checkBox_wrap->isChecked() );
	settings->setValue("consume", ui.checkBox_consume->isChecked() );
	settings->setValue("random", ui.checkBox_random->isChecked() );
	settings->setValue("details", ui.checkBox_showinfo->isChecked() );
	
	settings->deleteLater();
 
	return;
}

//////////////////////////// Protected Functions ////////////////////////////
//
//	Reimplement the QHideEvent 
void Playlist::hideEvent(QHideEvent* )
{
	
	geometry = this->frameGeometry();
	
	return;
}

//
//	Reimplement the QShowEvent
void Playlist::showEvent(QShowEvent* )
{
  geometry.isNull() ? this->move(20,50) : this->setGeometry(geometry);	

	return;
}

//
//	Create a context menu activate by right click of the mouse.
void Playlist::contextMenuEvent(QContextMenuEvent* e)
{
	playlist_menu->popup(e->globalPos());
}	


//////////////////////////// Private Functions ////////////////////////////
//
// Function to process a .m3u (playlist) file.  Called from addFile() when
// a file ends with .m3u
void Playlist::processM3U(const QString& plfile)
{
	// Make a QDir out of the input string, if plfile contains relative
	// paths we'll need this.
	QFileInfo pldir = QFileInfo(plfile);
	
	// Open plfile for reading
  QFile file(plfile);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

	// read the file and create playlist entries from the contents
  QTextStream in(&file);
  QString line = QString();
  while (!in.atEnd()) {
		line = in.readLine();
    if (line.startsWith("#EXTINF:", Qt::CaseSensitive)) {
			line = in.readLine();
			if (line.startsWith("ftp") || line.startsWith("http") ) {
				new PlaylistItem(line, ui.listWidget_playlist, MBMP_PL::Url);
			}	// if a URL
			else {
				QFileInfo itemtarget = QFileInfo(line);
				if (itemtarget.isRelative()) 
					new PlaylistItem(QString(pldir.canonicalPath() + "/" + itemtarget.filePath()), ui.listWidget_playlist, MBMP_PL::File);
				else 
					new PlaylistItem(itemtarget.canonicalFilePath(), ui.listWidget_playlist, MBMP_PL::File);
			}	// else a file
		}	// if
	}	// while
	
	file.close();
	return;	
}	

//
// Function to process a .pls (shoutcast playlist) file.  Called from addFile() when
// a file ends with .pls
void Playlist::processPLS(const QString& plfile)
{
	// Make a QDir out of the input string, if plfile contains relative
	// paths we'll need this.
	QFileInfo pldir = QFileInfo(plfile);
	
	// Open plfile for reading
  QFile file(plfile);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

	// read the file and create playlist entries from the contents.  Use the
	// stringlist incase the PLS file is not in order.
  QTextStream in(&file);
  QString line = QString();
  QStringList sl = QStringList();
  while (!in.atEnd()) {
		line = in.readLine();
    if (line.startsWith("File", Qt::CaseSensitive)) {
			bool ok = false;
			int idx = line.section('=', 0, 0).remove(0, 4).toInt(&ok, 10);
			if (ok) sl.insert(idx - 1, line.section('=', 1, 1) );
		}	// if
	}	// while
	
	// create a playlist item for each element in the string list
	if (sl.count() > 0) {
		for (int i = 0; i < sl.count(); ++i) {
			if (sl.at(i).startsWith("ftp") || sl.at(i).startsWith("http") ) {
				new PlaylistItem(sl.at(i), ui.listWidget_playlist, MBMP_PL::Url);
			}	// if a URL
			else if (! sl.at(i).isEmpty() ) {
				QFileInfo itemtarget = QFileInfo(sl.at(i));
				if (itemtarget.isRelative()) 
					new PlaylistItem(QString(pldir.canonicalPath() + "/" + itemtarget.filePath()), ui.listWidget_playlist, MBMP_PL::File);
				else 
					new PlaylistItem(itemtarget.canonicalFilePath(), ui.listWidget_playlist, MBMP_PL::File);
			}	// else if a file
		} // for
	}	// if sl count > 0
			  
	file.close();
	return;
}

//
// Function to update the summary text. Called at the end of the add functions,
// at the end of seedPlaylist(), and at the end of removeItem()
void Playlist::updateSummary()
{
	// Variables
	int totaltime = 0;
	
	// If there are no items in the playlist blank out the sumamry
	// and enable all controls
	if (ui.listWidget_playlist->count() <= 0) {
		ui.label_summary->clear();
		ui.label_iteminfo->clear();
		ui.label_artwork->clear();
		lockControls(false);
		return;
	}
	
	// Walk through the playlist and get the duration for each item
  for (int i = 0; i < ui.listWidget_playlist->count(); ++i) {
		PlaylistItem* pli = static_cast<PlaylistItem*>(ui.listWidget_playlist->item(i));
		totaltime = totaltime + pli->getDuration();
	}
	
	if (totaltime == 0) 
		ui.label_summary->setText(tr("%1 Playlist items").arg(ui.listWidget_playlist->count()));
 	else {
		QTime n(0,0,0);
		QTime t;
		t = n.addSecs(totaltime);			
		ui.label_summary->setText(tr("%1 Items, Total playing time %2") 
						.arg(ui.listWidget_playlist->count() )
						.arg(totaltime > (60 * 60) ? t.toString("h:mm:ss") : t.toString("mm:ss")) );
	}
	
	return;
}

// 
// Function to read the local file containing CD metadata.  Return false on any error.
// The bool return is used to determine if we need to go out to MusicBrainz or not.
bool Playlist::readCDMetaFile(const QString& discid)
{	
	QFile sourcefile;
	sourcefile.setFileName(cdmeta_dir.absoluteFilePath(discid + ".xml") );
	if (sourcefile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		
		// Prepare data container
		cdmetadata->clear();
		
		// Read and parse the file	
		QXmlStreamReader* xml = new QXmlStreamReader(&sourcefile);	
		QStringList pos;
		while (! xml->atEnd() ) {	
			switch(xml->readNext() ) {
				case QXmlStreamReader::StartElement:
					pos.append(xml->name().toString() );
					//qDebug() << pos.join(',');
					if (pos.join(',') == "metadata,discid") {
						cdmetadata->setDiscID(xml->readElementText(QXmlStreamReader::SkipChildElements) );
						pos.removeLast();
						}
					else if (pos.join(',') == "metadata,releaseid") {
						cdmetadata->setReleaseID(xml->readElementText(QXmlStreamReader::SkipChildElements) );
						pos.removeLast();
						}	
					else if (pos.join(',') == "metadata,relgrpid") {
						cdmetadata->setRelGrpID(xml->readElementText(QXmlStreamReader::SkipChildElements) );
						pos.removeLast();
					}
					else if (pos.join(',') == "metadata,title") {
						cdmetadata->setTitle(xml->readElementText(QXmlStreamReader::SkipChildElements) );
						pos.removeLast();
					}
					else if (pos.join(',') == "metadata,artist") {
						cdmetadata->setArtist(xml->readElementText(QXmlStreamReader::SkipChildElements) );
						pos.removeLast();
					}
					else if (pos.join(',') == "metadata,date") {
						cdmetadata->setDate(xml->readElementText(QXmlStreamReader::SkipChildElements) );
						pos.removeLast();
					}
					else if (pos.join(',') == "metadata,status") {
						cdmetadata->setStatus(xml->readElementText(QXmlStreamReader::SkipChildElements) );
						pos.removeLast();
					}
					else if (pos.join(',') == "metadata,label") {
						cdmetadata->setLabel(xml->readElementText(QXmlStreamReader::SkipChildElements) );	
						pos.removeLast();
					}
					else if (pos.join(',') == "metadata,tracklist,track") {
						Track track;
						track.title = QString();
						track.tracknumber = QString();
						track.duration = QString();
						while (! xml->atEnd() ) {
							switch (xml->readNext() ) {
								case QXmlStreamReader::StartElement:
									pos.append(xml->name().toString() );
									if (pos.join(',') == "metadata,tracklist,track,track_number") {
										track.tracknumber = xml->readElementText(QXmlStreamReader::SkipChildElements);
										pos.removeLast();
									}
									if (pos.join(',') == "metadata,tracklist,track,title") {
										track.title =	xml->readElementText(QXmlStreamReader::SkipChildElements);
										pos.removeLast();
									}
									if (pos.join(',') == "metadata,tracklist,track,duration") {
										track.duration =	xml->readElementText(QXmlStreamReader::SkipChildElements);
										pos.removeLast();
									}   
									break;	
								case QXmlStreamReader::EndElement:
									if (xml->name() == "track") {
										cdmetadata->setTrack(track);
									}
									//qDebug() << pos.join(',');
									pos.removeLast();
									break;	
								default:
									continue;
							}	// switch
							if (xml->tokenType() == QXmlStreamReader::EndElement && xml->name() == "tracklist") break;
						}	// while
					}	// if track-list,track
					break;		
				case QXmlStreamReader::EndElement:	
					pos.removeLast();
					//qDebug() << pos.join(',');
					break;	
				case QXmlStreamReader::Invalid:
					#if QT_VERSION >= 0x050400 
						qCritical("XML stream reading error: %s %s", qUtf8Printable(xml->error()), qUtf8Printable(xml->errorString()) );
					# else	
						qCritical("XML stream reading error: %s %s", qPrintable(xml->error()), qPrintable(xml->errorString()) );
					# endif
					break;
				default:
					continue;
			}	// switch
		}	// while
		
		sourcefile.close();	
		delete xml;
		return true;
	}	// if file could be opened for reading
	
	else
		return false;
}

//
// Function to update the track information, called from read CD metadata
// and only if the read worked.
void Playlist::updateTracks()
{
	QList<Track> tl = cdmetadata->getTrackList();
	
	// playlist count must equal the metadata tracklist count
	if (ui.listWidget_playlist->count() != tl.count() ) return;
	
	// update each entry
	for (int i = 0; i < ui.listWidget_playlist->count(); ++i) {		
		PlaylistItem* pli = static_cast<PlaylistItem*>(ui.listWidget_playlist->item(i) ) ;
		for (int j = 0; j < tl.count(); ++j) {
			bool ok;
			int seq = (tl.at(j).tracknumber).toInt(&ok);	
			if (pli->getSequence() == seq && ok) {
				pli->setTitle(tl.at(j).title);
				long dur = (tl.at(j).duration).toLong(&ok);
				if (ok) pli->setDuration(dur / 1000 ); // use musicbrainz duration
				// now add information common to the CD
				pli->addTag("artist", cdmetadata->getArtist());
				pli->addTag(GST_TAG_ALBUM, cdmetadata->getTitle());
				pli->addTag("release_date", cdmetadata->getDate());
				pli->addTag("release_status", cdmetadata->getStatus());
				pli->addTag("release_label", cdmetadata->getLabel());
				pli->addTag(GST_TAG_MUSICBRAINZ_ALBUMID, cdmetadata->getDiscID());
				pli->addTag("musicbrainz-albumid", cdmetadata->getReleaseID());
				pli->addTag("musizbrainz-releasegroupid", cdmetadata->getRelGrpID());
				pli->makeDisplayText();
			}	// if
		}	// j for
	}	// i for
	// update the now playing details
	QString s = static_cast<PlaylistItem*>(ui.listWidget_playlist->currentItem())->getInfoText();
	if (! s.isEmpty() ) ui.label_iteminfo->setText(s);
	
}

//
// Function to get local album art.  Return a null pixmap is none found
QPixmap Playlist::getLocalAlbumArt(const QStringList& searchtags, const QDir& dir)
{
	QStringList ext = (QStringList() << ".jpg" << ".png" << ".gif" << ".jpeg" << ".bmp");
	QStringList namefilters;
	QStringList entlist;
	
	namefilters.clear();
	entlist.clear();
	arturl.clear();
	
	// Look for any image in the media directory first, if a directory was provided
	if (dir.exists() ) {
		for (int i = 0; i < ext.count(); ++i) {
			namefilters << QString("*%1").arg(ext.at(i));
		}
		entlist = dir.entryList(namefilters, QDir::Files, QDir::NoSort);
		if (entlist.count() > 0) {
			arturl = QUrl::fromLocalFile(dir.absoluteFilePath(entlist.at(0)) );
			return QPixmap(dir.absoluteFilePath(entlist.at(0)) );
		}
	}	// if dir exists
	
	// Not image in media directory, look in artwork_dir			
	namefilters.clear();
	for (int i = 0; i < searchtags.count(); ++i) {
		for (int j = 0; j < ext.count(); ++j) {
			namefilters << QString("%1%2").arg(searchtags.at(i)).arg(ext.at(j));
		}
		entlist = artwork_dir.entryList(namefilters, QDir::Files, QDir::NoSort);
		if (entlist.count() > 0) {
			arturl = QUrl::fromLocalFile(artwork_dir.absoluteFilePath(entlist.at(0)) );
			return QPixmap(artwork_dir.absoluteFilePath(entlist.at(0)) );
		}	// if found cover
	}	// for searchtags
	
	// no luck, return a null pixmap
	return QPixmap();
}
