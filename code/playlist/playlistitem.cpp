/************************ playlistitem.cpp *****************************

Playlist items, derived from QListWidgetItem

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

# include "./code/playlist/playlistitem.h"
# include "./code/playlist/playlist.h"

# include <QtCore/QDebug>
# include <QTime>


// use GStreamer to process media tags
# include <gst/gst.h>
# include <gst/tag/tag.h>
#include <gst/pbutils/pbutils.h>

PlaylistItem::PlaylistItem(const QString& text, QListWidget* parent, int type, uint seq, int dur) : QListWidgetItem(text, parent, type)
{
	// data members
	errors = QString();
	sequence = seq;
	uri = QString();	
	duration = dur;
	seekable = false;		
	title = QString();
	artist = QString();
	album = QString();
	
	
	// Somewhat of a hack, but to display text in nice columns we either need a full QTableWidget (or worse a QTableView),
	// or QListWidgetItems with monospace text.  Since we're only looking for appearance, not function, monospace fonts
	// here we come.
	QFont font("Monospace");
	font.setStyleHint(QFont::Monospace);
	this->setFont(font);
		
	// process based on the item type
	switch (type) {
		// For Files try to determine some track information to display.
		case MBMP_PL::File: {
			// Constants
			const short wcol1 = -10;
			const short wcol2 = -25;	
			const short wcol3 = -60;	
			
			// Text should be the absolute path
			uri = QString("file://" + text);
					
			// variables (actually pointers)
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
				g_clear_error (&err);
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
					s0.append(QObject::tr("Invalid URI: %1").arg(gst_discoverer_info_get_uri(info)) );
					s0.append("\n");
					break;
				case GST_DISCOVERER_ERROR:
					s0.append(QObject::tr("Error: %1").arg(err->message) );
					s0.append("\n");
					g_clear_error (&err);				
					break;
				case GST_DISCOVERER_TIMEOUT:
					s0.append(QObject::tr("Timeout"));
					s0.append("\n");
					break;
				case GST_DISCOVERER_BUSY:	
					s0.append(QObject::tr("Discoverer Busy"));
					s0.append("\n");
					break;
				case GST_DISCOVERER_MISSING_PLUGINS:
					const GstStructure *s;
					gchar *str;
					s = gst_discoverer_info_get_misc (info);
					str = gst_structure_to_string (s);
					s0.append(QObject::tr("Missing plugins: %1").arg(str) );
					s0.append("\n");
					g_free (str);
					break;
				case GST_DISCOVERER_OK:
					break;
				}	// switch
			  
			if (result != GST_DISCOVERER_OK) {
				errors.append(s0);
				errors.append(QObject::tr("This URI cannot be played") );
				gst_discoverer_info_unref(info);
				g_object_unref (disc);
				return;
			}
			
			// Get information not in tags
			duration = gst_discoverer_info_get_duration(info) / (1000 * 1000 * 1000);
			seekable = static_cast<bool>(gst_discoverer_info_get_seekable(info) ); 		
			
			// Get the Tags
			tags = gst_discoverer_info_get_tags(info);
			if (tags) {
				gchar* str = NULL;
				uint val = 0;			
				
				if (gst_tag_list_get_string (tags, GST_TAG_TITLE, &str)) {
					if (str) title = QString(str);
					g_free (str);
				}
				
				if (gst_tag_list_get_string (tags, GST_TAG_ARTIST, &str)) {
					if (str) artist = QString(str);
					g_free (str);
				}
				
				if (gst_tag_list_get_string (tags, GST_TAG_ALBUM, &str)) {
					if (str) album = QString(str);
					g_free (str);
				}
				
				if (gst_tag_list_get_uint (tags, GST_TAG_TRACK_NUMBER, &val)) {
					if (val) sequence = val;
				}
			}	// if there were tags
			
			// Put the duration in seconds into a QTime
			QTime n(0,0,0);
			QTime t;
			t = n.addSecs(duration);
	
			// Set the display text
			if (duration < 0 ) {
				if (title.isEmpty()) this->setText(text); 
				else this->setText(title.simplified());
			}	// if there is no duration
			else {
				if (title.isEmpty()) {
					this->setText(QString("%1%2")
						.arg(duration > (60 * 60) ? t.toString("h:mm:ss") : t.toString("mm:ss"), wcol1, QChar(' '))
						.arg(text) );
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
			
			// clean up discoverer stuff
			gst_discoverer_info_unref(info);
			g_object_unref (disc);
	
			break; }	// case PlaylistItem is a file
	
		// For DVD's display text is based the text sent as an argument and the chapter number
		case MBMP_PL::DVD: {
			// Constants
			const short wcol1 = -4;
	
			// Set the display text
			setText(QString("%1%2-%3")
				.arg(QString::number(sequence, 10), wcol1, QChar(' ') )
				.arg(text) 
				.arg(sequence) );
			break; }	// case PlaylistItem is a DVD chapter
			
		// For CD's display text is based on track duration	
		case MBMP_PL::ACD: {
			// Constants
			const short wcol1 = -8;
			
			// Set the display text is based on text sent as an argument and duration
			QTime n(0,0,0);
			QTime t = n.addSecs(duration);
			this->setText(QString("%1%2")
				.arg(duration > (60 * 60) ? t.toString("h:mm:ss") : t.toString("mm:ss"), wcol1, QChar(' '))
				.arg(text) );
			break; }	// case PlaylistItem is Audio CD track
			
		// for URL's really just need to show the URL	
		case MBMP_PL::Url: {
			break; }	// case PlaylistItem is URL	
	
	}	// switch
	
	
	return;
}
