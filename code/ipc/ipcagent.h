/**************************** ipcagent.h *****************************

Code for the MPRISv2.2 interface on DBus.  When registered MBMP
will communicate to other processes.  This program and registering on
dbus will be started from the playerctl constructor.

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


# ifndef IPC_AGENT
# define IPC_AGENT

# include <QObject>
# include <QtDBus/QDBusContext>
# include <QString>
# include <QStringList>
# include <QVector>

# include "./code/resource.h"
# include "./ipcplayer.h"

# define IPC_SERVICE "org.mpris.MediaPlayer2.mbmp"
# define IPC_OBJECT "/org/mpris/MediaPlayer2"
# define IPC_INTERFACE_AGENT "org.mpris.MediaPlayer2"

class IPC_Agent : public QObject, protected QDBusContext
{
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", IPC_INTERFACE_AGENT)
    
  public:
    IPC_Agent(QObject* parent = 0);
		
		// properties
		Q_PROPERTY (bool CanQuit READ getCanQuit);
		Q_PROPERTY (bool Fullscreen READ getFullscreen WRITE setFullscreen) ;
		Q_PROPERTY (bool CanSetFullscreen READ getCanSetFullscreen);
		Q_PROPERTY (bool CanRaise READ getCanRaise);
		Q_PROPERTY (bool HasTrackList READ getHasTrackList);
		Q_PROPERTY (QString Identity READ getIdentity);
		Q_PROPERTY (QString DesktopEntry READ getDesktopEntry);
		Q_PROPERTY (QStringList SupportedUriSchemes READ getSupportedUriSchemes);
		Q_PROPERTY (QStringList SupportedMimeTypes READ getSupportedMimeTypes);
		
		// functions
		void stopAgent();
		
		// get functions for memebers
	  inline bool getCanQuit() const {return canquit;}
	  inline bool getFullscreen() const {return fullscreen;}
	  inline bool getCanSetFullscreen() const {return cansetfullscreen;}
	  inline bool getCanRaise() const {return canraise;}
	  inline bool getHasTrackList() const {return hastracklist;}
	  inline QString getIdentity() const {return identity;}
	  inline QString getDesktopEntry() const  {return desktopentry;}
	  inline QStringList getSupportedUriSchemes() const {return supportedurischemes;}
	  inline QStringList getSupportedMimeTypes() const {return supportedmimetypes;}

		// set functions for members (most can or will never be used)
		inline void setCanQuit(bool b_cc) {canquit = b_cc; changeditems.append(MBMP_MPRIS::CanQuit); emit propertyChanged();}
	  inline void setFullscreen(bool b_fs) {fullscreen = b_fs; changeditems.append(MBMP_MPRIS::Fullscreen); emit propertyChanged();}
	  inline void setCanSetFullscreen(bool b_cfs) {cansetfullscreen = b_cfs; changeditems.append(MBMP_MPRIS::CanSetFull); emit propertyChanged();}
	  inline void setRaise(bool b_r) {canraise = b_r; changeditems.append(MBMP_MPRIS::CanRaise); emit propertyChanged();}	
	  inline void setHasTrackList(bool b_htl) {hastracklist = b_htl; changeditems.append(MBMP_MPRIS::HasTrackList); emit propertyChanged();}
	  inline void setIdentity(QString s_id) {identity = s_id; changeditems.append(MBMP_MPRIS::Identity); emit propertyChanged();}
	  inline void setDesktopEntry(QString s_de) {desktopentry = s_de; changeditems.append(MBMP_MPRIS::DesktopEntry); emit propertyChanged();}	 
		inline void setSupportedUriSchemes(QStringList sl_u) {supportedurischemes = sl_u; changeditems.append(MBMP_MPRIS::UriSchemes); emit propertyChanged();}	
		inline void setSupportedMimeSchemes(QStringList sl_m) {supportedmimetypes = sl_m; changeditems.append(MBMP_MPRIS::MimeTypes); emit propertyChanged();}			  		  
  
	public Q_SLOTS:
		Q_SCRIPTABLE inline void Quit() {emit controlStop();} 
		Q_SCRIPTABLE inline void Raise() {}	// does nothing
		
		void sendPropertyChanged();
	
	Q_SIGNALS:		
		void controlStop();
		void propertyChanged();
		
	private:
		// data members
		bool canquit;
		bool fullscreen;
		bool cansetfullscreen;
		bool canraise;
		bool hastracklist;
		QString identity;
		QString desktopentry;
		QStringList supportedurischemes;
		QStringList supportedmimetypes;
		
		QVector<int> changeditems;
		IPC_Player* ipcplayer; 
};  


#endif
