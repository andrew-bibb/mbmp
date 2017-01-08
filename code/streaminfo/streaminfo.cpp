/**************************** streaminfo.cpp *****************************

Code to manage the media playlist.

Copyright (C) 2014-2017
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

# include "./code/streaminfo/streaminfo.h"	
# include "./code/playerctl/playerctl.h"	
# include "./code/scman/scman.h"

# include <QtCore/QDebug>


StreamInfo::StreamInfo(QObject* parent) : QDialog()
//StreamInfo::StreamInfo() : QDialog()
{
  // setup the user interface
  ui.setupUi(this);
  
  // actions are defined in the ui, need to add them to this so
	// that shortcuts work from here
	this->addAction(ui.actionCycleAudio);
	this->addAction(ui.actionCycleVideo);
	this->addAction(ui.actionCycleSubtitle);
	
	// now assign the shortcuts to each action
	ShortCutManager scman(this);
	ui.actionCycleAudio->setShortcuts(scman.getKeySequence("cmd_cycleaudio") );
	ui.actionCycleVideo->setShortcuts(scman.getKeySequence("cmd_cyclevideo") );
	ui.actionCycleSubtitle->setShortcuts(scman.getKeySequence("cmd_cyclesubtitle") );
	
	// create the stream menu
	stream_menu = new QMenu(this);
	stream_menu->addAction(ui.actionCycleAudio);
	stream_menu->addAction(ui.actionCycleVideo);
	stream_menu->addAction(ui.actionCycleSubtitle);
	
	// signal and slots
	connect (ui.comboBox_audio, SIGNAL(currentIndexChanged(int)), parent, SLOT(setAudioStream(int)));
	connect (ui.comboBox_video, SIGNAL(currentIndexChanged(int)), parent, SLOT(setVideoStream(int)));
	connect (ui.comboBox_subtitle, SIGNAL(currentIndexChanged(int)), parent, SLOT(setTextStream(int)));
	connect (ui.actionCycleAudio, SIGNAL(triggered()), this, SLOT(cycleAudioStream()));
	connect (ui.actionCycleVideo, SIGNAL(triggered()), this, SLOT(cycleVideoStream()));	
	connect (ui.actionCycleSubtitle, SIGNAL(triggered()), this, SLOT(cycleTextStream()));			
}

////////////////////////////// Public Slots ////////////////////////////
//
//	Slot to cycle to the next available audio stream
//	Called from a QAction
void StreamInfo::cycleAudioStream()
{qDebug() << "CYCLE AUDIO";
	// can't cycle if there is only one or less items
	if (ui.comboBox_audio->count() <= 1 ) return;
	
	int idx = ui.comboBox_audio->currentIndex();
	++idx;
	
	// if we are past the end cycle back to the start
	if (idx == ui.comboBox_audio->count() ) idx = 0;	
	
	// set the next stream in the combobox
	ui.comboBox_audio->setCurrentIndex(idx);
	return;
}

//
//	Slot to cycle to the next available video stream
//	Called from a QAction
void StreamInfo::cycleVideoStream()
{
	// can't cycle if there is only one or less items
	if (ui.comboBox_video->count() <= 1 ) return;
	
	int idx = ui.comboBox_video->currentIndex();
	++idx;
	
	// if we are past the end cycle back to the start
	if (idx == ui.comboBox_video->count() ) idx = 0;
	
	// set the next stream in the combobox
	ui.comboBox_video->setCurrentIndex(idx);
	
	return;	
}

//
//	Slot to cycle to the next available subtitle stream
//	Called from a QAction
void StreamInfo::cycleTextStream()
{
	// can't cycle if there is only one or less items
	if (ui.comboBox_subtitle->count() <= 1 ) return;
	
	int idx = ui.comboBox_subtitle->currentIndex();
	++idx;
	
	// if we are past the end cycle back to the start
	if (idx == ui.comboBox_subtitle->count() ) idx = 0;
	
	// set the next stream in the combobox
	ui.comboBox_subtitle->setCurrentIndex(idx);	
	
	return;
}

//////////////////////////// Public Functions //////////////////////////
//
// Function to setup the comboboxes.  Called when GST_Interface
// enters a playing state.  Called from pollGstBus() after analyzeStream()
void StreamInfo::setComboBoxes(const QMap<QString,int>& map)
{
	// audio combo box
	ui.comboBox_audio->clear();
	if (map["n-audio"] < 1 )
		ui.comboBox_audio->setEnabled(false);
	else {
		ui.comboBox_audio->setEnabled(true);
		for (int i = 0; i < map["n-audio"]; ++i) {
			ui.comboBox_audio->addItem(tr("Stream: %1").arg(i) );
		}	// for
		ui.comboBox_audio->setCurrentIndex(map["current-audio"]);																				
	}	// else
	
		// video combo box
	ui.comboBox_video->clear();
	if (map["n-video"] < 1 )
		ui.comboBox_video->setEnabled(false);
	else {
		ui.comboBox_video->setEnabled(true);
		for (int i = 0; i < map["n-video"]; ++i) {
			ui.comboBox_video->addItem(tr("Stream: %1").arg(i) );
		}	// for
		ui.comboBox_video->setCurrentIndex(map["current-video"]);
	}	// else
	
	// subtitle combo box
	ui.comboBox_subtitle->clear();
	if (map["n-text"] < 1 )
		ui.comboBox_subtitle->setEnabled(false);
	else {
		ui.comboBox_subtitle->setEnabled(true);
		for (int i = 0; i < map["n-text"]; ++i) {
			ui.comboBox_subtitle->addItem(tr("Stream: %1").arg(i) );
		}	// for
		ui.comboBox_subtitle->setCurrentIndex(map["current-text"]);
	}	// else	
	
	return;
}

//
//	Function to enable or disable the groupboxes
void StreamInfo::enableAll(bool b)
{
	ui.groupBox_audio->setEnabled(b);
	ui.groupBox_video->setEnabled(b);
	ui.groupBox_subtitle->setEnabled(b);
	this->stream_menu->setEnabled(b);
	
	return;
}

//////////////////////////// Protected Functions ////////////////////////////
//
//
//	Create a context menu activate by right click of the mouse.
void StreamInfo::contextMenuEvent(QContextMenuEvent* e)
{
	stream_menu->popup(e->globalPos());
}

