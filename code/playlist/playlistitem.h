/************************ playlistitem.h *****************************

Playlist items, derived from QListWidgetItem

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

# ifndef PLAYLISTITEM_H
# define PLAYLISTITEM_H

# include <QListWidget>
# include <QListWidgetItem>
# include <QString>
# include <QPixmap>
# include <QMap>

// Use GStreamer to process media tags
# include <gst/gst.h>
# include <gst/tag/tag.h>
# include <gst/pbutils/pbutils.h>

//	Class based on a QListWidget item, used for entries in the Playlist class below
class PlaylistItem : public QListWidgetItem
{
	public:
		PlaylistItem (const QString&, QListWidget*, int);
		
		// get functions
		inline qint16 getSequence() {return sequence;}
		inline QString getUri() {return uri;}
		inline QString getArtist() {return artist;}
		inline QString getTitle() {return title;}
		inline qint32 getDuration() {return duration;}
		inline bool isPlayable() {return errors.isEmpty() ? true : false;}
		inline QString getInfoText() {return toolTip();}
		inline bool hasArtwork() {return b_has_artwork;}
		inline QPixmap getArtwork() {return pm_artwork;}
		QString getTagAsString(const QString& tag);
		
		// set functions
		inline void setSequence(uint seq) {sequence = seq;}
		inline void setDuration(qint16 dur) {duration = dur;}
		inline void setTitle(QString ttl) {title = ttl;}
		inline void setArtist(QString art) {artist = art;} // not currently used for anything
		inline void addTag(QString key, QString val) {tag_map[key] = val;}
		
		// functions
		void makeDisplayText();
		
	private:
	// members - which ones are used depends upon the item type
		qint16 sequence;			// ACD track or chapter number or file track tag 
		qint32 duration;			// length in seconds, or a negative number for duation not known		
		QString uri;					// the uri of the media 
		bool seekable;				// true if we can seek in the stream, false otherwise	
		QString title;				// the title tag of the media file
		QString artist;				// the artist tag
		QString album;				// the album tag
		QString errors;				// compliation of any errors encountered in creating the item		
		bool b_has_artwork;		// true if the tags contain artwork 
		QPixmap pm_artwork;		// the artwork extracted from GStreamer tags
		QMap<QString,QString> tag_map;	// tags and values stored in a QMap
		
	// functions	
		void makeToolTip();
		void runDiscoverer();
		
};
		

#endif

