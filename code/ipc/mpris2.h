/***************************** mpris2.h ********************************

Code for the MPRISv2.2 interface on DBus. Controls the interfaces registered
on /org/mpris/MediaPlayer2.  It is kind of a hack because QT does not
really support creating multiple interfaces on one object.  Some of the
broad outline for this layout came from the Amarok project, although I've
geatly modified the internal workings. 

Copyright (C) 2013-2018
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


# ifndef IPC_MPRIS2
# define IPC_MPRIS2

# include <QtCore/QObject>
# include <QtDBus/QtDBus>
# include <QtDBus/QDBusAbstractAdaptor>

# define IPC_SERVICE "org.mpris.MediaPlayer2.mbmp"
# define IPC_OBJECT "/org/mpris/MediaPlayer2"
# define IPC_INTERFACE_MEDIAPLAYER2 "org.mpris.MediaPlayer2"
# define IPC_INTERFACE_MEDIAPLAYER2PLAYER "org.mpris.MediaPlayer2.Player"

class Mpris2 : public QObject
{
  Q_OBJECT
    
	public:
		Mpris2 (QObject* parent = 0);
		inline void emitApplicationStop() {emit applicationStop();}
		inline void emitLoopStatusChanged(bool b_ls) {emit loopStatusChanged(b_ls);}
		inline void emitShuffleChanged(bool b_s) {emit shuffleChanged(b_s);}
		inline void emitVolumeChanged(int vol) {emit volumeChanged(vol);}
		inline void emitPlaylistNext() {emit playlistNext();}
		inline void emitPlaylistBack() {emit playlistBack();}
		inline void emitControlPause() {emit controlPause();}
		inline void emitControlPlayPause() {emit controlPlayPause();}
		inline void emitControlStop() {emit controlStop();}
		inline void emitControlPlay() {emit controlPlay();}
		inline void emitControlSeek(qlonglong offset) {emit controlSeek(offset);}
		inline void emitControlOpenUri(QString uri) {emit controlOpenUri(uri);}
		inline void emitControlToggleConsume() {emit controlToggleConsume();}
		inline void emitControlToggleWrap() {emit controlToggleWrap();}
		inline void emitControlToggleRandom() {emit controlToggleRandom();}
		inline void emitControlToggleDetail() {emit controlToggleDetail();}
		
	private:
		QDBusAbstractAdaptor* mediaplayer2;
		QDBusAbstractAdaptor* mediaplayer2player;
  
  // Slots and signals here are to relay informtion in and out of the 
  // adaptors.  A slot or signal in an adaptor will be published in
  // the adaptor and that is not usually correct.  These slots and signals
  // are connected to and from plain functions in the adaptor code
  public Q_SLOTS:
		// information we need to send to the mpris adaptors, mainly so they can
		// issue a propertyChanged signal
		void setState(const int&);
		void setLoopStatus(const bool&);
		void setShuffle(const bool&);
		void setMetadata(const QVariantMap&);
		void setVolume(const double&);
		void setPosition(const qlonglong&);
		void setCanGoNext(const bool&);
		void setCanGoPrevious(const bool&);
		void setCanPlay(const bool&);
		void setCanPause(const bool&);
		void setCanSeek(const bool&);
		void seeked(const qlonglong&);
		void clearMetaData();
   
  Q_SIGNALS:		
		// signals we need to issue back to the player from the adaptors
		// issue using the public inline functions above
		void applicationStop(); 
		void loopStatusChanged(bool);
		void shuffleChanged(bool);
		void volumeChanged(int);
		void playlistNext();
		void playlistBack();
		void controlPause();
		void controlPlayPause();
		void controlStop();
		void controlPlay();
		void controlSeek(qlonglong);
		void controlOpenUri(QString);
		void controlToggleConsume();
		void controlToggleWrap();
		void controlToggleRandom();
		void controlToggleDetail();
};		

#endif
