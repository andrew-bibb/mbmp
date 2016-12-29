/************************** mpris2.cpp ***************************

Code for the MPRISv2.2 interface on DBus.  When this object is registered 
MBMP will communicate to other processes.  


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

//  header files generated by qmake from the xml file created by qdbuscpp2xml
# include "./mediaplayer2_adaptor.h"
# include "./mediaplayer2player_adaptor.h"

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
  DBusAbstractAdaptor* adaptor = new MediaPlayer2(this);
  adaptor->setDBusPath( "/org/mpris/MediaPlayer2" );
  adaptor = new MediaPlayer2Player(this);
  adaptor->setDBusPath( "/org/mpris/MediaPlayer2" );
	QDBusConnection::sessionBus().registerObject(IPC_OBJECT, this, QDBusConnection::ExportAdaptors );
	
	
  return;
}  
    
