/**************************** ipcagent.h *****************************

Code for the ipc agentr registered on DBus.  When registered MBMP
will communicate to other processes.  This program and registering on
dbus will be started in the constructor.

Copyright (C) 2013-2015
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


# ifndef IPC_AGENT
# define IPC_AGENT

# include <QObject>
# include <QtDBus/QDBusContext>
# include <QVariant>

class IPC_Agent : public QObject, protected QDBusContext
{
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.mbmp.ipcagent")

  public:
    IPC_Agent(QObject* parent = 0);
 
		// functions
		void stopAgent();
  
  public slots:
		void stopPlayer();   
	
	signals:
//i	void trackChanged(QVariantMap);
		
	private:
		// data members
		QVariantMap vmap;	
};  

#endif