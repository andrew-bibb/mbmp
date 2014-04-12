/**************************** playlist.cpp *****************************

Code to manage the media playlist.

Copyright (C) 2014
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

# include <QtCore/QDebug>
# include <QFileDialog>
# include <QInputDialog>
# include <QStringList>
# include <QListWidgetItem>
# include <QActionGroup>
# include <QFileInfo>
# include <QTime>
# include <QTableWidgetItem>

Playlist::Playlist(QWidget* parent) : QDialog(parent)
{
  // setup the user interface
  ui.setupUi(this);
  
  // initialize class members
  geometry = QRect();
  ui.listWidget_playlist->clear();
  
  // Assign actions defined in the UI to toolbuttons.  This also has the
  // effect of adding actions to this dialog so shortcuts work.  If there
  // is no toolbutton for the action then just add the action.
	this->ui.toolButton_moveup->setDefaultAction(ui.actionMoveUp);
	this->ui.toolButton_movedown->setDefaultAction(ui.actionMoveDown);
	this->ui.toolButton_add->setDefaultAction(ui.actionAddMedia);
	this->addAction(ui.actionAddAudio);
	this->addAction(ui.actionAddVideo);
	this->addAction(ui.actionAddFiles);
	this->addAction(ui.actionAddURL);
	this->ui.toolButton_remove->setDefaultAction(ui.actionRemoveItem);
	this->ui.toolButton_removeall->setDefaultAction(ui.actionRemoveAll);
	this->addAction(ui.actionHidePlaylist);	
	this->ui.toolButton_exit->setDefaultAction(ui.actionHidePlaylist);
	
	// add actions to action groups
	media_group = new QActionGroup(this);	
	media_group->addAction(ui.actionAddAudio);
	media_group->addAction(ui.actionAddVideo);
	media_group->addAction(ui.actionAddFiles);
	
	// create the media menu
	media_menu = new QMenu(this);
	media_menu->setTitle(ui.actionAddMedia->text());
	media_menu->setIcon(ui.actionAddMedia->icon());
	media_menu->addAction(ui.actionAddAudio);
	media_menu->addAction(ui.actionAddVideo);
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
	playlist_menu->addAction(ui.actionHidePlaylist);	
		  
  // add the shortcuts defined by the user to the actions
  ui.actionMoveUp->setShortcuts(qobject_cast<PlayerControl*>(parent)->getShortcuts("cmd_moveup") );
  ui.actionMoveDown->setShortcuts(qobject_cast<PlayerControl*>(parent)->getShortcuts("cmd_movedown") );
  ui.actionAddAudio->setShortcuts(qobject_cast<PlayerControl*>(parent)->getShortcuts("cmd_addaudio") );
  ui.actionAddVideo->setShortcuts(qobject_cast<PlayerControl*>(parent)->getShortcuts("cmd_addvideo") );
  ui.actionAddFiles->setShortcuts(qobject_cast<PlayerControl*>(parent)->getShortcuts("cmd_addfile") );
  ui.actionAddURL->setShortcuts(qobject_cast<PlayerControl*>(parent)->getShortcuts("cmd_addurl") );  
  ui.actionRemoveItem->setShortcuts(qobject_cast<PlayerControl*>(parent)->getShortcuts("cmd_removeitem") );
  ui.actionRemoveAll->setShortcuts(qobject_cast<PlayerControl*>(parent)->getShortcuts("cmd_removeall") );
  ui.actionHidePlaylist->setShortcuts(qobject_cast<PlayerControl*>(parent)->getShortcuts("cmd_playlist") );
  
  // connect signals to slots
  connect (ui.actionMoveUp, SIGNAL(triggered()), this, SLOT(moveItemUp()));
  connect (ui.actionMoveDown, SIGNAL(triggered()), this, SLOT(moveItemDown()));  
  connect (ui.actionAddMedia, SIGNAL(triggered()), this, SLOT(addMedia()));
  connect (ui.actionAddURL, SIGNAL(triggered()), this, SLOT(addURL()));
  connect (media_group, SIGNAL(triggered(QAction*)), this, SLOT(addFile(QAction*)));
  connect (ui.actionRemoveItem, SIGNAL(triggered()), this, SLOT(removeItem()));
  connect (ui.actionRemoveAll, SIGNAL(triggered()), ui.listWidget_playlist, SLOT(clear()));
  connect (ui.actionHidePlaylist, SIGNAL(triggered()), this, SLOT(hide()));
  connect (ui.listWidget_playlist, SIGNAL(itemDoubleClicked(QListWidgetItem*)), qobject_cast<PlayerControl*>(parent), SLOT(playMedia()));
}

////////////////////////////// Public Slots ////////////////////////////
//
// Slot to return a string from the playlist.  Direction tells which
// way to go in the playlist and should be an MBMP namespace enum.
QString Playlist::getItem(const short& direction)
{
	// If there is nothing in the playlist return now
	if (ui.listWidget_playlist->count() < 1 ) return QString();
		
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
	// If consume is checked remove the item first, then return the empty
	// string
	if (row  == ui.listWidget_playlist->currentRow() && direction != MBMP_PL::Current ) {
		if (ui.checkBox_consume->isChecked() ) 
			delete ui.listWidget_playlist->takeItem(ui.listWidget_playlist->row(curitem));
	return QString();
	}
			
	// Set the current row and the item string based on the previous calculations
	ui.listWidget_playlist->setCurrentRow(row);	
	
	// If the consume box is checked and we are not looking for the current item
	// consume the item that was current when we entered this function
	if (ui.checkBox_consume->isChecked() && direction != MBMP_PL::Current ) 
		delete ui.listWidget_playlist->takeItem(ui.listWidget_playlist->row(curitem));

	// Qualify the name for gstreamer.  
	if (ui.listWidget_playlist->item(row)->type() == MBMP_PL::File) return ui.listWidget_playlist->item(row)->text().prepend("file://");
	else if (ui.listWidget_playlist->item(row)->type() == MBMP_PL::Url) return ui.listWidget_playlist->item(row)->text();
		else if (ui.listWidget_playlist->item(row)->type() == MBMP_PL::ACD) return ui.listWidget_playlist->item(row)->text();
			else if (ui.listWidget_playlist->item(row)->type() == MBMP_PL::DVD) return ui.listWidget_playlist->item(row)->text();
			
	
	// Default return value (should never get here)
	return QString();
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
	
	// select the files to be presented
	QString s_files = tr("Media (%1 %2);;All Files (*.*)").arg(audio).arg(video);
	if (a == ui.actionAddAudio ) s_files = tr("Audio (%1);;All Files (*.*)").arg(audio);
	else if (a == ui.actionAddVideo ) s_files = tr("Video (%1);;All Files (*.*)").arg(video);
	
	// Open a file dialog to select media files
	QStringList sl_files = QFileDialog::getOpenFileNames(
                        this,
                        tr("Select one or more media files to add to the playlist"),
                        "/home",
                         s_files);	
                        
  // If we selected files add them to the playlist.  If the playlist contains device tracks
	// clear them out first.
  if (sl_files.size() > 0) {
		if (ui.listWidget_playlist->count() > 0 ) {
			this->setWindowTitle(tr("Playlist"));
			if (ui.listWidget_playlist->item(0)->type() >= MBMP_PL::Dev) ui.listWidget_playlist->clear();
		}
		for (int i = 0; i < sl_files.size(); ++i) {
			new QListWidgetItem(sl_files.at(i), ui.listWidget_playlist, MBMP_PL::File);
		}	// for
	}	//if        

	return;
}

//
//	Slot to open an input dialog to get a url
void Playlist::addURL()
{
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
			if (ui.listWidget_playlist->item(0)->type() >= MBMP_PL::Dev) ui.listWidget_playlist->clear();
		}	// if
		new QListWidgetItem(s, ui.listWidget_playlist, MBMP_PL::Url);	
	}	// if
	
	return;
}

//
// Slot to add tracks (for instance from an Audio CD) to the playlist.  Tracks and files/url's
// can't coexist in the playlist, so clear the playlist first.  The stringlist comes from PlayerControl
// which creates the list from data provided by GST_Interface.
void Playlist::addTracks(QStringList sl_tracks)
{
	// return if there is nothing to process
	if (sl_tracks.size() <= 0 ) return;
	
	// clear the tracklist entries
	ui.listWidget_playlist->clear();
	
	// set the title
	this->setWindowTitle(tr("Audio CD - Tracklist"));
	
	// create the tracklist entry
	for (int i = 0; i < sl_tracks.size(); ++i) {
			new QListWidgetItem(sl_tracks.at(i), ui.listWidget_playlist, MBMP_PL::ACD);
	}	// for
	
	// Make the first entry current
	ui.listWidget_playlist->setCurrentRow(0);
	
	// Disable adding of any other media types
	ui.actionAddMedia->setDisabled(true);
	ui.actionAddFiles->setDisabled(true);
	ui.actionAddURL->setDisabled(true);
	ui.actionAddAudio->setDisabled(true);
	ui.actionAddVideo->setDisabled(true);
	media_menu->setDisabled(true);
	
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
	
	// clear the tracklist entries
	ui.listWidget_playlist->clear();
	
	// set the title
	this->setWindowTitle(tr("DVD - Chapters"));
	
	// create the entries
	for (int i = 1; i <= count; ++i) {
		new QListWidgetItem(tr("Chapter %1").arg(i), ui.listWidget_playlist, MBMP_PL::DVD);
	} // for
		
	return;	
}

//
//	Slot to remove the currently selected playlist item
void Playlist::removeItem()
{
	if (ui.listWidget_playlist->currentRow() >= 0)
		delete ui.listWidget_playlist->takeItem(ui.listWidget_playlist->currentRow() );
		
	
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

////////////////////////////// Public Functions ////////////////////////////
//
//	Function to seed the playlist (called from the playerctl constructor)
void Playlist::seedPlaylist(const QStringList& sl_seed)
{
	// Assume url's are good, for files check to make sure we can find it before we add it
	for (int i = 0; i < sl_seed.size(); ++i) {
		if (sl_seed.at(i).startsWith("ftp", Qt::CaseInsensitive) || sl_seed.at(i).startsWith("http", Qt::CaseInsensitive)) {
			new QListWidgetItem(sl_seed.at(i), ui.listWidget_playlist, MBMP_PL::Url);
		}	// if
		
		else {
			QFileInfo fi = sl_seed.at(i);
			if (fi.exists()) {
				new QListWidgetItem(fi.canonicalFilePath(), ui.listWidget_playlist, MBMP_PL::File);
			}	// if fileinfo exists
		}	// else does not start with ftp or http
	}	// for
	
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
	media_menu->setDisabled(b_lock);
	
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

