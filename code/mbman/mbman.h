/**************************** mbman.h *****************************

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

# ifndef MUSICBRAINZ_MANAGER_H
# define MUSIZBRAINZ_MANAGER_H

# include <QNetworkAccessManager>
# include <QFile>
# include <QDir>

# include "./code/resource.h"

class MusicBrainzManager : public QNetworkAccessManager
{
  Q_OBJECT
  
  public:
  // members
    MusicBrainzManager(QObject* parent);  
    
	// functions
		void startLooking(const QString&, const QString&, const QString&);
		void retrieveCDMetaData(const QString&);
		void retrieveAlbumArt(const QString&, const QString&);
	
	private Q_SLOTS:
		void retrieveReleaseData();
		void releaseDataFinished();
		void metaDataFinished();
		void artworkRequestFinished();

	Q_SIGNALS:
		void metaDataRetrieved(const QString&);
		void artworkRetrieved();
		void abort();

  private:
		QFile destfile;
		QFile artfile;
		QDir artwork_dir;
		QDir cdmeta_dir;
		QString release;
		QString artist;
		QString releaseid;
		QString releasegrpid;
		short queryreq;
		const QString useragent = QString("%1/%2 (%3)").arg(LONG_NAME).arg(VERSION).arg(URL);
    
};

#endif
