/************************ playlistitem.cpp *****************************

Playlist items, derived from QListWidgetItem

Copyright (C) 2014-2022
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

# include "./code/playlist/playlistitem.h"
# include "./code/playlist/playlist.h"

# include <QtCore/QDebug>
# include <QTime>

PlaylistItem::PlaylistItem(const QString& text, QListWidget* parent, int type) : QListWidgetItem(text, parent, type)
{
	// Data members.  Not all are used for any single PlaylistItem.  We probably could have used the QListWidgetItem::data
	// functions (with Qt::UserData), but we need to subclass PlaylistItem anyway and it just seemed easier to add
	// in our own data items.
	sequence = -1;
	duration = -1;	
	uri = QString();	
	seekable = false;		
	title.clear();
	artist.clear();
	album.clear();
	errors.clear();	
	lyrics.clear();
	tag_map.clear();
	b_has_lyrics = false;
	b_has_artwork = false;
	pm_artwork = QPixmap();
	
	// Somewhat of a hack, but to display text in nice columns we either need a full QTableWidget (or worse a QTableView),
	// or QListWidgetItems with monospace text.  Since we're only looking for appearance, not function, monospace fonts
	// here we come.
	QFont font("Monospace");
	font.setStyleHint(QFont::Monospace);
	this->setFont(font);
		
	// Finish constructing based on the item type
	switch (type) {	
		case MBMP_PL::File: {
			uri = QString("file://" + text);
			this->runDiscoverer();	
			this->makeDisplayText();
			break; }
			
		case MBMP_PL::Url: {
			uri = text;
			break; }
		
		// for ACD and DVD makeDisplayText() is called from the function that
		// creates this item, in Playlist.cpp.  No reason to call it here.	
		case MBMP_PL::ACD: {
			seekable = true;
			break; }
			
		case MBMP_PL::DVD: {
			seekable = true;
			break; }
		
		default:
			break;
	}	// switch	
		
	return;
}


//////////////////////////// Public Functions ////////////////////////////
//
// Function to return a tag out of the tag_map
QString PlaylistItem::getTagAsString(const QString& tag)
{
	if (tag_map.contains(tag) )
		return tag_map.value(tag);
	else
		return QString();
}

//
// Function to set the display text for the PlaylistItem
// called from the constructor and from playlist.cpp when we are
// making ACD or DVD items
void PlaylistItem::makeDisplayText()
{
	// process based on the item type
	switch (this->type()) {
		case MBMP_PL::File: {
			// Constants
			const short wcol1 = -10;
			const short wcol2 = -25;	
			const short wcol3 = -60;	
			
			if (duration < 0 ) {
				if (title.isEmpty()) this->setText(this->text() ); 
				else this->setText(title.simplified());
			}	// if there is no duration
			else {
				// Put the duration (seconds) into a QTime
				QTime n(0,0,0);
				QTime t;
				t = n.addSecs(duration);				
				
				if (title.isEmpty()) {
					this->setText(QString("%1%2")
						.arg(duration > (60 * 60) ? t.toString("h:mm:ss") : t.toString("mm:ss"), wcol1, QChar(' '))
						.arg(text() ) );
					}	// if title is empty
				else {
					if (artist.isEmpty() || album.isEmpty()) {
						this->setText(QString("%1%2%3")
						.arg(duration > (60 * 60) ? t.toString("h:mm:ss") : t.toString("mm:ss"), wcol1, QChar(' '))
						.arg(artist.simplified(), wcol2, QChar(' '))
						.arg(title.simplified()) );
					}	// if artist or album is empty
					else {										
						this->setText(QString("%1%2%3%4")
							.arg(duration > (60 * 60) ? t.toString("h:mm:ss") : t.toString("mm:ss"), wcol1, QChar(' '))
							.arg(artist.simplified(), wcol2, QChar(' '))
							.arg(album.simplified(), wcol3, QChar(' '))
							.arg(title.simplified()) );
					}	// else artist or album are not empty
				}	// else title is not empty
			}	// else	there is a duration
			break; }	// case PlaylistItem is a file
		
		case MBMP_PL::Url: {
			break; }	// case PlaylistItem is a url
		
		case MBMP_PL::ACD: {
			// Constants
			const short wcol1 = -8;				
			QTime n(0,0,0);
			QTime t = n.addSecs(duration);
			this->setText(QString("%1 %2")
				.arg(duration > (60 * 60) ? t.toString("h:mm:ss") : t.toString("mm:ss"), wcol1, QChar(' '))
				.arg(title) );
			break; }	// case PlaylistItem is Audio CD track

		case MBMP_PL::DVD: {
			// Constants
			const short wcol1 = -4;
			setText(QString("%1%2 %3")
				.arg(QString::number(sequence, 10), wcol1, QChar(' ') )
				.arg(QObject::tr("Chapter") ) 
				.arg(sequence) );
			break; }	// case PlaylistItem is a DVD chapter
	
		default:
			break;			
	}	// switch
	
	this->makeToolTip();
	
	return;
}

//////////////////////////// Private Functions ////////////////////////////
//
// Function to set the tooltip text for the PlaylistItem
// Called from makeDisplayText
void PlaylistItem::makeToolTip()
{
	QString s_tt = QString();
	
	// If the error string is not empty show the errors
	if (! isPlayable() ) {
		this->setToolTip(QString("<p style='white-space:pre'>%1").arg(errors));
		return;
	}
			
	// Otherwise tooltip should contain whatever information we can find about the uri
	s_tt.append(QObject::tr("<p style='white-space:pre'>Entry:"));
	switch (type()) {
		case MBMP_PL::File: s_tt.append(QObject::tr("<br> Type: Local file")); break;
		case MBMP_PL::Url: s_tt.append(QObject::tr("<br>  Type: URL")); break;
		case MBMP_PL::ACD: s_tt.append(QObject::tr("<br>  Type: Audio CD track")); break;
		case MBMP_PL::DVD: s_tt.append(QObject::tr("<br>  Type: DVD chapter")); break;
	}
	if (! uri.isEmpty() ) s_tt.append(QObject::tr("<br>  Uri: %1").arg(uri));
	
	s_tt.append(QObject::tr("<p style='white-space:pre'>Properties:"));
	if (duration > 0 ) {
		QTime n(0,0,0);
		QTime t;
		t = n.addSecs(duration);				
		s_tt.append(QObject::tr("<br>  Duration: %1").arg(duration > (60 * 60) ? t.toString("h:mm:ss") : t.toString("mm:ss")) );
	}
		
	s_tt.append(QObject::tr("<br>  Seekable: %1").arg(seekable ? QObject::tr("yes") : QObject::tr("no")) );

	if (tag_map.count() > 0) {
		s_tt.append(QObject::tr("<br>  Tags:") );
		
		// scan through tags find the ones we want to display
		QStringList blacklist;
		blacklist << GST_TAG_IMAGE << GST_TAG_PREVIEW_IMAGE << GST_TAG_LYRICS;
		QMapIterator<QString, QString> itr(tag_map);
		while (itr.hasNext()) {
			itr.next();
			if (! blacklist.contains(itr.key()) ) 
				s_tt.append(QString("<br>    %1 : %2").arg(itr.key()).arg(itr.value()) ) ;
		} // while
	}	// if taglist is not NULL

	this->setToolTip(s_tt);
	return;
}

//
// Function to run gstDiscoverer on the uri to try and get some information about the media.  
// Called from the constructor
void PlaylistItem::runDiscoverer()
{
	// Variables (actually pointers)
	GstDiscoverer* disc = NULL;
	GstDiscovererInfo* info = NULL;
	const GstTagList* tags = NULL;
	GError* err = NULL;
	
	// Create the discoverer
	disc = gst_discoverer_new (5 * GST_SECOND, &err); // timeout is 5 seconds
	if (! disc ) {
		errors.append(QObject::tr("Error creating a gst_discoverer instance: %1").arg(err->message) 
		);
		errors.append("\n");
		errors.append(QObject::tr("This URI may not be able to be played") );
		g_clear_error (&err);
		this->setForeground(Qt::yellow);
		return;
	}
	
	// Run discoverer on the url
	info = gst_discoverer_discover_uri(disc, qPrintable(uri), &err);
	
	// Process the dicoverer result
	GstDiscovererResult result;
	result = gst_discoverer_info_get_result(info);
	QString s0 = QString(QObject::tr("Discoverer Result: " ));
	switch (result)	{
		case GST_DISCOVERER_URI_INVALID:
			s0.append(QObject::tr("<br>Invalid URI: %1").arg(gst_discoverer_info_get_uri(info)) );
			s0.append("\n");
			break;
		case GST_DISCOVERER_ERROR:
			s0.append(QObject::tr("<br>Error: %1").arg(err->message) );
			s0.append("\n");
			g_clear_error (&err);				
			break;
		case GST_DISCOVERER_TIMEOUT:
			s0.append(QObject::tr("<br>Timeout"));
			s0.append("\n");
			break;
		case GST_DISCOVERER_BUSY:	
			s0.append(QObject::tr("<br>Discoverer Busy"));
			s0.append("\n");
			break;
		case GST_DISCOVERER_MISSING_PLUGINS:
			const GstStructure *s;
			gchar *str;
			s = gst_discoverer_info_get_misc (info);
			str = gst_structure_to_string (s);
			s0.append(QObject::tr("<br>Missing plugins: %1").arg(str) );
			s0.append("\n");
			g_free (str);
			break;
		case GST_DISCOVERER_OK:
			break;
		}	// switch
	  
	if (result != GST_DISCOVERER_OK) {
		errors.append(s0);
		errors.append(QObject::tr("This URI may not be able to be played") );
		gst_discoverer_info_unref(info);
		g_object_unref (disc);
		this->setForeground(Qt::red);
		return;
	}
	
	// Get information not in tags
	duration = gst_discoverer_info_get_duration(info) / (1000 * 1000 * 1000);
	seekable = static_cast<bool>(gst_discoverer_info_get_seekable(info) ); 		
	
	// Get tags and try to extract information from them
	tags = gst_discoverer_info_get_tags(info);	// tags belongs to info
	
	// Save the taglist in a QMap
	for (int i = 0; i < gst_tag_list_n_tags(tags); ++i) {
		GValue val = G_VALUE_INIT;
		gchar *str;
       
		gst_tag_list_copy_value (&val, tags, gst_tag_list_nth_tag_name(tags, i));                
			
		if (G_VALUE_HOLDS_STRING (&val))
			str = g_value_dup_string (&val);
		else
			str = gst_value_serialize (&val);
			
		tag_map[QString::fromUtf8(gst_tag_list_nth_tag_name(tags, i))] = QString::fromUtf8(str);
		g_free (str);
	} //for
	
	// Save selected values directly out of the taglist for display in the QListWidget 
	if (tags) {
		gchar* str = NULL;
		uint val = 0;		
		GstSample* sam = NULL;	
		
		if (gst_tag_list_get_string (tags, GST_TAG_TITLE, &str)) {
			if (str) title = QString(str);
		}
		
		if (gst_tag_list_get_string (tags, GST_TAG_ARTIST, &str)) {
			if (str) artist = QString(str);
		}
		
		if (gst_tag_list_get_string (tags, GST_TAG_LYRICS, &str)) {
			if (str) {
				lyrics = QString(str);
				b_has_lyrics = true;
			}	// if
		}
		
		if (gst_tag_list_get_string (tags, GST_TAG_ALBUM, &str)) {
			if (str) album = QString(str);
		}
		
		if (gst_tag_list_get_uint (tags, GST_TAG_TRACK_NUMBER, &val)) {
			if (val) sequence = val;
		}
		
		if (gst_tag_list_get_sample(tags, GST_TAG_IMAGE, &sam) || gst_tag_list_get_sample(tags, GST_TAG_PREVIEW_IMAGE, &sam) ) {	 		
			GstBuffer* buffer = gst_sample_get_buffer (sam); 
			gsize bufsize;
			gsize bufoff;
			bufsize = gst_buffer_get_sizes(buffer, &bufoff, NULL);
			
			void* dest = malloc (bufsize);
			gst_buffer_extract_dup(buffer, bufoff, bufsize, &dest, &bufsize);				
			
			pm_artwork.loadFromData( (uchar*)dest, bufsize);
			
			g_free(dest);
			gst_sample_unref(sam);
			b_has_artwork = ! pm_artwork.isNull();
		}	// if saving pixmap worked
		
	}	// if there were tags

	// clean up discoverer stuff
	gst_discoverer_info_unref(info);
	g_object_unref (disc);	
	
	return;
}
