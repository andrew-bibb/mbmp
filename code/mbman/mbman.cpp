/**************************** mbman.cpp *****************************

Code to manage going out onto the internet to Musicbrainz and getting
meta data to include album art

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


# include "./mbman.h"
# include "./code/resource.h"

# include <QProcessEnvironment>
# include <QXmlStreamReader>
# include <QXmlStreamWriter>
# include <QUrl>
# include <QNetworkRequest>
# include <QNetworkReply>
# include <QImage>

// Constructor
MusicBrainzManager::MusicBrainzManager(QObject* parent) : QNetworkAccessManager(parent) 
{
	
  // Setup the data directories 
  // APP defined in resource.h
  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  QString home = env.value("HOME"); 
	artwork_dir = QDir(QString(env.value("XDG_DATA_HOME", QString(QDir::homePath())) + "/.local/share/%1/artwork").arg(QString(APP).toLower()) );
	if (! artwork_dir.exists()) artwork_dir.mkpath(artwork_dir.absolutePath() ); 
	cdmeta_dir = QDir(QString(env.value("XDG_DATA_HOME", QString(QDir::homePath())) + "/.local/share/%1/cdmeta").arg(QString(APP).toLower()) );
	if (! cdmeta_dir.exists()) cdmeta_dir.mkpath(cdmeta_dir.absolutePath() );	
	
	
	return;
}

//////////////////////////// Public Functions //////////////////////////
//
//	Function to retrieve metadata about an audio CD
void MusicBrainzManager::retrieveCDMetaData(const QString& discid)
{
	
	QNetworkRequest request;
	request.setUrl(QUrl(QString("http://musicbrainz.org/ws/2/discid/%1?inc=recordings+labels+release-groups+artists").arg(discid)) );
	
	// Store the data using the discid as the file name as opposed to releaseid or relgrpid since we only get here when
	// someone is playing an actual CD.  When they play it we get the discid as calculated by GStreamer (based on track
	// offsets and other things on the physical disc).  Releaseid and relgrpid are more universal, but to use them we'd
	// need to go online which kind of defeats the purpose of saving a file to avoid going online.   
	destfile.setFileName(cdmeta_dir.absoluteFilePath(QString(discid + ".xml")) );
	
	QNetworkReply* reply = this->get(request);
	connect(reply, SIGNAL(finished()), this, SLOT(metaDataFinished()));
		
	return;
}

//
// Function to retrieve album art if we can find it. Coverartarchive will do a redirect
// so this function has to deal with that.
void MusicBrainzManager::retrieveAlbumArt(const QString& relgrpid, const QString& albumid)
{
	QNetworkRequest request;
	request.setUrl(QUrl(QString("http://coverartarchive.org/release-group/%1/front").arg(relgrpid)) );      
	
	// Store the artwork using the albumid as the filename.
	artfile.setFileName(artwork_dir.absoluteFilePath(QString(albumid + ".jpg")) );
	QNetworkReply* reply = this->get(request);
	connect(reply, SIGNAL(finished()), this, SLOT(artworkRequestFinished()));
	
	return;
}

//////////////////////////// Public Slots //////////////////////////
//
void MusicBrainzManager::metaDataFinished()
{
	QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
	
	if (reply->error() != QNetworkReply::NoError) {
		#if QT_VERSION >= 0x050400 
			qCritical("Network error getting CD info from Musicbrainz:\n %s", qUtf8Printable(reply->errorString()) );
		# else	
			qCritical("Network error getting CD info from Musicbrainz:\n %s", qPrintable(reply->errorString()) );
		# endif
		emit metaDataRetrieved(QString() ) ; // used to cleanup the receiver
		return;
	}
	
	// Prepare to write the data to local storage, 
	QString discid = QString();
	if (destfile.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QXmlStreamWriter xmlwriter(&destfile);
		xmlwriter.setAutoFormatting(true);
		xmlwriter.writeStartDocument();
		
		//QString dtd;
		//dtd.append("<!DOCTYPE metadata\n");
		//dtd.append("[\n");
		//dtd.append("<!ELEMENT metadata (discid,title,date,status,label,tracklist)>\n");
		//dtd.append("<!ELEMENT discid (#PCDATA)>\n");
		//dtd.append("<!ELEMENT title (#PCDATA)>\n");
		//dtd.append("<!ELEMENT date (#PCDATA)>\n");
		//dtd.append("<!ELEMENT status (#PCDATA)>\n");
		//dtd.append("<!ELEMENT label (#PCDATA)>\n");
		//dtd.append("<!ELEMENT tracklist (track)>\n");
		//dtd.append("<!ELEMENT track (title, track_number, duration)>\n");
		//dtd.append("<!ELEMENT track_number (#PCDATA)>\n");
		//dtd.append("<!ELEMENT duration (#PCDATA)>\n");
		//dtd.append("]>");
		//xmlwriter.writeDTD(dtd);
		xmlwriter.writeStartElement("metadata");		
			
		// Read through the reply data and store pieces we want locally
		QXmlStreamReader* xml = new QXmlStreamReader(reply);	
		QStringList pos;
		while (! xml->atEnd() ) {	
			switch(xml->readNext() ) {
				case QXmlStreamReader::StartElement:
					pos.append(xml->name().toString() );
					//qDebug() << pos.join(',');
					if (pos.join(',') == "metadata,disc") {
						xmlwriter.writeTextElement("discid", xml->attributes().value("id").toString() );
						discid = xml->attributes().value("id").toString();
						}
					else if (pos.join(',') == "metadata,disc,release-list,release") {
						xmlwriter.writeTextElement("releaseid", xml->attributes().value("id").toString() );
						}	
					else if (pos.join(',') == "metadata,disc,release-list,release,release-group") {
						xmlwriter.writeTextElement("relgrpid", xml->attributes().value("id").toString() );
					}
					else if (pos.join(',') == "metadata,disc,release-list,release,title") {
						xmlwriter.writeTextElement("title", xml->readElementText(QXmlStreamReader::SkipChildElements) );
						pos.removeLast();
					}
					else if (pos.join(',') == "metadata,disc,release-list,release,date") {
						xmlwriter.writeTextElement("date", xml->readElementText(QXmlStreamReader::SkipChildElements) );
						pos.removeLast();
					}
					else if (pos.join(',') == "metadata,disc,release-list,release,status") {
						xmlwriter.writeTextElement("status", xml->readElementText(QXmlStreamReader::SkipChildElements) );
						pos.removeLast();
					}
					else if (pos.join(',') == "metadata,disc,release-list,release,label-info-list,label-info,label,name") {
						xmlwriter.writeTextElement("label", xml->readElementText(QXmlStreamReader::SkipChildElements) );	
						pos.removeLast();
					}
					else if (pos.join(',') == "metadata,disc,release-list,release,artist-credit,name-credit,artist,name") {
						xmlwriter.writeTextElement("artist", xml->readElementText(QXmlStreamReader::SkipChildElements) );	
						pos.removeLast();
					}
					else if (pos.join(',') == "metadata,disc,release-list,release,medium-list,medium,track-list,track") {
						xmlwriter.writeStartElement("", "tracklist");
						while (! xml->atEnd() ) {
							switch (xml->readNext() ) {
								case QXmlStreamReader::StartElement:
									pos.append(xml->name().toString() );
									//qDebug() << pos.join(',');
									if (pos.join(',') == "metadata,disc,release-list,release,medium-list,medium,track-list,track,number") {
										xmlwriter.writeStartElement("", "track");
										xmlwriter.writeTextElement("track_number", xml->readElementText(QXmlStreamReader::SkipChildElements) );
										pos.removeLast();
									}
									if (pos.join(',') == "metadata,disc,release-list,release,medium-list,medium,track-list,track,recording,title") {
										xmlwriter.writeTextElement("title", xml->readElementText(QXmlStreamReader::SkipChildElements) );
										pos.removeLast();
									}
									if (pos.join(',') == "metadata,disc,release-list,release,medium-list,medium,track-list,track,recording,length") {
										xmlwriter.writeTextElement("duration", xml->readElementText(QXmlStreamReader::SkipChildElements) );
										pos.removeLast();
									}   
									break;	
								case QXmlStreamReader::EndElement:
									if (xml->name() == "recording") {
										xmlwriter.writeEndElement();	// track	
									}
									pos.removeLast();
									//qDebug() << pos.join(',');
									break;	
								default:
									continue;
							}	// switch
							
							if (xml->tokenType() == QXmlStreamReader::EndElement && xml->name() == "track-list") {
								xmlwriter.writeEndElement();	// tracklist
								break;	// out of inner while
							}	// if end of tracklist
						}	// while
					}	// if track-list,track
					break;	// startElement
							
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
		xmlwriter.writeEndDocument();
		destfile.close();
		delete xml;
	}	// if destfile could be opened
	
	// cleanup
	emit metaDataRetrieved(discid);
	reply->deleteLater();
	return;	
}

void MusicBrainzManager::artworkRequestFinished()
{
	QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
	
	if (reply->error() != QNetworkReply::NoError) {
		#if QT_VERSION >= 0x050400 
			qCritical("Network error getting album art from CoverArtArchive:\n %s", qUtf8Printable(reply->errorString()) );
		# else	
			qCritical("Network error getting album art from CoverArtArchive:\n %s", qPrintable(reply->errorString()) );
		# endif
		return;
	}
	
	// check for the redirection
	if(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 302 || reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 307 ) {
		connect (get(QNetworkRequest(reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl())),
					SIGNAL(finished()),
					this,
					SLOT(artworkRequestFinished()) );
	}	// if
	else {
		QImage img = QImage::fromData(reply->readAll() );
		if (img.height() > 500 || img.width() > 500)
			img = img.scaled(QSize(500, 500), Qt::KeepAspectRatio, Qt::SmoothTransformation);
		img.save(artfile.fileName(), "JPG");
	}	// else

	reply->deleteLater();
	return;
}

	
