/************************ playlistitem.h *****************************

Playlist items, derived from QListWidgetItem

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

# ifndef PLAYLISTITEM_H
# define PLAYLISTITEM_H

# include <QListWidget>
# include <QListWidgetItem>
# include <QString>


//	Class based on a QListWidget item, used for entries in the Playlist class below
class PlaylistItem : public QListWidgetItem
{
	public:
		PlaylistItem (const QString&, QListWidget*, int, uint seq = 0, int dur = -1);
		
		inline uint getSequence() {return sequence;};
		inline QString getUri() {return uri;};
		
	private:
	// members - which ones are used depends upon the item type
		QString errors;				// compliation of any errors encountered in creating the item
		uint sequence;				// track or chapter number
		QString uri;					// the uri of the media 
		qint16 duration;			// length in seconds, or a negative number for duation not known
		bool seekable;				// true if we can seek in the stream, false otherwise	
		QString title;				// the title of the media file	
		QString artist;				// the artist
		QString album;				// the album	
};
		

#endif

