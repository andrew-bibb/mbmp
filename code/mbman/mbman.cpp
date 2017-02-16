/**************************** mbman.cpp *****************************

Code to manage going out onto the internet to Musicbrainz and getting
meta data to include album art

Copyright (C) 2014-2016
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

# include <QProcessEnvironment>
# include <QXmlStreamReader>
# include <QXmlStreamWriter>
# include <QUrl>
# include <QUrlQuery>
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
	artwork_dir = QDir(QString(env.value("XDG_DATA_HOME", QString(QDir::homePath()) + "/.local/share") + "/%1/artwork").arg(QString(APP).toLower()) );
	if (! artwork_dir.exists()) artwork_dir.mkpath(artwork_dir.absolutePath() ); 
	cdmeta_dir = QDir(QString(env.value("XDG_DATA_HOME", QString(QDir::homePath()) + "/.local/share") + "/%1/cdmeta").arg(QString(APP).toLower()) );
	if (! cdmeta_dir.exists()) cdmeta_dir.mkpath(cdmeta_dir.absolutePath() );	
	
	release.clear();
  artist.clear();
  title.clear();
  releaseid.clear();
  trackid.clear();
  releasegrpid.clear();
  queryreq = 0;
	
	return;
}

//////////////////////////// Public Functions //////////////////////////
//
//
// Function to start looking for metadata about a track based on track title, artist or releaseid
// If we have releaseid use that.  If not try title and artist, if that fails try title only 
void MusicBrainzManager::startLooking(const QString& rel, const QString& ast, const QString& tit, const QString& relid, const QString& trkid)
{
	// abort any active network requests, downloads, etc.
	this->emit abort();
	
	// if release is empty return - we can't do anything without the release name
	if (rel.isEmpty() ) return;
	
	// save data sent
	release = rel;
	artist = ast;
	title = tit;
	releaseid = relid;
	trackid = trkid;
	releasegrpid.clear();
	queryreq = 0;
		
	// start quering the MusicBrainz database based on what data we have so far
	++queryreq;
	if (! releaseid.isEmpty() ) {
		retrieveReleaseData();
		return;
	}
	
	++queryreq;
	if (! trackid.isEmpty() ) {
		retrieveReleaseData();
		return;
	}
	
	++queryreq;
	if (! release.isEmpty() && ! artist.isEmpty() ) {
		retrieveReleaseData();
		return;
	}
	
	++queryreq;
	if (! release.isEmpty() && ! title.isEmpty() ) {
		retrieveReleaseData();
		return;
	}
	
	++queryreq;
	if (! release.isEmpty() ) {
		retrieveReleaseData();
		return;
	}
	
	qCritical("No release or releaseid provided - cannot continue looking for album art");
	return;	
}

//////////////////////////// Private Slots //////////////////////////
//
// Function to retrieve the release data based on information we have in hand. 
// Called from startLooking() and releaseDataFinished().
void MusicBrainzManager::retrieveReleaseData()
{
	QString rel = release;
	QString ast = artist;
	QString tit = title;
	
	// Process special characters for Lucene
	QStringList sl_specials;
	sl_specials << "+"  << "-" << "&&" << "||" << "!" << "(" << ")" << "{" << "}"  << "[" << "]" << "^" << "\"" << "~" << "*" << "?" << ":" << "\\" << "/";
	for (int i = 0; i < sl_specials.count(); ++i) {
		rel.replace(sl_specials.at(i), QString("%5C" + sl_specials.at(i)) );
		ast.replace(sl_specials.at(i), QString("%5C" + sl_specials.at(i)) );
		tit.replace(sl_specials.at(i), QString("%5C" + sl_specials.at(i)) );
	}
	
			
	// Create the URL
	QUrl url;
	url.setScheme("http");
	url.setHost("musicbrainz.org");
	QUrlQuery urlq;
	const QString dquote("\"");
	switch (queryreq)
	{
		case 1:
			url.setPath(QString("/ws/2/release") );
			urlq.addQueryItem("query", QString("reid:" + releaseid) );
			break;
		case 2:	
			url.setPath(QString("/ws/2/recording") );
			urlq.addQueryItem("query", QString("rid:" + trackid) );
			break;	
		case 3:	//  	
			url.setPath(QString("/ws/2/release") );
			urlq.addQueryItem("query", QString("release:" + dquote + rel + dquote + " AND " + "artist:" + dquote + ast + dquote) );
			break;
		case 4: 	
			url.setPath(QString("/ws/2/recording") );
			urlq.addQueryItem("query", QString("release:" + dquote + rel + dquote + " AND " + "recording:" + dquote + tit + dquote) );
			break;		
		case 5:
			url.setPath(QString("/ws/2/release") );
			urlq.addQueryItem("query", QString("release:" + dquote + rel + dquote) );
			break;
		default:
			return;	
	}	// switch
	
	url.setQuery(urlq);	
	
	// Create the request
	QNetworkRequest request;
	request.setUrl(url);
	request.setRawHeader("User-Agent", useragent.toLatin1());
qDebug() << url;
	#if QT_VERSION >= 0x050400 
		qInfo("Search Case %i - Retrieving database information from Musicbrainz for release %s by %s.\n", queryreq, qUtf8Printable(release), qUtf8Printable(artist) );
	# else	
		qInfo("Search Case %i Retrieving database information from Musicbrainz for release %s by %s.\n", queryreq, qPrintable(release), qPrintable(artist) );
	# endif
	
	//qDebug() << url;
	// Create and connect the reply message to the processing slot
	QNetworkReply* reply = this->get(request);
	connect(this, SIGNAL(abort()), reply, SLOT(abort()));
	connect(reply, SIGNAL(finished()), this, SLOT(releaseDataFinished()));
	
	return;
}

//
//	Function to retrieve metadata about an audio CD
void MusicBrainzManager::retrieveCDMetaData(const QString& discid)
{
	
	// abort any active network requests, downloads, etc.
	this->emit abort();
	
	QNetworkRequest request;
	request.setUrl(QUrl(QString("http://musicbrainz.org/ws/2/discid/%1?inc=recordings+labels+release-groups+artists").arg(discid)) );
	
	// Store the data using the discid as the file name as opposed to releaseid or releasegrpid since we only get here when
	// someone is playing an actual CD.  When they play it we get the discid as calculated by GStreamer (based on track
	// offsets and other things on the physical disc).  Releaseid and releasegrpid are more universal, but to use them we'd
	// need to go online which kind of defeats the purpose of saving a file to avoid going online.   
	destfile.setFileName(cdmeta_dir.absoluteFilePath(QString(discid + ".xml")) );
	
	request.setRawHeader("User-Agent", useragent.toLatin1());
	QNetworkReply* reply = this->get(request);
	connect(this, SIGNAL(abort()), reply, SLOT(abort()));
	connect(reply, SIGNAL(finished()), this, SLOT(metaDataFinished()));
		
	return;
}

//
// Function to retrieve album art if we can find it. Coverartarchive will do a redirect
// so this function has to deal with that by connecting to artworkRequestFinished()
void MusicBrainzManager::retrieveAlbumArt(const QString& releasegrpid, const QString& savename)
{
	QNetworkRequest request;
	request.setUrl(QUrl(QString("http://coverartarchive.org/release-group/%1/front").arg(releasegrpid)) );      
	
	// Store the artwork using savename 
	artfile.setFileName(artwork_dir.absoluteFilePath(QString(savename + ".jpg")) );

	request.setRawHeader("User-Agent", useragent.toLatin1());
	QNetworkReply* reply = this->get(request);
	connect(this, SIGNAL(abort()), reply, SLOT(abort()));
	connect(reply, SIGNAL(finished()), this, SLOT(artworkRequestFinished()));
	
	return;
}

// 
// Parse XML returned from Musicbrainz looking for releasegroupid,
// Called via signal/slot from retrieveReleaseData().  
void MusicBrainzManager::releaseDataFinished()
{
	QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
	
	if (reply->error() != QNetworkReply::NoError) {
		#if QT_VERSION >= 0x050400 
			qCritical("Network error getting XML info from:%s\n %s", qUtf8Printable(reply->url().toString()), qUtf8Printable(reply->errorString()) );
		# else	
			qCritical("Network error getting XML info from:%s\n %s", qPrintable(reply->url().toString()), qPrintable(reply->errorString()) );
		# endif
		reply->deleteLater();
		return;
	}

	// Read through the reply data and store pieces we want locally
	QXmlStreamReader* xml = new QXmlStreamReader(reply);	
	QStringList pos;
	while (! xml->atEnd() ) {	
		switch(xml->readNext() ) {
			case QXmlStreamReader::StartElement:
				pos.append(xml->name().toString() );
				//qDebug() << pos.join(',');

				// Note that it is not possible to get a releaseid from artist and title.  There could
				// be multiple releases for this case, so starting with only with artist and title we can
				// only retrieve the releasegrpid. 
				//
				// query /ws/2/release
				if (pos.join(',') == "metadata,release-list,release,release-group") {
					if (releasegrpid.isEmpty() ) releasegrpid = xml->attributes().value("id").toString();
				}
				
				//else if (pos.join(',') == "metadata,release-list,release-group") {
					//if (releasegrpid.isEmpty() ) releasegrpid = xml->attributes().value("id").toString();
				//}
				
				// query /ws/2/recording
				else if (pos.join(',') == "metadata,recording-list,recording,release-list,release,release-group") {
					if (releasegrpid.isEmpty() ) releasegrpid = xml->attributes().value("id").toString();
					pos.removeLast();
				}
				
				break;	// startElement
						
			case QXmlStreamReader::EndElement:	
				if (! pos.isEmpty() ) pos.removeLast();			
				//qDebug() << pos.join(',');
				break;	
				
			case QXmlStreamReader::Invalid:
				#if QT_VERSION >= 0x050400 
					qCritical("XML stream reading error: %s %s", qUtf8Printable(xml->error()), qUtf8Printable(xml->errorString()) );
				# else	
					qCritical("XML stream reading error: %s %s", qPrintable(xml->error()), qPrintable(xml->errorString()) );
				# endif
				break;
		
			case QXmlStreamReader::EndDocument:
				break;
				
			default:
				continue;
		}	// switch
		
		if (! releasegrpid.isEmpty() ) break;	// break once we've got what we want (and the first instance of same)
	}	// while	
	delete xml;

	// Get Album art for the releasegrpid or try another query
	if (! releasegrpid.isEmpty() && ! releaseid.isEmpty() )
		retrieveAlbumArt(releasegrpid, releaseid);
	else if (! releasegrpid.isEmpty() && ! release.isEmpty() ) 	
		retrieveAlbumArt(releasegrpid, release);
	else {
			qCritical("Search case %i - Unable to extract XML data returned from Musicbrainz.", queryreq );
			++ queryreq;
			retrieveReleaseData();
	}	
	
	// cleanup
	reply->deleteLater();
	return;
}

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
		reply->deleteLater();
		emit metaDataRetrieved(QString() ) ; // used to cleanup the receiver
		return;
	}
	
	// Prepare to write the data to local storage, 
	QString discid = QString();
	if (destfile.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QXmlStreamWriter xmlwriter(&destfile);
		xmlwriter.setAutoFormatting(true);
		xmlwriter.writeStartDocument();
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
						xmlwriter.writeTextElement("releasegrpid", xml->attributes().value("id").toString() );
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
			if (xml->tokenType() == QXmlStreamReader::EndElement && xml->name() == "release") break;	// break while after first release group is read
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

//
// Slot called from retrieveAlbumArt when the reply is finished
void MusicBrainzManager::artworkRequestFinished()
{
	QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
	
	if (reply->error() != QNetworkReply::NoError) {
		#if QT_VERSION >= 0x050400 
			qCritical("Network error getting http redirect from CoverArtArchive:\n %s", qUtf8Printable(reply->errorString()) );
		# else	
			qCritical("Network error getting http redirect from CoverArtArchive:\n %s", qPrintable(reply->errorString()) );
		# endif
		reply->deleteLater();
		return;
	}

	// save the return code
	int rtncode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	
	// check for the redirection
	if(rtncode == 302 || rtncode == 307 ) {
		connect (get(QNetworkRequest(reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl())),
					SIGNAL(finished()),
					this,
					SLOT(artworkRequestFinished()) );
	}	// if
	
	else {
		if (rtncode == 400 || rtncode == 404 || rtncode == 405 || rtncode == 503) 
			qCritical("Error retrieving album art: HTTP reply code %i\n", rtncode  );
		
		else {	
		QImage img = QImage::fromData(reply->readAll() );
		if (img.height() > 500 || img.width() > 500)
			img = img.scaled(QSize(500, 500), Qt::KeepAspectRatio, Qt::SmoothTransformation);
		img.save(artfile.fileName(), "JPG");
		emit artworkRetrieved();
		}	// else
	} //else

	reply->deleteLater();
	return;
}

	
