/**************************** gstiface.h ******************************

Code to interface from our QT widgets, mainly PlayerCtl and Gstreamer

Copyright (C) 2014-2015
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
# ifndef GST_INTERFACE
# define GST_INTERFACE

# include <gst/gst.h>
# include <gst/video/navigation.h>

# include <QWidget>
# include <QString>
# include <QObject>
# include <QTimer>
# include <QMap>
# include <QList>
# include <QVariant>

# include "./code/streaminfo/streaminfo.h"

//  Enum's local to this program
namespace MBMP_GI 
{
  enum {
    // for the bus
    State       = 0x01,   // state changed
    EOS         = 0x02,   // end of stream detected
    SOS         = 0x03,   // start of stream detected
    Error       = 0x04,   // error message
    Warning     = 0x05,   // warning message
    Info        = 0x06,   // information message
    ClockLost   = 0x07,   // lost the clock
    Buffering   = 0x08,   // stream buffering message
    Application = 0x09,   // application messages we generate 
    Duration    = 0x0a,   // a stream duration message
    TOC         = 0x0b,   // a table of contents
    TOCTL       = 0x0c,   // a table of contents with a new track list  
    Tag         = 0x0d,   // received a TAG message
    TagCL       = 0x0e,   // received a TAG and extracted a new chapter list
    TagCC       = 0x0f,   // received a TAG and extracted a new current chapter
    NewTrack		= 0x10,		// tags indicate a new track
    StreamStatus= 0x11,		// stream status message
    Unhandled   = 0x2f,   // an unhandled message
    // return codes
    NoCDPipe    = 0x31,   // not able to create an Audio CD pipe
    BadCDRead   = 0x32,   // can't read the CD
    NoDVDPipe   = 0x33,   // not able to create a DVD pipe
    BadDVDRead  = 0x34,   // can't read the DVD 
    // media types
    NotPlaying  = 0x00,   // not playing anything
    File        = 0x01,   // a local media file
    URL         = 0x02,   // a remote file
    CD          = 0x03,   // an audio cd
    DVD         = 0x04,   // a dvd
  };
} // namespace MBMP_GI

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


//  Table of contents structure
struct TocEntry
{
  uint track;     // track number
  int start;      // start time (seconds)
  int end;        // end time (seconds)
};

class GST_Interface : public QObject
{
  Q_OBJECT
  
  public:
    GST_Interface(QObject*);
    ~GST_Interface();
    
    void rankElement(const QString&, bool);  
    int checkCD(QString); 
    int checkDVD(QString);      
    void playMedia(WId, QString, int track = 0);
    void playPause();
    GstState getState();
    double getVolume();
    void changeVisualizer(const QString&);
    bool checkPlayFlag(const guint&);
    void setPlayFlag(const guint&, const bool&);
    QString getAudioStreamInfo();
    QString getVideoStreamInfo();
    QString getTextStreamInfo();
    void busHandler(GstMessage*);
    // inline function to get private data members
    inline QList<QString> getVisualizerList() {return vismap.keys();}
    inline QList<TocEntry> getTrackList() {return tracklist;}
    inline QMap<QString, int> getStreamMap() {return streammap;} 
    inline int getChapterCount() {return map_md_dvd.value("chaptercount").toInt();}
    inline int getCurrentChapter() {return map_md_dvd.value("currentchapter").toInt();}
    inline QString getDVDTitle() {return map_md_dvd.value(GST_TAG_TITLE).toString();}
    inline int getMediaType() {return mediatype;};
        
    public slots:
    void mouseNavEvent(QString, int, int, int);
    void keyNavEvent(GstNavigationCommand);
    void seekToPosition(int);
    void setAudioStream(const int&);
    void setVideoStream(const int&);
    void setTextStream(const int&);
    void toggleMute();
    void changeVolume(const double&);
    void changeConnectionSpeed(const guint64&);   
    inline void playerStop() {gst_element_set_state (pipeline_playbin, GST_STATE_NULL);}
    void toggleStreamInfo();
    // passthrough slots
    inline void cycleAudioStream() {streaminfo->cycleAudioStream();}
    inline void cycleVideoStream() {streaminfo->cycleVideoStream();}
    inline void cycleTextStream()  {streaminfo->cycleTextStream();}

  signals:
    void busMessage(int, QString = QString());
    
  private:
    // members
    GstElement* pipeline_playbin;
    GstBus* bus;
    QTimer* pos_timer;
    QTimer* dl_timer;
    QMap<QString, GstElementFactory*> vismap; 
    QMap<QString, int> streammap;
    StreamInfo* streaminfo;   
    QWidget* mainwidget;
    bool b_positionenabled;
    QList<TocEntry> tracklist;
    QMap<QString, QVariant> map_md_cd;
    QMap<QString, QVariant> map_md_dvd;
    QString opticaldrive;
    int mediatype;
    bool is_live;
    bool is_buffering;
    
    // functions
    void extractTocTrack(const GstTocEntry*);
    void analyzeStream();
    bool queryStreamSeek();
    gint64 queryDuration();
    
    private slots:
    void queryStreamPosition();    
    void downloadBuffer();
};
    
# endif   
