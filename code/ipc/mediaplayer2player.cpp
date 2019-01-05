/********************* mediaplayer2player.cpp ***************************

Code for the MediaPlayer2.Player interface on DBus.  When registered MBMP
will communicate to other processes.  

Copyright (C) 2013-2019
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

# include <QtCore/QDebug>
# include <QtDBus/QDBusConnection>
# include <QtDBus/QDBusMessage>

# include "./mediaplayer2player.h"

//  constructor
MediaPlayer2Player::MediaPlayer2Player(Mpris2* parent) : QDBusAbstractAdaptor(parent)
{

	// initialize player properties
	playbackstatus = QString("Stopped");
	loopstatus = QString("None");
	playbackrate = 1.0;
	shuffle = false;
	metadata.clear();
	volume = 0.0;
	position = 0.0;
	minimumrate = 1.0;
	maximumrate = 1.0;
	cangonext = false;
	cangoprevious = false;
	canplay = false;
	canpause = false;
	canseek = false;
	cancontrol = true;
	
	// data members
	changeditems.clear();	
	
}
 
/////////////////////// Public Functions //////////////////////////////
//
// Function to set the Playback status. 
// dbus read only
void MediaPlayer2Player::setPlaybackStatus(const QString& s_ps)
{
	const QStringList valid{"Playing", "Paused", "Stopped"};
	
	// return if no change
	if (playbackstatus == s_ps) return;
	
	// return if not valid
	if (! valid.contains(s_ps, Qt::CaseSensitive) ) return;
	
	// changed and valid
	playbackstatus = s_ps;
	changeditems.append(MBMP_MPRIS::PlaybackStatus);
	sendPropertyChanged();
}

//
// Function to set the Loop status. 
// dbus read/write
void MediaPlayer2Player::setLoopStatus(const QString& s_ls)
{
	const QStringList valid{"None", "Track", "Playlist"};
	
	// return if no change
	if (loopstatus == s_ls) return;
	
	// return if not valid
	if (! valid.contains(s_ls, Qt::CaseSensitive) ) return;
	
	// MBMP does not support Track repeat, return if set (via dbus interface)
	if (s_ls == "Track") return;
	
	// changed and valid
	loopstatus = s_ls;
	changeditems.append(MBMP_MPRIS::LoopStatus);
	sendPropertyChanged();
	
	// let MBMP know we've changed (change could have been via the mpris2 dbus interface)
	if (s_ls == "Playlist")
		static_cast<Mpris2*>(this->parent())->emitLoopStatusChanged(true);
	else
		static_cast<Mpris2*>(this->parent())->emitLoopStatusChanged(false);
}

//
// Function to set the Shuffle property. 
// dbus read/write
void MediaPlayer2Player::setShuffle(const bool& b_s)
{
	// return if no change
	if (shuffle == b_s) return;
	
	// changed and valid
	shuffle = b_s;
	changeditems.append(MBMP_MPRIS::Shuffle);
	sendPropertyChanged();
	
	// let MBMP know we've changed (change could have been via the mpris2 dbus interface)
	static_cast<Mpris2*>(this->parent())->emitShuffleChanged(b_s);
}

//
// Function to set the Metadata property
// dbus read only
void MediaPlayer2Player::setMetadata(const QVariantMap& vmap)
{
	// new data
	metadata = vmap;
	changeditems.append(MBMP_MPRIS::Metadata);
	sendPropertyChanged();
	
	return;
}
	
//
// Function to set the Shuffle property. 
// dbus read/write
void MediaPlayer2Player::setVolume(const double& d_v)
{
	// return if no change
	if (volume == d_v) return;
	
	// volume changed
	if (d_v < 0.0 ) volume = 0.0;
		else if (d_v > 1.0) volume = 1.0;
			else volume = d_v;	
	
	// volume changed
	changeditems.append(MBMP_MPRIS::Volume);
	sendPropertyChanged();

	// let MBMP know we've changed (change could have been via the mpris2 dbus interface)
	static_cast<Mpris2*>(this->parent())->emitVolumeChanged(static_cast<int>(d_v * 30.0 + 0.5) );
}

//
// Function to set the CanGoNext property
// Dbus read only
void MediaPlayer2Player::setCanGoNext(const bool& b_cgn)
{
	if (cangonext == b_cgn) return;
	
	cangonext = b_cgn;
	changeditems.append(MBMP_MPRIS::CanGoNext);
	sendPropertyChanged();
	
	return;
}

//
// Function to set the CanGoPrevious property
// Dbus read only
void MediaPlayer2Player::setCanGoPrevious(const bool& b_cgp)
{
	if (cangoprevious == b_cgp) return;
	
	cangoprevious = b_cgp;
	changeditems.append(MBMP_MPRIS::CanGoPrevious);
	sendPropertyChanged();
	
	return;
}

//
// Function to set the CanPlay property
// Dbus read only
void MediaPlayer2Player::setCanPlay(const bool& b_cpl)
{
	if (canplay == b_cpl) return;
	
	canplay = b_cpl;
	changeditems.append(MBMP_MPRIS::CanPlay);
	sendPropertyChanged();
	
	return;
}

//
// Function to set the CanPause property
// Dbus read only
void MediaPlayer2Player::setCanPause(const bool& b_cpu)
{
	if (canpause == b_cpu) return;
	
	canpause = b_cpu;
	changeditems.append(MBMP_MPRIS::CanPause);
	sendPropertyChanged();
	
	return;
}

//
// Function to set the CanSeek property
// Dbus read only
void MediaPlayer2Player::setCanSeek(const bool& b_s)
{
	if (canseek == b_s) return;
	
	canseek = b_s;
	changeditems.append(MBMP_MPRIS::CanSeek);
	sendPropertyChanged();
	
	return;
}

/////////////////////// Public Slots///// //////////////////////////////
//
// Slots will be visible in the dbus interface

//
// Slot to issue a Next command
void MediaPlayer2Player::Next()
{
	if (cangonext) 
		static_cast<Mpris2*>(this->parent())->emitPlaylistNext();
		
	return;
}

//
// Slot to issue a Previous command
void MediaPlayer2Player::Previous()
{
	if (cangoprevious)
		static_cast<Mpris2*>(this->parent())->emitPlaylistBack();
	
	return;
}

//
// Slot to issue a Pause command
void MediaPlayer2Player::Pause()
{
	if (canpause)
		static_cast<Mpris2*>(this->parent())->emitControlPause();
	
	return;
}

//
// Slot to issue a Play/Pause (toggle) command
void MediaPlayer2Player::PlayPause()
{
		static_cast<Mpris2*>(this->parent())->emitControlPlayPause();
	
	return;
}

//
// Slot to issue a stop command
void MediaPlayer2Player::Stop()
{
	if (canpause)
		static_cast<Mpris2*>(this->parent())->emitControlStop();
	
	return;
}

//
// Slot to issue a play command
void MediaPlayer2Player::Play()
{
	if (canplay)
		static_cast<Mpris2*>(this->parent())->emitControlPlay();
	
	return;
}

//
// Slot to issue a seek foward or backward command
// offset is the amount foward (+) or backward (-) to 
// seek in microseconds
void MediaPlayer2Player::Seek(qlonglong offset)
{
	if (canseek)
		static_cast<Mpris2*>(this->parent())->emitControlSeek(offset);
	
	return;
}

// Slot to issue a seek to position command.  pos is the stream position
// in microsecods.
void MediaPlayer2Player::SetPosition(QDBusObjectPath obj, qlonglong pos)
{
	// first make sure function received valid arguments
	if (! metadata.contains("mpris:trackid") ) return;
	if (! metadata.contains("mpris:length") ) return;
	if (obj != metadata["mpris:trackid"].value<QDBusObjectPath>() ) return;
	if (pos < 0 || pos > metadata["mpris:length"].value<qlonglong>() ) return;
	
	// all test passed
	this->Seek(pos - position);
		
	return;
}

// Slot to issue a OpenUri command. 
void MediaPlayer2Player::OpenUri(QString uri)
{
	// uri validation is done in playlist addURI() function
	static_cast<Mpris2*>(this->parent())->emitControlOpenUri(uri);
		
	return;
}	

// Slots below are not part of the mpris2 specification.  They are to replace
// functions I consider useful and which I lost when I converted from
// my own IPC to the standard Mpris2 IPC.

//
// Slot to return the current artist
QString MediaPlayer2Player::getArtist()
{
	return metadata.contains("xesam:artist") ? metadata["xesam:artist"].value<QString>() : QString();
}

// Title and track could be different.  Title is read from the tags in
// the playlist.  Track is read from the message when GStreamer issues
// a NewTrack signal. For instance an internet radio stream will not
// have any tags in the playlist, but will (may?) have information
// we want when it fires a NewTrack signal.
//
// Slot to return the current title
QString MediaPlayer2Player::getTitle()
{
	return metadata.contains("xesam:title") ? metadata["xesam:title"].value<QString>() : QString();
}

//
// Slot to return the current track.
QString MediaPlayer2Player::getTrack()
{
	return metadata.contains("mbmp:track") ? metadata["mbmp:track"].value<QString>() : QString();
}

//
// Notes for the xxxInSeconds() functions:  Mpris2 has all internal duration and
// position values as microseconds.  GStreamer has the same in nanoseconds, and we typically
// use full seconds (although I would not be surprised to find millseconds here and there).

// Slot to return the current duration in seconds.
int MediaPlayer2Player::getDurationInSeconds()
{
	return metadata.contains("mpris:length") ? static_cast<int>(metadata["mpris:length"].value<qlonglong>() / (1000 * 1000)) : 0;
}

//
// Slot to return the current position in seconds. 
int MediaPlayer2Player::getPositionInSeconds()
{
	return static_cast<int>(position / (1000 * 1000));
}


//
// Slot to toggle the consume checkbox in the playlist
void MediaPlayer2Player::toggleConsumeMode()
{
	static_cast<Mpris2*>(this->parent())->emitControlToggleConsume();
		
	return;
}

//
// Slot to toggle the wrap checkbox in the playlist
void MediaPlayer2Player::toggleWrapMode()
{
	static_cast<Mpris2*>(this->parent())->emitControlToggleWrap();
		
	return;
}

//
// Slot to toggle the random checkbox in the playlist
void MediaPlayer2Player::toggleRandomMode()
{
	static_cast<Mpris2*>(this->parent())->emitControlToggleRandom();
		
	return;
}

//
// Slot to toggle the detail checkbox in the playlist
void MediaPlayer2Player::toggleDetailMode()
{
	static_cast<Mpris2*>(this->parent())->emitControlToggleDetail();
		
	return;
}

//
// Slot to return a bool based on the paused state
bool MediaPlayer2Player::isPaused()
{
	return playbackstatus == "Paused" ? true : false;
}

//
// Slot to return a bool based on the playing state
bool MediaPlayer2Player::isPlaying()
{
	return playbackstatus == "Playing" ? true : false;
}

//
// Slot to return a bool based on the stopped state
bool MediaPlayer2Player::isStopped()
{
	return playbackstatus == "Stopped" ? true : false;
}

/////////////////////// Private Functions //////////////////////////////
//
// Function to emit the org.freedesktop.Dbus.Properties.PropertiesChanged()
// DBus signal.  Called from the local inline setxxx functions
void MediaPlayer2Player::sendPropertyChanged()
{
	// changed properties
	if (changeditems.size() <= 0) return;  

	// create a map of all changed items. Use for loop because it is possible
	// (but highly unlikely) that this slot could be called with more than 
	// one property needing to be changed 
  QMap<QString, QVariant> vmap;
  for (int i = 0; i < changeditems.size(); ++i) {
		switch (changeditems.at(i) ) {
			case MBMP_MPRIS::PlaybackStatus:
				vmap["PlaybackSatus"] = QVariant(playbackstatus);
				break;
			case MBMP_MPRIS::LoopStatus:
				vmap["LoopSatus"] = QVariant(loopstatus);
				break;
			case MBMP_MPRIS::PlaybackRate:
				vmap["Rate"] = QVariant(playbackrate);
				break;
			case MBMP_MPRIS::Shuffle:
				vmap["Shuffle"] = QVariant(shuffle);
				break;
			case MBMP_MPRIS::Metadata:
				vmap["Metadata"] = QVariant(metadata);
				break;
			case MBMP_MPRIS::Volume:
				vmap["Volume"] = QVariant(volume);
				break;
			case MBMP_MPRIS::MinRate:
				vmap["MinimumRate"] = QVariant(minimumrate);
				break;
			case MBMP_MPRIS::MaxRate:
				vmap["MaximumRate"] = QVariant(maximumrate);
				break;
			case MBMP_MPRIS::CanGoNext:
				vmap["CanGoNext"] = QVariant(cangonext);
				break;
			case MBMP_MPRIS::CanGoPrevious:
				vmap["CanGoPrevious"] = QVariant(cangoprevious);
				break;
			case MBMP_MPRIS::CanPlay:
				vmap["CanPlay"] = QVariant(canplay);
				break;	
			case MBMP_MPRIS::CanPause:
				vmap["CanPause"] = QVariant(canpause);
				break;
			case MBMP_MPRIS::CanSeek:
				vmap["CanSeek"] = QVariant(canseek);
				break;
			default:
				break;
		}	// switch				
  }	// for
  changeditems.clear();
  
  // create the message.  We never remove a property so we don't need
  // to deal with that - send an empty qstringlist
  QList<QVariant> vlist;
  vlist << QVariant(IPC_INTERFACE_MEDIAPLAYER2PLAYER) << vmap << QStringList();
  QDBusMessage msg = QDBusMessage::createSignal(IPC_OBJECT, "org.freedesktop.Dbus.Properties", "PropertiesChanged");  
  msg.setArguments(vlist);																																														
	QDBusConnection::sessionBus().send(msg);
	
	return;
}    

