/**************************** resource.h *******************************

Header file that contains program #defines and information used across
the program.

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

#ifndef RESOURCE_H
#define RESOURCE_H

///////////////////////////////// Program Values ///////////////////////
//
// Program Info (may be visible, but don't mark for tranalation) 
#define VERSION "2016.11.27-1"
#define RELEASE_DATE "2016.11.27"
#define COPYRIGHT_DATE "2013-2016"

// Program Values:
//	QApplication (not user visible)
//  QSettings (visible in filesystem only)
//	System Logging (visible in system logs only)
#define LONG_NAME "MBMP Player"
#define ORG "mbmp"
#define APP "mbmp"
#define LOG_NAME "MBMP"
#define PLAYER_NAME "mbmp_player"

// Program Values - Misc. (not user visible)
#define INTERNAL_THEME "MBMP_Icon_Theme"

namespace MBMP_MPRIS 
{
  enum {
    CanQuit       = 0x01,  
    Fullscreen		= 0x02,
    CanSetFull		= 0x03,
    CanRaise			= 0x04,
    HasTrackList	= 0x05,
    Identity			= 0x06,
    DesktopEntry	= 0x07,
    UriSchemes		= 0x08,
    MimeTypes			= 0x09,
    
		PlaybackStatus= 0x21,
		LoopStatus		= 0x22,
		PlaybackRate	= 0x23,
		Shuffle				= 0x24,
		Metadata			= 0x25,
		Volume				= 0x26,
		Position			= 0x27,
		MinRate				= 0x28,
		MaxRate				= 0x29,
		CanGoNext			= 0x2a,		
		CanGoPrevious	= 0x2b,	
		CanPlay				= 0x2c,	
		CanPause			= 0x2d,	
		CanSeek				= 0x2e,	
		CanControl		= 0x2f,	    
	};
};

#endif

