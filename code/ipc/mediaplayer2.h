/************************** mediaplayer2.h *****************************

Code for the MediaPlayer2 interface on DBus.  When registered MBMP
will communicate to other processes. 

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


# ifndef IPC_MEDIAPLAYER2
# define IPC_MEDIAPLAYER2

# include <QObject>
# include <QtDBus/QDBusContext>
# include <QtDBus/QDBusAbstractAdaptor>
# include <QString>
# include <QStringList>
# include <QVector>

# include "./code/resource.h"
# include "./mpris2.h"

class MediaPlayer2 : public QDBusAbstractAdaptor
{
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", IPC_INTERFACE_MEDIAPLAYER2)
    
  public:
    MediaPlayer2(Mpris2*);
		
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
	  
		// set functions for members.  most can or will never be used, they are here because we are supposed to emit a propertyChanged()
		// signal over dbus if one does change,   
		inline void setCanQuit(bool b_cc) {canquit = b_cc; changeditems.append(MBMP_MPRIS::CanQuit); sendPropertyChanged();}
	  inline void setFullscreen(bool b_fs) {fullscreen = b_fs; changeditems.append(MBMP_MPRIS::Fullscreen); sendPropertyChanged();}
	  inline void setCanSetFullscreen(bool b_cfs) {cansetfullscreen = b_cfs; changeditems.append(MBMP_MPRIS::CanSetFull); sendPropertyChanged();}
	  inline void setRaise(bool b_r) {canraise = b_r; changeditems.append(MBMP_MPRIS::CanRaise); sendPropertyChanged();}	
	  inline void setHasTrackList(bool b_htl) {hastracklist = b_htl; changeditems.append(MBMP_MPRIS::HasTrackList); sendPropertyChanged();}
	  inline void setIdentity(QString s_id) {identity = s_id; changeditems.append(MBMP_MPRIS::Identity); sendPropertyChanged();}
	  inline void setDesktopEntry(QString s_de) {desktopentry = s_de; changeditems.append(MBMP_MPRIS::DesktopEntry); sendPropertyChanged();}	 
		inline void setSupportedUriSchemes(QStringList sl_u) {supportedurischemes = sl_u; changeditems.append(MBMP_MPRIS::UriSchemes); sendPropertyChanged();}	
		inline void setSupportedMimeSchemes(QStringList sl_m) {supportedmimetypes = sl_m; changeditems.append(MBMP_MPRIS::MimeTypes); sendPropertyChanged();}			  		  
			  

	public Q_SLOTS:
		Q_SCRIPTABLE void Quit(); 
		Q_SCRIPTABLE inline void Raise() {}	// does nothing
		
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
		
		// functions
		void sendPropertyChanged();		
};  

#endif
