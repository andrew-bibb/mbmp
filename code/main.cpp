/**************************** main.cpp *******************************

Main program to parse command line arguments and start the event loop

Copyright (C) 2013-2020
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
# include <QtCore/QDebug>
# include <QApplication>
# include <QCommandLineOption>
# include <QCommandLineParser>
# include <QStringList>
# include <QTime>
# include <QTranslator>
# include <QLibraryInfo>

# include <signal.h> 

# include "./code/playerctl/playerctl.h"
# include "./code/resource.h"	


// Create a signal handler to catch ^C from console
void signalhandler(int sig) {
  if(sig == SIGINT || sig == SIGTERM) {
    qApp->quit();
  }
  
  return;
}

int main(int argc, char *argv[])
{
  QApplication::setApplicationName(LONG_NAME);
  QApplication::setApplicationVersion(VERSION);
  QApplication::setOrganizationName(ORG); 
  QApplication::setDesktopSettingsAware(true);
  QApplication app(argc, argv);

	// Create seed for the QT random number generator
	QTime time = QTime::currentTime();
	qsrand((uint)time.msec());

	// setup the command line parser
	QCommandLineParser parser;
	parser.setApplicationDescription(QApplication::translate("main.cpp", "GStreamer based media player.") );
	
	QCommandLineOption streamBuffering(QStringList() << "b" << "stream-buffering", QCoreApplication::translate("main.cpp", "Enable buffering of the demuxed or parsed data in the stream (default is no stream buffering).") );
	parser.addOption(streamBuffering);	
	
  QCommandLineOption connectionSpeed(QStringList() << "c" << "connection-speed", QCoreApplication::translate("main.cpp", "Specify a network connection speed in kbps (default is 0 meaning unknown)."), QCoreApplication::translate("main.cpp", "connection-speed"), "0" );  
  parser.addOption(connectionSpeed);
  
	QCommandLineOption downloadBuffering(QStringList() << "d" << "download-buffering", QCoreApplication::translate("main.cpp", "Enable progressive download buffering of selected formats (default is no download buffering).") );
	parser.addOption(downloadBuffering);	  
	
	QCommandLineOption openFullScreen(QStringList() << "f" << "fullscreen", QCoreApplication::translate("main.cpp", "Start the player in full screen mode (default is start in window).") );
	parser.addOption(openFullScreen);
	
	QCommandLineOption openGUI(QStringList() << "g" << "gui", QCoreApplication::translate("main.cpp", "Open the player in GUI mode (default is no GUI).") );
	parser.addOption(openGUI);
	
  parser.addHelpOption();  
  
  QCommandLineOption useIconTheme(QStringList() << "i" << "icon-theme",
		QCoreApplication::translate("main.cpp", "Use an icon theme from your system."),
		QCoreApplication::translate("main.cpp", "Icon Theme Name"),
		QString() );
  parser.addOption(useIconTheme);   
	
	QCommandLineOption logLevel(QStringList() << "l" << "loglevel", QCoreApplication::translate("main.cpp", "Set the log level from 0 to 4 (default is 1)."), QCoreApplication::translate("main.cpp", "loglevel"), "1" );
	parser.addOption(logLevel);  

 	QCommandLineOption openShadeMode(QStringList() << "s" << "shademode", QCoreApplication::translate("main.cpp", "Start the player in shade mode (default is start in normal mode).") );
	parser.addOption(openShadeMode);
	
	QCommandLineOption noHardwareDecoding(QStringList() << "w" << "no-hardware-decoding", QCoreApplication::translate("main.cpp", "Disable hardware decoding, (default is enabled).") );
	parser.addOption(noHardwareDecoding);
 
  parser.addVersionOption();	  
    
  QCommandLineOption cdDevice(QStringList() << "C" << "CD", QCoreApplication::translate("main.cpp", "Specify the optical drive that will play the audio CD (default is /dev/sr0)."), QCoreApplication::translate("main.cpp", "CD"), "/dev/sr0");
  parser.addOption(cdDevice);
  
  QCommandLineOption dvdDevice(QStringList() << "D" << "DVD", QCoreApplication::translate("main.cpp", "Specify the optical drive that will play the DVD (default is /dev/sr0)."), QCoreApplication::translate("main.cpp", "DVD"), "/dev/sr0");
  parser.addOption(dvdDevice);
 
 QCommandLineOption enableSubtitles(QStringList() << "S" << "subtitles", QCoreApplication::translate("main.cpp", "Enable display of subtitles if a subtitle stream is found (default is no subtitles).") );
  parser.addOption(enableSubtitles);  
	 
  QCommandLineOption enableVisualizer(QStringList() << "V" << "visualizer", QCoreApplication::translate("main.cpp", "Enable a visualizer when playing audio tracks (default is no visualizer).") );
  parser.addOption(enableVisualizer);  

  QCommandLineOption promoteElement(QStringList() << "promote", QCoreApplication::translate("main.cpp", "List of GStreamer elements (comma separated) to be promoted."), QCoreApplication::translate("main.cpp", "element list"), "" );  
  parser.addOption(promoteElement);
  
  QCommandLineOption blacklistElement(QStringList() << "blacklist", QCoreApplication::translate("main.cpp", "List (comma separated) of GStreamer elements to be blacklisted."), QCoreApplication::translate("main.cpp", "element list"), "" );  
  parser.addOption(blacklistElement);
    
	parser.addPositionalArgument("filename", QCoreApplication::translate("main.cpp", "Media file to play."));
  
  // Setup translations   
  QTranslator qtTranslator;
  qtTranslator.load("qt_" + QLocale::system().name(),
  QLibraryInfo::location(QLibraryInfo::TranslationsPath));
  app.installTranslator(&qtTranslator);

  QTranslator mbmpTranslator;
  if (mbmpTranslator.load("mbmp_" + QLocale::system().name(), ":/translations/translations" ) ) {  
		app.installTranslator(&mbmpTranslator);	
	}
	// else use en_US as it contains Connman strings properized and some singular/plural strings
	else if (mbmpTranslator.load("mbmp_en_US", ":/translations/translations" ) ) {
		app.installTranslator(&mbmpTranslator);	
	}


  // Make sure all the command lines can be parsed 
  // using parse() instead of process() as process stops on an error if an option needs a value
  // and it is not specified, even if we provide a default.  We're supposed to catch errors if we
  // use parse(), but parse.errorText() returns an empty string on this.  Bag the error checking
  // for now.
  parser.parse(QCoreApplication::arguments() );   
  QStringList sl = parser.unknownOptionNames();
  if (sl.size() > 0 ) parser.showHelp(1);
  if (parser.isSet("help") ) parser.showHelp(1);	
  if (parser.isSet("version") ) parser.showVersion();
  
  // signal handler             
  signal(SIGINT, signalhandler); 
  
  PlayerControl pctl(parser);
  pctl.show();
   
  return app.exec();

}


