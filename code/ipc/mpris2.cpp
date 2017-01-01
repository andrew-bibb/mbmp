/************************** mpris2.cpp ***************************

Code for the MPRISv2.2 interface on DBus. Controls the interfaces registered
on /org/mpris/MediaPlayer2.  It is kind of a hack because QT does not
really support creating multiple interfaces on one object.  Some of the
broad outline for this layout came from the Amarok project, although I've
geatly modified the internal workings.
  

Copyright (C) 2013-2016
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

# include <unistd.h>

# include <QtCore/QDebug>
# include <QtDBus/QDBusConnection>

# include "./mpris2.h"
# include "./mediaplayer2.h"
# include "./mediaplayer2player.h"

# include <gst/gstelement.h>


//  constructor
Mpris2::Mpris2(QObject* parent) : QObject(parent)
{
	// try to register an object on the system bus.
	if (! QDBusConnection::sessionBus().registerService(IPC_SERVICE)) {
    if (! QDBusConnection::sessionBus().registerService(QString(IPC_SERVICE).append(".instance" + QString::number(getpid()))) ) {;
			QCoreApplication::instance()->exit(1);
		}
	}	// if registering service failed

  //  Create adaptors
  mediaplayer2  = new MediaPlayer2(this);
  mediaplayer2player = new MediaPlayer2Player(this);
	QDBusConnection::sessionBus().registerObject(IPC_OBJECT, this, QDBusConnection::ExportAdaptors );
	
  return;
}  

/////////////////////// Public Functions //////////////////////////////
//
// Function to send along a change of state.  Called from playerctl
// processGstifacdMessages when a change of state is received
void Mpris2::setState(const int& i_s)
{
	// translate gstreamer enums to mpris2 values
	if (i_s == GST_STATE_PLAYING) static_cast<MediaPlayer2Player*>(mediaplayer2player)->setPlaybackStatus("Playing");
		else if (i_s == GST_STATE_PAUSED) static_cast<MediaPlayer2Player*>(mediaplayer2player)->setPlaybackStatus("Paused");
			else static_cast<MediaPlayer2Player*>(mediaplayer2player)->setPlaybackStatus("Stopped");
			
	return;		
}

//
// Function to send along a change in wrap mode known as LoopStatus in mpris2
// Called from a playerctl signal which receives a signal from playlist
void Mpris2::setLoopStatus(const bool& b_ls)
{
	// MBMP does not support Track repeat, so only choices are Playlist or None
	static_cast<MediaPlayer2Player*>(mediaplayer2player)->setLoopStatus(b_ls ? "Playlist" : "None");
	
	return;
}

//
// Function to send along a change in random mode known as Shuffle in mpris2
// Called from a playerctl signal which receives a signal from playlist
void Mpris2::setShuffle(const bool& b_s)
{
	static_cast<MediaPlayer2Player*>(mediaplayer2player)->setShuffle(b_s);
	
	return;
}	
	
//
// Function to send along a change of volume.  Called from playerctl
// when the UI volume control is changed
void Mpris2::setVolume(const double& d_v)
{
	// Gstreamer volume ranges are doubles in the range of 0.0 to 10.0
	// 0.0 = mute, 1.0 = 100%, so 10.0 must be really loud.  The volume
	// scale is linear, and the default is 1.0  Our dial uses integers
	// and is set up on a range of 0-30, with 10 being 100%. Mpris2 is 
	// based on doubles range from 0.0 to 1.0,  The conversions:
	//	Vol%		GStreamer MBMP	mpris2
	//	  0%		0.0				0			0.00
	//	100%		1.0				10		0.33
	//	300%		3.0				30  	1.00 	This is the maximum we want to go
	
	static_cast<MediaPlayer2Player*>(mediaplayer2player)->setVolume(d_v);
	
	return;
}

    
