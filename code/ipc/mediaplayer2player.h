/******************* medaiaplayer2player.h *****************************

Code for the MediaPlayer2.Player interface on DBus.  When registered MBMP
will communicate to other processes.  

Copyright (C) 2013-2017
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


# ifndef IPC_MEDIAPLAYER2PLAYER
# define IPC_MEDIAPLAYER2PLAYER

# include <gst/gst.h>

# include <QObject>
# include <QtDBus/QDBusContext>
# include <QtDBus/QDBusAbstractAdaptor>
# include <QVariant>
# include <QMap>
# include <QString>
# include <QVector>

# include "./code/resource.h"
# include "./mpris2.h"


class MediaPlayer2Player : public QDBusAbstractAdaptor
{
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", IPC_INTERFACE_MEDIAPLAYER2PLAYER)
    
  public:
    MediaPlayer2Player(Mpris2*);
    
    Q_PROPERTY (QString PlaybackStatus READ getPlaybackStatus);		
		Q_PROPERTY (QString LoopStatus	READ getLoopStatus WRITE setLoopStatus);
		Q_PROPERTY (double Rate	READ getPlaybackRate WRITE setPlaybackRate);		
		Q_PROPERTY (bool Shuffle READ getShuffle WRITE setShuffle);
		Q_PROPERTY (QMap<QString,QVariant> Metadata READ getMetadata);		
		Q_PROPERTY (double Volume	READ getVolume WRITE setVolume);		
		Q_PROPERTY (qlonglong Position	READ getPosition);		
		Q_PROPERTY (double MinimumRate READ getMinimumRate);		
		Q_PROPERTY (double MaximumRate READ getMaximumRate);		
		Q_PROPERTY (bool CanGoNext READ getCanGoNext);		
		Q_PROPERTY (bool CanGoPrevious READ getCanGoPrevious);		
		Q_PROPERTY (bool CanPlay READ getCanPlay);		
		Q_PROPERTY (bool CanPause READ getCanPause);		
		Q_PROPERTY (bool CanSeek READ getCanSeek);		
		Q_PROPERTY (bool CanControl READ getCanControl);			
		
	// get functions for memebers (properties)
		inline QString getPlaybackStatus() const {return playbackstatus;}
		inline QString getLoopStatus() const {return loopstatus;}
		inline double getPlaybackRate() const {return playbackrate;}
		inline bool getShuffle() const {return shuffle;}
		inline QMap<QString,QVariant> getMetadata() const {return metadata;}
		inline double getVolume() const {return volume;}
		inline qlonglong getPosition() const {return position;}
		inline double getMinimumRate() const {return minimumrate;}	
		inline double getMaximumRate() const {return maximumrate;}
		inline bool getCanGoNext() const {return cangonext;}
		inline bool getCanGoPrevious() const {return cangoprevious;}	
		inline bool getCanPlay() const {return canplay;}
		inline bool getCanPause() const {return canpause;}
		inline bool getCanSeek() const {return canseek;}
		inline bool getCanControl() const {return cancontrol;}

		// set functions for members.  most can or will never be used, they are here because we are supposed to emit a propertyChanged()
		// signal over dbus if one does change, 
		void setPlaybackStatus(const QString&);
		void setLoopStatus(const QString&);
		inline void setPlaybackRate(double d_r) {(void) d_r;}	// We don't allow changing the playback rate
		void setShuffle(const bool&);
		void setMetadata(const QVariantMap&); 
		void setVolume(const double&);
		inline void setPos(qlonglong pos) {position = pos;}	// propertyChanged not emitted when this changes
		inline void setMinimumRate(double d_mi) {(void) d_mi;}	// We don't allow changing the minimum rate
		inline void setMaximumRate(double d_mx) {(void) d_mx;}	// We don't allow changing the maximum rate
		void setCanGoNext(const bool& b_cgn); 		
		void setCanGoPrevious(const bool& b_cgp); 
		void setCanPlay(const bool& b_cpl);	
		void setCanPause(const bool& b_cpu);	
		void setCanSeek(const bool& b_s);	
		inline void setCanControl(bool b_ctl) {(void) b_ctl;}	// We don't allow changing the CanControl property
		inline void emitSeeked(const qlonglong& pos) {emit Seeked(pos);} 
		inline void clearMetaData() {metadata.clear();}

	// public slots (Q_SCRIPTABLE not actually needed) all slots published on the dbus interface
	public Q_SLOTS:
		Q_SCRIPTABLE void Next();
		Q_SCRIPTABLE void Previous();
		Q_SCRIPTABLE void Pause();
		Q_SCRIPTABLE void PlayPause();
		Q_SCRIPTABLE void Stop();
		Q_SCRIPTABLE void Play();
		Q_SCRIPTABLE void Seek(qlonglong);
		Q_SCRIPTABLE void SetPosition(QDBusObjectPath, qlonglong);
		Q_SCRIPTABLE void OpenUri(QString);
		
		// These are not part of the mpris2 specification.  They are to replace
		// functions I consider useful and which I lost when I converted from
		// my own IPC to the standard Mpris2 IPC.
		Q_SCRIPTABLE QString getTitle();
		Q_SCRIPTABLE QString getTrack();
		Q_SCRIPTABLE QString getArtist();
		Q_SCRIPTABLE int getDurationInSeconds();
		Q_SCRIPTABLE int getPositionInSeconds();
		Q_SCRIPTABLE void toggleConsumeMode();
		Q_SCRIPTABLE void toggleWrapMode();
		Q_SCRIPTABLE void toggleRandomMode();
		Q_SCRIPTABLE void toggleDetailMode();
		Q_SCRIPTABLE bool isPaused();
		Q_SCRIPTABLE bool isPlaying();
		Q_SCRIPTABLE bool isStopped();
			
	Q_SIGNALS:
		Q_SCRIPTABLE void Seeked(qlonglong);
		
	private:
		// data members
		QString playbackstatus;
		QString loopstatus;
		double playbackrate;
		bool shuffle;
		QMap<QString,QVariant> metadata;
		double volume;
		qlonglong position;
		double minimumrate;
		double maximumrate;
		bool cangonext;
		bool cangoprevious;
		bool canplay;
		bool canpause;
		bool canseek;
		bool cancontrol;
		
		QVector<int> changeditems;
		
		// functions
		void sendPropertyChanged();
};  

#endif
