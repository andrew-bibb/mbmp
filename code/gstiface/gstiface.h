# ifndef GST_INTERFACE
# define GST_INTERFACE

#include <gst/gst.h>

# include <QWidget>
# include <QString>
# include <QObject>
# include <QTimer>
# include <QMap>
# include <QList>

# include "./code/streaminfo/streaminfo.h"

//	Enum's local to this program
namespace MBMP 
{
  enum {
		State 			= 0x01,		// state changed
		EOS					= 0x02,		// end of stream detected
		SOS					= 0x03,		// start of stream detected
		Error				= 0x04,		// error message
		Warning 		= 0x05,		// warning message
		Info				= 0x06,		// information message
		ClockLost		= 0x07,		// lost the clock
		Buffering		= 0x08,		// stream buffering message
		Application	= 0x09,		// application messages we generate	
		Duration		= 0x0a,		// a stream duration message
		TOC					= 0x0b,		// a table of contents
		Unhandled		= 0xff,		// an unhandled message
  };
} // namespace MBMP

// Gstreamer playbin GstPlayFlags
typedef enum {
  GST_PLAY_FLAG_VIDEO         = (1 << 0),
  GST_PLAY_FLAG_AUDIO         = (1 << 1),
  GST_PLAY_FLAG_TEXT          = (1 << 2),
  GST_PLAY_FLAG_VIS           = (1 << 3),
  GST_PLAY_FLAG_SOFT_VOLUME   = (1 << 4),
  GST_PLAY_FLAG_NATIVE_AUDIO  = (1 << 5),
  GST_PLAY_FLAG_NATIVE_VIDEO  = (1 << 6),
  GST_PLAY_FLAG_DOWNLOAD      = (1 << 7),
  GST_PLAY_FLAG_BUFFERING     = (1 << 8),
  GST_PLAY_FLAG_DEINTERLACE   = (1 << 9),
  GST_PLAY_FLAG_SOFT_COLORBALANCE = (1 << 10)
} GstPlayFlags;

class GST_Interface : public QObject
{
	Q_OBJECT
	
	public:
		GST_Interface(QObject*);
		~GST_Interface();

		void playMedia(WId, const QString&);
		void playPause();
		GstState getState();
		double getVolume();
		QList<QString> getVisualizerList();
		void changeVisualizer(const QString&);
		bool checkPlayFlag(const guint&);
		void setPlayFlag(const guint&, const bool&);
		QString getAudioStreamInfo();
		QString getVideoStreamInfo();
		QString getTextStreamInfo();
		inline QMap<QString, int> getStreamMap() {return streammap;} 
				
		public slots:
		void playCD();
		void seekToPosition(int);
		void setAudioStream(const int&);
		void setVideoStream(const int&);
		void setTextStream(const int&);
		void pollGstBus();
		void toggleMute();
		void changeVolume(const double&);
		void changeConnectionSpeed(const guint64&);		
		void playerStop();
		void toggleStreamInfo();
		// passthrough slots
		inline void cycleAudioStream() {streaminfo->cycleAudioStream();}
		inline void cycleVideoStream() {streaminfo->cycleVideoStream();}
		inline void cycleTextStream()  {streaminfo->cycleTextStream();}
	
	signals:
		void busMessage(int, QString = QString());
		
	private:
		// members
		GstElement* pipeline;
		GstBus* bus;
		QTimer* timer;
		QMap<QString, GstElementFactory*> vismap; 
		QMap<QString, int> streammap;
		StreamInfo* streaminfo;		
		QWidget* mainwidget;
		bool b_positionenabled;
		
		// functions
		void analyzeStream();
		void queryStreamPosition();
		bool queryStreamSeek();
		gint64 queryDuration();
};
		
# endif		
