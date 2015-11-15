/**************************** resource.h *******************************

Header file that contains program #defines and information used across
the program.

Copyright (C) 2013-2014
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
#define VERSION "2015.11.15-1"
#define RELEASE_DATE "15 September 2015"
#define COPYRIGHT_DATE "2013-2015"

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

#endif

