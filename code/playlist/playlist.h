
/**************************** playlist.h *****************************

Code to manage the media playlist.

Copyright (C) 2014-2014
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

# ifndef PLAYLIST_H
# define PLAYLIST_H

# include <QListWidget>
# include <QDialog>
# include <QRect>
# include <QContextMenuEvent>
# include <QString>
# include <QStringList>
# include <QAction>
# include <QActionGroup>
# include <QMenu>
# include <QList>

# include "ui_playlist.h"

//	Enum's local to this program
namespace MBMP_PL 
{
  enum {
		First = 0x01,							// First item	
		Previous = 0x02,					// Previous item
    Current	= 0x03,						// Current item 
    Next	= 0x04, 						// Next item
    Last = 0x05,							// Last item 
    File = (QListWidgetItem::UserType + 1),		// Playlist file 
    Url  = (QListWidgetItem::UserType + 2),		// Playlist url
    Dev  = (QListWidgetItem::UserType + 100),	// Marker for the start of devices
    ACD  = (QListWidgetItem::UserType + 101),	// Playlist Audio CD
    DVD  = (QListWidgetItem::UserType + 102)	// Playlist DVD	
  };
} // namespace MBMP_PL

//	This class is based on a QListWidget and a QDialog
class Playlist : public QDialog 
{	
  Q_OBJECT

  public:
		Playlist (QWidget*);
  
  public slots:
		QString getItem(const short&);
		void addMedia();
		void addFile(QAction*);	
		void addURL();
		void addTracks(QStringList);
		void removeItem();
		void moveItemUp();
		void moveItemDown();
		inline void triggerAddAudio() {ui.actionAddAudio->trigger();}
		inline void triggerAddVideo() {ui.actionAddVideo->trigger();}
		inline void triggerAddFiles() {ui.actionAddFiles->trigger();}
		inline int currentItemType() {return ui.listWidget_playlist->currentItem()->type();}
	
	public:
		void seedPlaylist(const QStringList&);

	protected:
		void hideEvent(QHideEvent*);
		void showEvent(QShowEvent*);
		void contextMenuEvent(QContextMenuEvent*);

  private:
  // members 
    Ui::Playlist ui;  
		QRect geometry;	
		QActionGroup* media_group;
		QMenu* playlist_menu;
		QMenu* media_menu;
	       
	// functions  

};

#endif

