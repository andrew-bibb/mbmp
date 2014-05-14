/**************************** gstiface.cpp *****************************

Code to interface from our QT widgets, mainly PlayerCtl and Gstreamer

Copyright (C) 2014-2014
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

# include "./code/gstiface/gstiface.h"
# include "./code/playerctl/playerctl.h"  

# include <gst/video/videooverlay.h>
# include <gst/tag/tag.h>

# include "./code/resource.h"

# include <QDebug>
# include <QWidget>
# include <QTime>
# include <QMessageBox>

//  Callback Function: Return TRUE if this is a Visualization element 
static gboolean filter_vis_features (GstPluginFeature *feature, gpointer data)
{ 
  (void) data;
  GstElementFactory* factory;
   
  if (!GST_IS_ELEMENT_FACTORY (feature))
    return FALSE;
  factory = GST_ELEMENT_FACTORY (feature);
  if (!g_strrstr (gst_element_factory_get_klass (factory), "Visualization"))
    return FALSE;
   
  return TRUE;
}

// Callback Function: Set the opticaldrive if necessary
static void sourceSetup(void* bin, GstElement* src, QString* opticaldrive)
{ 
  (void) bin;
  gchar* device = NULL;
  
  // return now if opticaldrive is empty (means we are not playing a CD or DVD)
  if (opticaldrive->isEmpty() ) return;

  g_object_get(G_OBJECT (src), "device", &device, NULL);
  // if no device (probably a file or url) return. 
  if (device == NULL)
    return;
  // otherwise set the device on the source element
  else 
    g_object_set(G_OBJECT (src), "device", opticaldrive->toLocal8Bit().data(), NULL);   
}


// Constructor
GST_Interface::GST_Interface(QObject* parent) : QObject(parent)
{
  // members
  mediatype = MBMP_GI::NotPlaying;  // the type of media playing
  is_live = false;            // true if we are playing a live stream
  is_buffering = false;       // true if we are currently buffering
  vismap.clear();
  streammap.clear();
  mainwidget = qobject_cast<QWidget*>(parent);
  opticaldrive.clear();
  tracklist.clear();
  map_md_cd.clear();        // map containing CD metadata
  map_md_dvd.clear();       // map containing DVD metadata
  b_positionenabled = true; // this will be set to false if we fail getting the stream position.
                            // Checked at the top of queryStreamPosition so we don't flood the logs
                            // or the display with failed position messages                 
  
  // setup the dialog to display stream info
  streaminfo = new StreamInfo(this, mainwidget);
  streaminfo->enableAll(false);
    
  // initialize gstreamer
  gst_init(NULL, NULL);
  
  // Create the playbin pipeline, call it PLAYER_NAME defined in resource.h
  pipeline_playbin = gst_element_factory_make("playbin", PLAYER_NAME);
  
  // Create the playbin bus
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline_playbin));
  
  // Monitor the playbin source-setup signal
  g_signal_connect (GST_ELEMENT(pipeline_playbin), "source-setup", G_CALLBACK (&sourceSetup), &opticaldrive);
  
  // Start a timer to monitor the bus
  bus_timer = new QTimer(this);
  connect(bus_timer, SIGNAL(timeout()), this, SLOT(pollGstBus()));
  
  dl_timer = new QTimer(this);
  connect(dl_timer, SIGNAL(timeout()), this, SLOT(downloadBuffer()));
    
  // Create a QMap of the available audio visualizers.  Map format is
  // QString key
  // GstElementFactory* value
  GList* list;
  GList* walk;
 
  // Get a list of all visualization plugins.  Use the helper function
  // filter_vis_features() at the top of this file.
  list = gst_registry_feature_filter (gst_registry_get(), filter_vis_features, FALSE, NULL);
  
  // Walk through each visualizer plugin looking for visualizers
  for (walk = list; walk != NULL; walk = g_list_next (walk)) {
    const gchar* name;
    GstElementFactory* factory;
     
    factory = GST_ELEMENT_FACTORY (walk->data);
    name = gst_element_factory_get_longname (factory);
    vismap[name] = factory;
  } // for
  
  // clean up
  g_list_free(list);
  g_list_free(walk);
  
}

// Destructor
GST_Interface::~GST_Interface()
{
  gst_element_set_state (pipeline_playbin, GST_STATE_NULL);
  gst_object_unref (GST_OBJECT (pipeline_playbin));
  
  gst_object_unref (bus);
}

///////////////////////////// Public Functions /////////////////////////
//
// Slot to query an audio CD for the number of audio tracks on it.
// If the query succeeds assume we can play the CD.  Also used to set
// or clear the opticaldrive data element
int GST_Interface::checkCD(QString dev)
{
  // set the optical device and clear CD tag map
  opticaldrive = dev;
  map_md_cd.clear();
  
  // Create an audiocd pipeline.  
  GstElement* source;
  GstElement* sink; 
  GstElement* pipeline_audiocd;
  source = gst_element_factory_make ("cdparanoiasrc", "cd_source");
  g_object_set (G_OBJECT (source), "device", qPrintable(opticaldrive), NULL);
  sink = gst_element_factory_make ("fakesink", "cd_sink");  
  pipeline_audiocd = gst_pipeline_new ("audiocd");
  
  // check that all elements were created properly    
  if (!pipeline_audiocd || !source || !sink) {
    gst_element_post_message (pipeline_playbin,
      gst_message_new_application (GST_OBJECT (pipeline_playbin),
        gst_structure_new ("Application", "MBMP_GI", G_TYPE_STRING, "Error: Failed to create audiocd pipeline - Not all elements could be created.", NULL)));  
    opticaldrive.clear();    
    return MBMP_GI::NoCDPipe;
  }
  // create the pipeline and check to see if we were successful.
  else {  
    gst_bin_add_many (GST_BIN (pipeline_audiocd), source, sink, NULL);
    if (gst_element_link (source, sink) != TRUE) {
      gst_element_post_message (pipeline_playbin,
        gst_message_new_application (GST_OBJECT (pipeline_playbin),
          gst_structure_new ("Application", "MBMP_GI", G_TYPE_STRING, "Error: Failed to create audiocd pipeline - Elements could not be linked.", NULL)));  
      opticaldrive.clear();
      return MBMP_GI::NoCDPipe;
    } // if
  } // else   
     
  // Set the audiocd pipeline to PAUSED so we can get a track count
  gst_element_set_state(pipeline_audiocd, GST_STATE_PAUSED);

  // Get the track count, if we can't it means we can't read the CD
  // so return with an error
  gint64 duration = 0;
  GstFormat fmt = gst_format_get_by_nick("track");
  if (! gst_element_query_duration(pipeline_audiocd, fmt, &duration) ) {
    gst_element_set_state(pipeline_audiocd, GST_STATE_NULL);
    opticaldrive.clear();
    return MBMP_GI::BadCDRead;
  }
  
  // Clean up
  gst_element_set_state (pipeline_audiocd, GST_STATE_NULL);
  gst_object_unref (GST_OBJECT (pipeline_audiocd));  
    
  // So far so go, return no error
  return 0;
}

//
// Slot to check the DVD by doing a query of the number of chapters that
// are on it. If the query succeeds assume we can play the DVD.  Also
// used to set or clear the opticaldrive data element (used in the 
// gstreamer callback to set a device source)
int GST_Interface::checkDVD(QString dev)
{
  // set the optical device and clear the dvd tag map
  opticaldrive = dev;
  map_md_dvd.clear();

  // Create a dvd pipeline.  
  GstElement* source;
  GstElement* sink; 
  GstElement* pipeline_dvd;
  source = gst_element_factory_make ("dvdreadsrc", "dvd_source");
  g_object_set (G_OBJECT (source), "device", qPrintable(opticaldrive), NULL);
  sink = gst_element_factory_make ("fakesink", "dvd_sink");  
  pipeline_dvd = gst_pipeline_new ("dvd");
  
  // Check that all elements were created properly    
  if (!pipeline_dvd || !source || !sink) {
    gst_element_post_message (pipeline_playbin,
      gst_message_new_application (GST_OBJECT (pipeline_playbin),
        gst_structure_new ("Application", "MBMP_GI", G_TYPE_STRING, "Error: Failed to create DVD pipeline - Not all elements could be created.", NULL)));  
    opticaldrive.clear();    
    return MBMP_GI::NoDVDPipe;
  }
  // create the pipeline and check to see if we were successful.
  else {  
    gst_bin_add_many (GST_BIN (pipeline_dvd), source, sink, NULL);
    if (gst_element_link (source, sink) != TRUE) {
      gst_element_post_message (pipeline_playbin,
        gst_message_new_application (GST_OBJECT (pipeline_playbin),
          gst_structure_new ("Application", "MBMP_GI", G_TYPE_STRING, "Error: Failed to create DVD pipeline - Elements could not be linked.", NULL)));  
      opticaldrive.clear();
      return MBMP_GI::NoDVDPipe;
    } // if
  } // else 
  
  // Set the dvd pipeline to PAUSED 
  gst_element_set_state(pipeline_dvd, GST_STATE_PAUSED);
  
  gint64 duration = 0;
  GstFormat fmt = gst_format_get_by_nick("chapter");
  if (! gst_element_query_duration(pipeline_dvd, fmt, &duration) ) {
    gst_element_set_state(pipeline_dvd, GST_STATE_NULL);
    opticaldrive.clear();
    return MBMP_GI::BadDVDRead;
  } 
  
  // Clean up
  gst_element_set_state (pipeline_dvd, GST_STATE_NULL);
  gst_object_unref (GST_OBJECT (pipeline_dvd)); 
  
  // So far so go, return no error
  return 0;
}

// Play the media.  For local files and URL's only need the WinId and uri
// Pipeline_playbin is reset for each new file.  For CD do the same initially,
// but then when we want a new track just do a track seek on the running
// stream.
void GST_Interface::playMedia(WId winId, QString uri, int track)
{
  // constants
  const int bus_timeout = 500;  // timeout value in miliseconds
  
  // if we need to seek in a currently playing disk (CD or DVD)
  if (track > 0 ) {
    if (uri.contains("cdda", Qt::CaseInsensitive)) {
      gst_element_seek_simple(pipeline_playbin, gst_format_get_by_nick("track"), (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SKIP) , track - 1); 
    }
    else if (uri.contains("dvd", Qt::CaseInsensitive)) {
      gst_element_seek_simple(pipeline_playbin, gst_format_get_by_nick("chapter"), (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SKIP) , track);
    }
  }
  
  // else need to set a new media source
  else {  
    // variables
    GstStateChangeReturn ret;
    
    // start with the pipeline_playbin set to NULL, is_live to false
    gst_element_set_state (pipeline_playbin, GST_STATE_NULL);
    is_live = false;
    
    // Set the media source. The device is set in checkCD
    g_object_set(G_OBJECT(pipeline_playbin), "uri", qPrintable(uri), NULL);
    
    // Set our media type variable
    if (uri.startsWith("cdda://", Qt::CaseInsensitive)) mediatype = MBMP_GI::CD;
    else if (uri.startsWith("dvd://", Qt::CaseInsensitive)) mediatype = MBMP_GI::DVD;
      else if (uri.startsWith("http://", Qt::CaseInsensitive) || uri.startsWith("ftp://", Qt::CaseInsensitive)) mediatype = MBMP_GI::URL;
        else  mediatype = MBMP_GI::File;

    // Set the video overlay and allow it to handle navigation events
    gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(pipeline_playbin), winId);
    gst_video_overlay_handle_events(GST_VIDEO_OVERLAY(pipeline_playbin), TRUE);                
    
    // Bring the pipeline to paused and see if we have a live stream (for buffering)
    ret = gst_element_set_state(pipeline_playbin, GST_STATE_PAUSED);
    bus_timer->start(bus_timeout);
    
    switch (ret) {
      case GST_STATE_CHANGE_SUCCESS:
        is_live = false;
        // playback will start from inside the ASYNC_DONE case in pollGstBus
        break;
  
      case GST_STATE_CHANGE_FAILURE:
        gst_element_post_message (pipeline_playbin,
          gst_message_new_application (GST_OBJECT (pipeline_playbin),
            gst_structure_new ("Application", "MBMP_GI", G_TYPE_STRING, "Error: Failed to raise pipeline state from NULL to PAUSED - will not be able to play the media.", NULL))); 
        this->playerStop();
        break;
  
      case GST_STATE_CHANGE_NO_PREROLL:
        is_live = true;
        gst_element_set_state(pipeline_playbin, GST_STATE_PLAYING);
        break;
  
      default:
        break;
    } // switch
  } // else need new media source
  
  return;
}

//
// Toggle between pause and play.  The actual slot that receives playPause
// signals is in playerctl.  That slot calls this as a plain function.
void GST_Interface::playPause()
{
  GstState state = getState();
  
  if (state == GST_STATE_PLAYING) 
    gst_element_set_state(pipeline_playbin, GST_STATE_PAUSED);
  
  if (state == GST_STATE_PAUSED)
    gst_element_set_state(pipeline_playbin, GST_STATE_PLAYING);
  
  return;
}

//
// Return the pipeline_playbin state (null, ready, paused, playing)
GstState GST_Interface::getState()
{
  // constants
  const guint timeout = 500;    // timeout in miliseconds
  
  // variables
  GstState state;
  
  // get the state and return it
  gst_element_get_state(pipeline_playbin, &state, NULL, timeout);
  return state;
}

//
// Function to return the software volume.  Allowed values are 0-10
// 0.0 = mute, 1.0 = 100%, so 10 must be really loud.  The volume
// scale is linear
double GST_Interface::getVolume()
{
  //variables
  gdouble vol = 0.0;
  
  // retrieve the volume and return it
  g_object_get (G_OBJECT (pipeline_playbin), "volume", &vol, NULL);
  return vol;
}   


//
// Function to change the visualizer.  Called as a function from a
// slot in the playerctl class
void GST_Interface::changeVisualizer(const QString& vis)
{
  GstElement* vis_plugin = NULL;
  GstElementFactory* selected_factory = NULL;
  
  // this should not fail, vis and the selected_factory should both
  // exist in the vismap, and the function that sends us here gets 
  // the vis string from the map, but just in case
  selected_factory = vismap.value(vis);
  if (!selected_factory) {
    gst_element_post_message (pipeline_playbin,
      gst_message_new_application (GST_OBJECT (pipeline_playbin),
        gst_structure_new ("Application", "MBMP_GI", G_TYPE_STRING, "Error: Visualizer factory not found", NULL)));  
    return ;
  } // if
  
  // We have now selected a factory for the visualization element 
  vis_plugin = gst_element_factory_create (selected_factory, NULL);
  if (!vis_plugin) vis_plugin = NULL; // if null use the default
  
  //set the vis plugin for our pipeline_playbin
  g_object_set (pipeline_playbin, "vis-plugin", vis_plugin, NULL); 
  
}

//
//  Function to check the setting of a GstPlayFlag. Send the flag
// to be checked, return true if set and false if unset
bool GST_Interface::checkPlayFlag(const guint& checkflag)
{
  // variables
  guint flags = 0;
  g_object_get (pipeline_playbin, "flags", &flags, NULL);
  
  // check if set
  return (flags & checkflag);
}

//
//  Function to set or unset a GstPlayFlag.  Send the flag
//  to operate on and a bool setting, true to set, false to unset
void GST_Interface::setPlayFlag (const guint& targetflag, const bool& b)
{
  // variables
  guint flags = 0;
  g_object_get (pipeline_playbin, "flags", &flags, NULL);
  
  // now do the setting or unsetting
  b ? flags |= targetflag : flags &= ~targetflag;
  g_object_set (pipeline_playbin, "flags", flags, NULL);  
}

//
//  Function to create and return a QString about the audio stream
// properties
QString GST_Interface::getAudioStreamInfo()
{
  QString s = QString();
  GstTagList* tags = 0;
  gchar* str = 0;
  guint rate = 0;
  
  // if no audio streams were found
  if (streammap["n-audio"] < 1)
    s.append(tr("No Audio Streams Found"));
    
  // else for each audio stream found
  else {  
    for (int i = 0; i < streammap["n-audio"]; i++) {
      tags = NULL;
  
      // Emit the get-audio-tags signal, store and process the stream's audio tags 
      g_signal_emit_by_name (pipeline_playbin, "get-audio-tags", i, &tags);
      if (tags) {
        if (i == streammap["current-audio"] ) s.append("<b>");
        s.append(tr("Audio Stream: %1<br>").arg(i));
        if (gst_tag_list_get_string (tags, GST_TAG_AUDIO_CODEC, &str)) {
          s.append(tr("Codec: %1<br>").arg(str) );
          g_free (str);
        }
        
        if (gst_tag_list_get_string (tags, GST_TAG_LANGUAGE_CODE, &str)) {
          s.append(tr("Language: %1<br>").arg(str) );
          g_free (str);
        }
        
        if (gst_tag_list_get_uint (tags, GST_TAG_BITRATE, &rate)) {
          s.append(tr("Bitrate: %1<br>").arg(rate));
        }
  
        s.append("<br>");
        gst_tag_list_free (tags);
        if (i == streammap["current-audio"] ) s.append("</b>");
      } // if tags were found
    } // for loop
  } // else
  
  return s;
}

//
//  Function to create and return a QString about the video stream
// properties
QString GST_Interface::getVideoStreamInfo()
{
  QString s = QString();
  GstTagList* tags;
  gchar* str;
    
  // if no video streams were found
  if (streammap["n-video"] < 1)
    s.append(tr("No Video Streams Found"));
  
  // else for each video stream found
  else {  
    for (int i = 0; i < streammap["n-video"]; i++) {
      tags = NULL;
    
      // Emit the get-video-tags signal, store and process the stream's video tags 
      g_signal_emit_by_name (pipeline_playbin, "get-video-tags", i, &tags);
      if (tags) {
        if (i == streammap["current-video"] ) s.append("<b>");
        s.append(tr("Video Stream: %1<br>").arg(i));      
      
        gst_tag_list_get_string (tags, GST_TAG_VIDEO_CODEC, &str);
        s.append(tr("Codec: %1<br>").arg(str) );
      
        s.append("<br>");
        g_free (str);
        gst_tag_list_free (tags);
        if (i == streammap["current-video"] ) s.append("</b>");
        } // if tags
      } // for        
  } // else

  return s;
}

//
//  Function to create and return a QString about the text stream
// properties
QString GST_Interface::getTextStreamInfo()
{
  QString s = QString();
  GstTagList* tags;
  gchar* str;
    
  // if no subtitle streams were found
  if (streammap["n-text"] < 1)
    s.append(tr("No Subtitle Streams Found"));
  
  // else for each subtitle stream found
  else {  
    for (int i = 0; i < streammap["n-text"]; i++) {
      tags = NULL;
    
      // Emit the get-text-tags signal, store and process the stream's text tags 
      g_signal_emit_by_name (pipeline_playbin, "get-text-tags", i, &tags);
      if (tags) {
        if (i == streammap["current-text"] ) s.append("<b>");
        s.append(tr("Subtitle Stream: %1<br>").arg(i));     
      
        if (gst_tag_list_get_string (tags, GST_TAG_LANGUAGE_CODE, &str)) {
          s.append(tr("Language: %1<br>").arg(str ? str : tr("unknown")));
          g_free (str);
        }
        s.append("<br>");
        gst_tag_list_free (tags);
        if (i == streammap["current-text"] ) s.append("</b>");
      } // if tags
      else
        s.append(tr("No subtitle tags found"));
    } // for        
  } // else

  return s; 
}

//////////////////////////// Public Slots ////////////////////////////
//
// Slot to process a mouse navigation event.  Out VideoWidget emits a signal
// (via PlayerCtl) containing the mouse event data. The signal is connected
// to this function which then injects the mouse event into the stream
void GST_Interface::mouseNavEvent(QString event, int button, int x, int y)
{
  // Do nothing if we are not playing 
  if (getState() != GST_STATE_PLAYING)  return;
    
  // Find the gstNavigation element (part of xvimagesink)
  GstNavigation* nav = 0;           
  nav = GST_NAVIGATION (pipeline_playbin);
  
  // Return if we could not find a GstNavigation element in the pipeline
  if (! nav) return;
  
  // Inject the mouse event data into the stream
  gst_navigation_send_mouse_event(nav, qPrintable(event), button, static_cast<double>(x), static_cast<double>(y));
  
  return;
}

//
//  Slot to process a key navigation event
void GST_Interface::keyNavEvent(GstNavigationCommand cmd)
{
  // Return if we're not playing a dvd
  if (mediatype != MBMP_GI::DVD) return;  
  
  // Find the gstNavigation element (part of xvimagesink)
  GstNavigation* nav = 0;           
  nav = GST_NAVIGATION (pipeline_playbin);  
  
  // Inject the command into the stream
  gst_navigation_send_command(nav, cmd);
  
  return;
}

//
// Slot to seek to a specific position in the stream. Seek position
// sent is in seconds so need to convert it to nanoseconds for gstreamer
// Called when a QAction is triggered
void GST_Interface::seekToPosition(int position)
{
  // return if seeking is not enabled
  if (! this->queryStreamSeek() ) return;
  
  // seek flags
  const int seekflags =  GST_SEEK_FLAG_FLUSH    | // flush the pipeline_playbin
                          GST_SEEK_FLAG_SKIP      | // allow skipping frames
                          GST_SEEK_FLAG_KEY_UNIT  ; // seek to the nearest keyframe, faster but maybe not as accurate
  
  // now do the seek
  gst_element_seek_simple(pipeline_playbin, GST_FORMAT_TIME, (GstSeekFlags)(seekflags) , position * GST_SECOND);
  return;
} 


//
// Slot to change the audio stream to the stream number sent
void GST_Interface::setAudioStream(const int& stream)
{
  // make sure the stream exists
  if (stream < 0 || stream >= (streammap["n-audio"]) ) return;
      
  // change the stream to the int sent to the function
  g_object_set (G_OBJECT (pipeline_playbin), "current-audio", stream, NULL);
    
  // update the streammap and then the text display boxes
  streammap["current-audio"] = stream;
  streaminfo->updateAudioBox(getAudioStreamInfo());
    
  return;
} 

//
// Slot to change the video stream to the stream number sent
void GST_Interface::setVideoStream(const int& stream)
{
  // make sure the stream exists
  if (stream < 0 || stream >= (streammap["n-video"]) ) return;
      
  // change the stream to the int sent to the function
  g_object_set (G_OBJECT (pipeline_playbin), "current-video", stream, NULL);
    
  // update the streammap and text display boxes
  streammap["current-video"] = stream;
  streaminfo->updateVideoBox(getVideoStreamInfo());
  
  return;
} 

//
// Slot to change the subtitle stream to the stream number sent
void GST_Interface::setTextStream(const int& stream)
{
  // make sure the stream exists
  if (stream < 0 || stream >= (streammap["n-text"]) ) return;
  
  // change the stream to the int sent to the function
  g_object_set (G_OBJECT (pipeline_playbin), "current-text", stream, NULL);
  
  // update the streammap and text display boxes
  streammap["current-text"] = stream;
  streaminfo->updateSubtitleBox(getTextStreamInfo());
  
  return;
} 

  
// Poll the bus and emit signals for messages we choose to deal with.
// Called by a QTimer in the constructor.  Do minimal processing here
// and emit the busMessage signal for PlayerControl::processBusMessage
// to pickup and complete the processing.  Basically anything that needs
// to operate immediately on the stream do here, anything that the user
// needs to know about do in PlayerControl.
void GST_Interface::pollGstBus()
{
  // query the stream position if we are currently playing. This will
  // set the PlayerControl position widgets
  if (getState() == GST_STATE_PLAYING) queryStreamPosition();
  
  // return if there are no new messages
  if (! gst_bus_have_pending(bus)) return;
  
  
  // variables and constants
  GstMessage* msg = 0;  
  const int msgtypes = GST_MESSAGE_EOS              |
                        GST_MESSAGE_ERROR             | 
                        GST_MESSAGE_WARNING           |
                        GST_MESSAGE_INFO              |
                        GST_MESSAGE_STATE_CHANGED     |
                        GST_MESSAGE_STREAM_START      |
                        GST_MESSAGE_APPLICATION       |
                        GST_MESSAGE_BUFFERING         | 
                        GST_MESSAGE_DURATION_CHANGED  |
                        GST_MESSAGE_TOC               |
                        GST_MESSAGE_TAG               |
                        GST_MESSAGE_ASYNC_DONE        |
                        GST_MESSAGE_CLOCK_LOST;
  
  msg = (GstMessage*)(gst_bus_pop_filtered(bus, (GstMessageType)(msgtypes)) );
  while (msg != NULL) {   
    switch (GST_MESSAGE_TYPE (msg)) {
      
      // An ERROR message generated somewhere in the pipeline_playbin.  Gstreamer docs say the pipeline_playbin should be taken out down if an ERROR
      // message is sent, however I've found this does not seem to be the behavior when using the commandline gst-launch-1.0 utility.
      // I've remove the command to take down the pipeline_playbin from here to match that behavior. 
      case GST_MESSAGE_ERROR: {
        GError* err = NULL;
        gchar* dbg_info = NULL;
        
        gst_message_parse_error (msg, &err, &dbg_info);
        emit busMessage(MBMP_GI::Error, QString(tr("ERROR from element %1: %2\n  Debugging information: %3\n  The pipeline_playbin has been shut down"))
            .arg(GST_OBJECT_NAME (msg->src))
            .arg(err->message)
            .arg( (dbg_info) ? dbg_info : "none") );
        
        g_error_free (err);
        g_free (dbg_info);    
        break; }
    
    // A WARNING message generated somewhere in the pipeline_playbin
      case GST_MESSAGE_WARNING: {
        GError* err = NULL;
        gchar* dbg_info = NULL;
        
        gst_message_parse_warning (msg, &err, &dbg_info);
        emit busMessage(MBMP_GI::Warning, QString(tr("WARNING MESSAGE from element %1: %2\n  Debugging information: %3"))
            .arg(GST_OBJECT_NAME (msg->src))
            .arg(err->message)
            .arg( (dbg_info) ? dbg_info : "none") );
        
        g_error_free (err);
        g_free (dbg_info);    
        break; }
      
      // An INFO message generated somewhere in the pipeline_playbin
      case GST_MESSAGE_INFO: {
        GError* err = NULL;
        gchar* dbg_info = NULL;
        
        gst_message_parse_info (msg, &err, &dbg_info);
        emit busMessage(MBMP_GI::Info, QString(tr("INFOMATION MESSAGE from element %1: %2\n  Debugging information: %3"))
            .arg(GST_OBJECT_NAME (msg->src))
            .arg(err->message)
            .arg( (dbg_info) ? dbg_info : "none") );
        
        g_error_free (err);
        g_free (dbg_info);    
        break; }
      
      // A clock_lost message, try to reset the clock by pausing then restarting the player
      case GST_MESSAGE_CLOCK_LOST : {
        emit busMessage(MBMP_GI::ClockLost, QString(tr("Pipeline clock has become unusable, trying to reset...")) );
        gst_element_set_state (pipeline_playbin, GST_STATE_PAUSED);
        gst_element_set_state (pipeline_playbin, GST_STATE_PLAYING);
        break;  }
  
      // The end of stream message. Put the player into the NULL state.
      case GST_MESSAGE_EOS: {
        emit busMessage(MBMP_GI::EOS, QString(tr("End of stream has been reached.")) );
        break; }
      
      // The start of stream message
      case GST_MESSAGE_STREAM_START: {
        emit busMessage(MBMP_GI::SOS, QString(tr("Start of a stream has been detected.")) );
        break; }
      
      // Player state changed.  Do a bunch of processing here to analyze the stream and
      // set the streaminfo dialog accordingly.
      case GST_MESSAGE_STATE_CHANGED: {
        GstState old_state;
        GstState new_state;
        
        gst_message_parse_state_changed (msg, &old_state, &new_state, NULL);
        emit busMessage(MBMP_GI::State, QString(tr("%1 has changed state from %2 to %3."))
                                            .arg(GST_OBJECT_NAME (msg->src))
                                            .arg(gst_element_state_get_name (old_state))
                                            .arg(gst_element_state_get_name (new_state)) ); 
        // set the streammap based on what state changed                                                                              
        if (QString(GST_OBJECT_NAME (msg->src)).contains(PLAYER_NAME, Qt::CaseSensitive)) {                                   
          switch (new_state) {
            case GST_STATE_PLAYING:
              analyzeStream();
              streaminfo->updateAudioBox(getAudioStreamInfo());
              streaminfo->updateVideoBox(getVideoStreamInfo());
              streaminfo->updateSubtitleBox(getTextStreamInfo());
              streaminfo->setComboBoxes(streammap); 
              streaminfo->setSubtitleBoxEnabled(checkPlayFlag(GST_PLAY_FLAG_TEXT));
              streaminfo->enableAll(true);
              qobject_cast<PlayerControl*>(mainwidget)->setDurationWidgets(queryDuration() / (1000 * 1000 * 1000), queryStreamSeek() );
              break;
            case GST_STATE_PAUSED:
              streaminfo->enableAll(false);
              break;
            default:
              streaminfo->updateAudioBox(tr("Audio Information"));
              streaminfo->updateVideoBox(tr("Video Information"));
              streaminfo->updateSubtitleBox(tr("Subtitle Information"));
              streammap.clear();
              streaminfo->setComboBoxes(streammap); 
              streaminfo->enableAll(false);
              qobject_cast<PlayerControl*>(mainwidget)->setDurationWidgets(-1);
              b_positionenabled = true;
          } // state switch
        } // if           
        break; }    
      
      // A message we generate
      case GST_MESSAGE_APPLICATION: {
        gchar* payload = NULL;
        gst_structure_get(gst_message_get_structure(msg), "MBMP_GI", G_TYPE_STRING, &payload, NULL); 
        emit busMessage(MBMP_GI::Application, QString(payload));
        g_free(payload);
        break; }
      
      // Buffering messages, pause the playback while buffering, restart when finished
      case GST_MESSAGE_BUFFERING: {
        // if a live stream don't buffer
        if (is_live) break;           
                
        // if download flag is set report buffering and let ASYNC_DONE deal with buffering
        guint flags = 0;
        g_object_get (pipeline_playbin, "flags", &flags, NULL);
        if (  (flags & GST_PLAY_FLAG_DOWNLOAD) ) {
          is_buffering = true;
          break;
        } 
        
        // non-download buffering
        gint percent = 0;
        gst_message_parse_buffering(msg, &percent);
        
        if (percent < 100) {
          if (! is_buffering) {
            gst_element_set_state (pipeline_playbin, GST_STATE_PAUSED);
            is_buffering = true;
          }
        }
        else {
          gst_element_set_state (pipeline_playbin, GST_STATE_PLAYING);
          is_buffering = false;
        }
        
        emit busMessage(MBMP_GI::Buffering, QString::number(percent));
        break; }
          
      // Duration changed message.  These are typically only created for streams that have a variable bit rate
      // where the pipeline_playbin calculates a duration based on some average bitrate.  Only report the duration changed
      // using the emit, we activate or disactivate the position widgets from the STATE_CHANGED case above.     
      case GST_MESSAGE_DURATION_CHANGED: {
        QTime t(0,0,0);
        t = t.addSecs(queryDuration() / (1000 * 1000 * 1000));
        emit busMessage(MBMP_GI::Duration, QString(tr("New stream duration: %1")).arg(t.toString("HH:mm:ss")) );
        break; }
        
      // TOC message, for instance from an audio CD or DVD      
      case GST_MESSAGE_TOC: {
        GstToc* toc;
        gboolean updated = false;
        
        // parse the TOC
        gst_message_parse_toc (msg, &toc, &updated);
          
        // if updated just send on the message, don't do any processing here
        if (updated) {
          emit busMessage(MBMP_GI::TOC, QString(tr("Received an updated table of contents for the media.")) );
         }
         
        // TOC is new, process as appropriate 
        else {
          // create a new track list if the TOC contains tracklists
          GList* entry = gst_toc_get_entries(toc);
          if (gst_toc_entry_get_entry_type((GstTocEntry*) g_list_nth_data(entry, 0)) == GST_TOC_ENTRY_TYPE_TRACK ) {
            tracklist.clear();  
            for (uint i = 0; i < g_list_length(entry); ++i) {
              this->extractTocTrack((GstTocEntry*) g_list_nth_data(entry, i));
            } // for
            emit busMessage(MBMP_GI::TOCTL, QString(tr("Received a new table of contents and tracklist.")) ); 
          } // if
          else {
            emit busMessage(MBMP_GI::TOC, QString(tr("Received a new table of contents for the media.")) );
          } // else
        } // else
              
        gst_toc_unref(toc); 
        break; }
      
      // TAG message.  Can be used to signal we need to query CDDB or MUSICBRAINZ
      // we have not implemented these as of yet, so for now just send a notification
      // to PlayerCtl that we got a tag
      case GST_MESSAGE_TAG: {
        gchar* str = NULL;
        guint num = 0;
        GstTagList* tags = NULL;
        gst_message_parse_tag (msg, &tags);
        
        // Get tags and emit a message listing the tags we've got with their values
        str = gst_tag_list_to_string(tags);
        emit busMessage(MBMP_GI::Tag, QString(tr("Stream contains this taglist: %1")).arg(QString(str)) );
        g_free(str);
        
        // Process tags appropriate to each media type
        switch (mediatype) {
          case MBMP_GI::CD: {
            // Get Audio CD tags. map_md_cd has already been cleared in function check_CD 
            // May need a new emit when we actually want to use some of this data, which right now we don't.
            if (!map_md_cd.contains(GST_TAG_CDDA_CDDB_DISCID) && gst_tag_list_get_string (tags, GST_TAG_CDDA_CDDB_DISCID, &str)) {
              map_md_cd[GST_TAG_CDDA_CDDB_DISCID] = QString(str);
              g_free (str);
            }
            if (!map_md_cd.contains(GST_TAG_CDDA_CDDB_DISCID_FULL) && gst_tag_list_get_string (tags, GST_TAG_CDDA_CDDB_DISCID_FULL, &str)) {
              map_md_cd[GST_TAG_CDDA_CDDB_DISCID_FULL] = QString(str);
              g_free (str);
            }
            if (!map_md_cd.contains(GST_TAG_CDDA_MUSICBRAINZ_DISCID) && gst_tag_list_get_string (tags, GST_TAG_CDDA_MUSICBRAINZ_DISCID, &str)) {
              map_md_cd[GST_TAG_CDDA_MUSICBRAINZ_DISCID] = QString(str);
              g_free (str);
            }
            if (!map_md_cd.contains(GST_TAG_CDDA_MUSICBRAINZ_DISCID_FULL) && gst_tag_list_get_string (tags, GST_TAG_CDDA_MUSICBRAINZ_DISCID_FULL, &str)) {
              map_md_cd[GST_TAG_CDDA_MUSICBRAINZ_DISCID_FULL] = QString(str);
              g_free (str);
            }
            if (!map_md_cd.contains(GST_TAG_TRACK_COUNT) && gst_tag_list_get_uint (tags, GST_TAG_TRACK_COUNT, &num)) {
              map_md_cd[GST_TAG_TRACK_COUNT] = num;
              num = 0;
            }
            if (gst_tag_list_get_uint (tags, GST_TAG_TRACK_NUMBER, &num)) {
              if (num != map_md_cd.value(GST_TAG_TRACK_NUMBER)) {
                map_md_cd[GST_TAG_TRACK_NUMBER] = num;
                mainwidget->setWindowTitle(tr("Audio CD - Track %1").arg(map_md_cd.value(GST_TAG_TRACK_NUMBER).toString()) );         
              } // if we have a new track number
              num = 0;
            }
            break; }  // cd case
        
          case MBMP_GI::DVD: {
            // Get DVD tags. map_md_dvd has already been cleared in function check_DVD. 
            // As with Audio CD we don't really do much with any of this (yet)
              if (!map_md_dvd.contains(GST_TAG_VIDEO_CODEC) && gst_tag_list_get_string (tags, GST_TAG_VIDEO_CODEC, &str)) {
                map_md_dvd[GST_TAG_VIDEO_CODEC] = QString(str);
                g_free (str);
              }
              if (!map_md_dvd.contains(GST_TAG_MINIMUM_BITRATE) && gst_tag_list_get_uint (tags, GST_TAG_MINIMUM_BITRATE, &num)) {
                map_md_dvd[GST_TAG_MINIMUM_BITRATE] = num;
                num = 0;
              }
              if (!map_md_dvd.contains(GST_TAG_BITRATE) && gst_tag_list_get_uint (tags, GST_TAG_BITRATE, &num)) {
                map_md_dvd[GST_TAG_BITRATE] = num;
                num = 0;
              }
              if (!map_md_dvd.contains(GST_TAG_MAXIMUM_BITRATE) && gst_tag_list_get_uint (tags, GST_TAG_MAXIMUM_BITRATE, &num)) {
                map_md_dvd[GST_TAG_MAXIMUM_BITRATE] = num;
                num = 0;
              }
              if (gst_tag_list_get_string (tags, GST_TAG_TITLE, &str)) {
                if (map_md_dvd.value(GST_TAG_TITLE).toString() != QString(str)) {
                  map_md_dvd[GST_TAG_TITLE] = QString(str);
                  mainwidget->setWindowTitle(map_md_dvd.value(GST_TAG_TITLE).toString());
                  g_free (str);
                } // if we have a new title
              } // if we have a new DVD title
              
              // not actually tag information, but see if we can extract some metadata from the dvd stream
              gint64 chaptercount = 0;
              gint64 currentchapter = 0;
              GstFormat fmt = gst_format_get_by_nick("chapter");
              if (gst_element_query_duration(pipeline_playbin, fmt, &chaptercount) ) {
                if (map_md_dvd.value("chaptercount") != static_cast<int>(chaptercount))  {
                  map_md_dvd["chaptercount"] = static_cast<int>(chaptercount);
                  emit busMessage(MBMP_GI::TagCL, QString(tr("DVD chapter count changed to %1")).arg(map_md_dvd.value("chaptercount").toInt()) );
                  chaptercount = 0;
                } // if there is a new chaptercount
              } // if we could extract the chaptercount             
              if (gst_element_query_position(pipeline_playbin, fmt, &currentchapter) ) {
                if (map_md_dvd.value("currentchapter") != static_cast<int>(currentchapter))  {
                  map_md_dvd["currentchapter"] = static_cast<int>(currentchapter);
                  emit busMessage(MBMP_GI::TagCC, QString(tr("DVD current chapter changed to %1")).arg(map_md_dvd.value("currentchapter").toInt()) );                 
                  currentchapter = 0;
                } // if there is a new chapter
              } // if we could extract the chapter                
                      
              gint64 titlecount = 0;                      
              gint64 currenttitle = 0;
              fmt = gst_format_get_by_nick("title");
              if (gst_element_query_duration(pipeline_playbin, fmt, &titlecount) ) {
                if (map_md_dvd.value("titlecount") != static_cast<int>(titlecount))  {
                  map_md_dvd["titlecount"] = static_cast<int>(titlecount);
                  titlecount = 0;
                } // if there is a new titlecount
              } // if we could extract the titlecount               
              if (gst_element_query_position(pipeline_playbin, fmt, &currenttitle) ) {
                if (map_md_dvd.value("currenttitle") != static_cast<int>(currenttitle))  {
                  map_md_dvd["currenttitle"] = static_cast<int>(currenttitle);
                  currenttitle = 0;
                } // if there is a new currenttitle
              } // if we could extract the currenttitle                         
            break; }  // dvd case
            
          default: {  
            // if we are not playing a CD or DVD try to get title and artist to set the player
            // window title
            if (map_md_cd.isEmpty() && map_md_dvd.isEmpty() ) {
              QString qs_title = QString();
              QString qs_artist = QString();
              if (gst_tag_list_get_string (tags, GST_TAG_TITLE, &str)) {
                if (str) qs_title = QString(str);
                else qs_title.clear();
                g_free (str);
              }     
              if (gst_tag_list_get_string (tags, GST_TAG_ARTIST, &str)) {
                if (str) qs_artist = QString(str);
                else qs_artist.clear();
                g_free (str);
              }
              if (qs_title.isEmpty() ) mainwidget->setWindowTitle(WINDOW_TITLE);
              else {
                if (qs_artist.isEmpty() )
                  mainwidget->setWindowTitle(QString("%1").arg(qs_title) );
                else
                  mainwidget->setWindowTitle(QString("%1 - %2").arg(qs_title).arg(qs_artist) );
              } // else
            } // if both maps are empty (were're playing something other than a CD or DVD)
            break; }  // default media type case
          } // mediatype switch
          
        gst_tag_list_free (tags); 
        break; }  // GST_TAG case
      
      // Posted when elements complete an async state change.  Use to avoid rebuffering
      // if the download flag is set.   
      case GST_MESSAGE_ASYNC_DONE: {
        // only buffer if the download playflag is set 
        guint flags = 0;
        g_object_get (pipeline_playbin, "flags", &flags, NULL);
        // if DOWNLOAD flag is set and we are currently buffering start the
        // download.  dl_timer is connected to downloadBuffer() which will
        // start the playback at the appropriate time.>> 
        if ( (flags & GST_PLAY_FLAG_DOWNLOAD) && is_buffering )  {
          dl_timer->start(500);
        } // if download flag is set
        
        // otherwise set the pipeline to playing  
        else {
          gst_element_set_state (pipeline_playbin, GST_STATE_PLAYING);        
        }

        break; }  // ASYNC_START case   
      
    default:
      emit busMessage(MBMP_GI::Unhandled, QString(tr("Unhandled GSTBUS message")) );
    } // switch
    
  gst_message_unref(msg);   
  msg = (GstMessage*)(gst_bus_pop_filtered(bus, (GstMessageType)(msgtypes)) );    
  } // while loop 
    
  return;
}

// Slot to toggle mute
void GST_Interface::toggleMute()
{
  // variables
  gboolean b_mute = false;
  
  // toggle the mute setting
  g_object_get (G_OBJECT (pipeline_playbin), "mute", &b_mute, NULL);     
  b_mute ? g_object_set (G_OBJECT (pipeline_playbin), "mute", false, NULL) : g_object_set (G_OBJECT (pipeline_playbin), "mute", true, NULL);
    
  return;
}

// Slot to change the volume.  Volume needs to be in the 0.0 to 10.0
// with a default of 1.0. This is checked in the calling function
void GST_Interface::changeVolume(const double& d_vol)
{
  // change the volume to the double we sent to the function
  g_object_set (G_OBJECT (pipeline_playbin), "volume", d_vol, NULL);
  
  return;
}

// Slot to change the connection speed.  Speed is measured in kbps
// and needs to be <= 18446744073709551.  Default is 0
void GST_Interface::changeConnectionSpeed(const guint64& ui64_speed)
{ 
  // change the connection soeed to the ui64 sent to the function
  g_object_set (G_OBJECT (pipeline_playbin), "connection-speed", ui64_speed, NULL);
    
  return;
}

// Slot to stop the player
void GST_Interface::playerStop()
{
  gst_element_set_state (pipeline_playbin, GST_STATE_NULL);
  
  // reset data elements
  opticaldrive.clear();
  map_md_cd.clear();
  map_md_dvd.clear();
  mediatype = MBMP_GI::NotPlaying;
  bus_timer->stop();  
  is_live = false;
  is_buffering = false;
  dl_timer->stop();
  
  // reset the window title
  mainwidget->setWindowTitle(WINDOW_TITLE);
  
  return;
}

//
// Slot to toggle the streaminfo dialog up and down.  Called from
// a QAction in various functions
void GST_Interface::toggleStreamInfo()
{
  streaminfo->isVisible() ? streaminfo->hide() : streaminfo->show();
  
  return;
}

//////////////////////////// Private Functions//////////////////////////
//
// Function to extract the information contained in a GstTocEntry and 
// write it into our QList<TocEntry>
void GST_Interface::extractTocTrack(const GstTocEntry* e)
{ 
  // make sure we have a track entry
  if (gst_toc_entry_get_entry_type(e) != GST_TOC_ENTRY_TYPE_TRACK ) return;
  
  uint track = 0;
  gint64 start = 0;
  gint64 end = 0;
  GstTagList* tags = 0;
  TocEntry entry;
  
  tags = gst_toc_entry_get_tags(e);         
  if (gst_tag_list_get_uint (tags, GST_TAG_TRACK_NUMBER, &track)) {
    entry.track = track;
    if (gst_toc_entry_get_start_stop_times(e, &start, &end) ) {
      entry.start = start / (1000 * 1000 * 1000); // in seconds
      entry.end = end / (1000 * 1000 * 1000);     // in seconds
    } // if
  } // if
  tracklist.append(entry);
  
  //  This is confusing, if line below is uncommented this throws lots 
  // of assertion errors.  With comments no problems.  Should check for
  // memory leak with valgrind.
  //gst_tag_list_free (tags); 
  return;
}

//
// Function to query the media stream and fillout the streammap.  Called
// from pollGstBus whenever the state of the player changes to PLAYING.
// Ignore all the other pipeline_playbin subobjects going into the PLAYING state.
void GST_Interface::analyzeStream()
{
  int target = 0;
  streammap.clear();
  
  g_object_get (pipeline_playbin, "n-video", &target, NULL);
  streammap["n-video"] = target;
  g_object_get (pipeline_playbin, "n-audio", &target, NULL);
  streammap["n-audio"] = target;
  g_object_get (pipeline_playbin, "n-text", &target, NULL);
  streammap["n-text"] = target;
  g_object_get (pipeline_playbin, "current-video", &target, NULL);
  streammap["current-video"] = target;
  g_object_get (pipeline_playbin, "current-audio", &target, NULL);
  streammap["current-audio"] = target;
  g_object_get (pipeline_playbin, "current-text", &target, NULL);
  streammap["current-text"] = target;

  return;
}

//
// Function to query the steam position.  Called from pollGstBus and only
// when it has been determined that stream is playing
void GST_Interface::queryStreamPosition()
{
  // return now if we ever failed getting a stream position
  if (! b_positionenabled) return;
  
  gint64 position = 0;  // the position in nanoseconds 
  
  if (!gst_element_query_position (pipeline_playbin, GST_FORMAT_TIME, &position)) {
    b_positionenabled = false;
    gst_element_post_message (pipeline_playbin,
      gst_message_new_application (GST_OBJECT (pipeline_playbin),
        gst_structure_new ("Application", "MBMP_GI", G_TYPE_STRING, "Error: Could not query the stream position", NULL))); 
       }  // if query failed
  else {
    qobject_cast<PlayerControl*>(mainwidget)->setPositionWidgets(position / (1000 * 1000 * 1000) );
  } // else query succeeded       
  
  return;
} 

//
// function to query the stream to see if we can seek in it.  Called in 
// pollGstBus when the player state changes into a PLAYING state. First
// check to make sure there is a stream playing.  This should actually
// be checked before the function is called, but just in case.
bool GST_Interface::queryStreamSeek()
{
  // Make sure we are playing
  if (getState() != GST_STATE_PLAYING ) return false;
  
  // Variables
  GstQuery* query = 0;
  gint64 start = 0; // for now we don't do anything with start and end
  gint64 end = 0;
  gboolean seek_enabled = false;
    
  // Query the stream see if we can seek in it
  query = gst_query_new_seeking (GST_FORMAT_TIME);
  if (gst_element_query (pipeline_playbin, query)) {
    gst_query_parse_seeking (query, NULL, &seek_enabled, &start, &end);
  }
  else {
    gst_element_post_message (pipeline_playbin,
      gst_message_new_application (GST_OBJECT (pipeline_playbin),
        gst_structure_new ("Application", "MBMP_GI", G_TYPE_STRING, "Error: Could not determine if seek is possible - disabling seeking in the stream", NULL)));
  }
  
  gst_query_unref (query);
  return static_cast<bool>(seek_enabled);
 }
  
 //
 // Function to query the stream duration.  Return the duration in 
 // gstreamer standard nanoseconds.  Called from two locations in pollGstBus
 // one is in the DURATION case which is mainly for VBR streams, and we
 // only treate consider it for informational purposes.  The second is when
 // the STATE changes to PLAYING.  This is used to set the duration widgets.
 gint64 GST_Interface::queryDuration()
 {
  gint64 duration = 0;  // the duration in nanoseconds (nanoseconds!! are you kidding me)
  
  if (!gst_element_query_duration (pipeline_playbin, GST_FORMAT_TIME, &duration)) {
    gst_element_post_message (pipeline_playbin,
      gst_message_new_application (GST_OBJECT (pipeline_playbin),
        gst_structure_new ("Application", "MBMP_GI", G_TYPE_STRING, "Error: Could not query the stream duration", NULL))); 
   }  // if query failed
   
   return duration;
 } 
 
 //////////////////////////// Private Slots //////////////////////////
//
// Slot to download a file, or most of a file to a buffer.  Called from 
// ASYNC_DONE case of pollGstBus. I cannot get the example in chapter
// 15 of the Gstreamer docs to work.  Anyway, download the entire
// file before we start playback
void GST_Interface::downloadBuffer()
{   
  // query the pipeline
  GstQuery* query;        
  gint64 estimated_left = 0;
  static gint64 maximum = -1;
  int percent = 0;  
  
  query = gst_query_new_buffering (GST_FORMAT_BYTES);
  if (! gst_element_query (pipeline_playbin, query)) return;  
  
  gst_query_parse_buffering_range (query, NULL, NULL, NULL, &estimated_left);
  
  if (estimated_left == -1) return;;
  
  // calculate the percent downloaded
  if (estimated_left > maximum) maximum = estimated_left;
  if (maximum != 0 ) {
    percent = static_cast<int>(100 * (maximum - estimated_left) / maximum);
  }
  else
    percent = 0;
  
  if (estimated_left == 0) {
    gst_element_set_state(pipeline_playbin, GST_STATE_PLAYING);
    dl_timer->stop();
    is_buffering = false;
    maximum = -1;
    percent = 100;
  }

  emit busMessage(MBMP_GI::Buffering, QString::number(percent));
  
  return;
 }
