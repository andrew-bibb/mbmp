/**************************** playlist.h *****************************

Code to manage the media playlist.

Copyright (C) 2014-2020
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
# include <QContextMenuEvent>
# include <QString>
# include <QStringList>
# include <QAction>
# include <QActionGroup>
# include <QMenu>
# include <QList>
# include <QListWidgetItem>
# include <QDir>
# include <QPixmap>
# include <QUrl>

# include "ui_playlist.h"
# include "./code/playlist/playlistitem.h"
# include "./code/gstiface/gstiface.h"
# include "./code/mbman/mbman.h"

//	Enum's local to this program
namespace MBMP_PL 
{
  enum {
		First = 0x01,							// First item	
		Previous = 0x02,					// Previous item
    Current	= 0x03,						// Current item 
    Next	= 0x04, 						// Next item
    Last = 0x05,							// Last item 
    None = (QListWidgetItem::UserType) + 1,		// No type
    File = (QListWidgetItem::UserType) + 11,	// Playlist file 
    Url  = (QListWidgetItem::UserType) + 12,	// Playlist url
    ACD  = (QListWidgetItem::UserType) + 101,	// Playlist Audio CD
    DVD  = (QListWidgetItem::UserType) + 102,	// Playlist DVD	
  };
} // namespace MBMP_PL

//  Structure to contain CD track information
struct Track
{
  QString title;
  QString tracknumber;
  QString  duration;
};

//  Class to hold and access CD information
class MetaData : public QObject
{
  public:
    MetaData (QObject*);
    void clear();
    inline void setTitle(const QString& t) {title = t;}
    inline void setDate(const QString& d) {date = d;}
    inline void setStatus(const QString& s) {status = s;}
    inline void setLabel(const QString& l) {label = l;}
    inline void setDiscID(const QString i) {discid = i;}
    inline void setReleaseID(const QString& id) {releaseid = id;}
    inline void setRelGrpID(const QString& rgid) {relgrpid = rgid;}
    inline void setArtist(const QString& a) {artist = a;}
    inline void setTrack(const Track& trk) {tracklist.append(trk);}

    inline QString getTitle() {return title;}
    inline QString getDate() {return date;}
    inline QString getStatus() {return status;}
    inline QString getLabel() {return label;}
    inline QString getDiscID() {return discid;}
    inline QString getReleaseID() {return releaseid;}
    inline QString getRelGrpID() {return relgrpid;}
    inline QString getArtist() {return artist;}
    inline QList<Track> getTrackList() {return tracklist;}

  private:
    QString title;
    QString date;
    QString status;
    QString label;
    QString discid;
    QString releaseid;
    QString relgrpid;
    QString artist;
    QList<Track> tracklist;
};
    
//	This class is based on a QListWidget and a QDialog
class Playlist : public QWidget 
{	
  Q_OBJECT

  public:
		Playlist (QWidget*);
  
  public slots:
		void savePlaylist();
		bool selectItem(const short&);
		void addMedia();
		void addFile(QAction*);	
		void addURL();
		void addURI(const QString&);
		void addTracks(QList<TocEntry>);
		void addChapters(int);
		void removeItem();
		void moveItemUp();
		void moveItemDown();
		void discIDChanged(const QString&);
		void cdMetaDataRetrieved(const QString&);
		void albumArtRetrieved();
		inline void clearPlaylist() {ui.listWidget_playlist->clear(); updateSummary();}
		inline void triggerAddAudio() {if (ui.actionAddAudio->isEnabled()) ui.actionAddAudio->trigger();}
		inline void triggerAddVideo() {if (ui.actionAddVideo->isEnabled()) ui.actionAddVideo->trigger();}
		inline void triggerAddPlaylist() {if (ui.actionAddPlaylist->isEnabled()) ui.actionAddPlaylist->trigger();}
		inline void triggerAddFiles() {if (ui.actionAddFiles->isEnabled()) ui.actionAddFiles->trigger();}	
		inline void setCurrentChapter(int chap) {ui.listWidget_playlist->setCurrentRow(chap - 1);}
		inline void setCurrentRow(const int& row) {ui.listWidget_playlist->setCurrentRow(row);}
		inline void toggleWrapMode() {ui.checkBox_consume->setChecked(false); ui.checkBox_wrap->toggle();}
		inline void setWrapMode(bool b_wm) {ui.checkBox_wrap->setChecked(b_wm);}
		inline void toggleConsumeMode() {ui.checkBox_wrap->setChecked(false); ui.checkBox_consume->toggle();}
		inline void toggleRandomMode() {ui.checkBox_random->toggle();}
		inline void setRandomMode(bool b_rm) {ui.checkBox_random->setChecked(b_rm);}
		inline void toggleDetailMode() {ui.checkBox_showinfo->toggle();}
	
		inline QString getCurrentUri() {return ui.listWidget_playlist->count() > 0 ? static_cast<PlaylistItem*>(ui.listWidget_playlist->currentItem())->getUri() : QString();}
		inline qint16 getCurrentSeq() {return ui.listWidget_playlist->count() > 0 ? static_cast<PlaylistItem*>(ui.listWidget_playlist->currentItem())->getSequence() : -1;}
		inline QString getCurrentTitle() {return ui.listWidget_playlist->count() > 0 ? static_cast<PlaylistItem*>(ui.listWidget_playlist->currentItem())->getTitle() : QString();}
		inline QString getCurrentArtist() {return ui.listWidget_playlist->count() > 0 ? static_cast<PlaylistItem*>(ui.listWidget_playlist->currentItem())->getArtist() : QString();}
		inline qint32 getCurrentDuration() {return ui.listWidget_playlist->count() > 0 ? static_cast<PlaylistItem*>(ui.listWidget_playlist->currentItem())->getDuration() : -1;}
		inline int getCurrentRow() {return ui.listWidget_playlist->count() > 0 ? ui.listWidget_playlist->currentRow() : -1;}		
		inline int getPlaylistSize() {return ui.listWidget_playlist->count();}
		inline QString getArtURL() {return arturl.url(QUrl::None);}
		inline QString getCurrentTagAsString(const QString& tag) {return ui.listWidget_playlist->count() > 0 ? static_cast<PlaylistItem*>(ui.listWidget_playlist->currentItem())->getTagAsString(tag) : QString();}
		inline bool canGoNext() {return cangonext;}
		inline bool	canGoPrevious() {return cangoprevious;}
		
		inline int currentItemType() {return ui.listWidget_playlist->count() > 0 ? ui.listWidget_playlist->currentItem()->type() : MBMP_PL::None;}
		inline bool currentIsPlayable() {return ui.listWidget_playlist->count() > 0 ? static_cast<PlaylistItem*>(ui.listWidget_playlist->currentItem())->isPlayable() : false;}
		inline bool currentIsSeekable() {return ui.listWidget_playlist->count() > 0 ? static_cast<PlaylistItem*>(ui.listWidget_playlist->currentItem())->isSeekable() : false;}
		void currentItemChanged(QListWidgetItem*, QListWidgetItem*);
		
	public:
		void seedPlaylist(const QStringList&);
		void lockControls(bool);
		QStringList getCurrentList();
		QString getWindowTitle();
		void saveSettings(const int&);
		const QPixmap* getAlbumArt();

	protected:
		void contextMenuEvent(QContextMenuEvent*);	

  private:
  // members 
    Ui::Playlist ui;  
		QActionGroup* media_group;
		QMenu* playlist_menu;
		QMenu* media_menu;
		QDir plist_dir;
		QDir artwork_dir;
    QDir cdmeta_dir;
	  MetaData* cdmetadata;
	  MusicBrainzManager* mbman; 
	  QUrl arturl;
	  bool cangonext;
	  bool cangoprevious;
	  
	// functions
		void processM3U(const QString&);
		void processPLS(const QString&);
		void updateSummary();
		bool readCDMetaFile(const QString&);
		void updateTracks();
		QPixmap getLocalAlbumArt(const QStringList&, const QDir& = QDir("NONE"));
		
	Q_SIGNALS:	
		void wrapModeChanged(const bool&);	
		void randomModeChanged(const bool&);
		void artworkRetrieved();
};

#endif

