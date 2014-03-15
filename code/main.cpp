# include <QtCore/QDebug>
# include <QApplication>
# include <QLocale>
# include <QCommandLineOption>
# include <QCommandLineParser>
# include <QStringList>
# include <QTime>

# include "./code/playerctl/playerctl.h"
# include "./code/resource.h"	

// uncomment to install translation code
//#define USE_TRANSLATIONS

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);
  QApplication::setApplicationName(PROGRAM_NAME);
  QApplication::setApplicationVersion(VERSION);

	// Create seed for the QT random number generator
	QTime time = QTime::currentTime();
	qsrand((uint)time.msec());

	// setup the command line parser
	QCommandLineParser parser;
	parser.setApplicationDescription(QApplication::translate("main.cpp", "GStreamer based media player.") );
  parser.addHelpOption();
  parser.addVersionOption();
	
	parser.addPositionalArgument("filename", QCoreApplication::translate("main.cpp", "Media file to play."));
	
	QCommandLineOption openFullScreen(QStringList() << "f" << "fullscreen", QCoreApplication::translate("main.cpp", "Start the player in full screen mode (default is start in window.") );
	parser.addOption(openFullScreen);
	
	QCommandLineOption openGUI(QStringList() << "g" << "gui", QCoreApplication::translate("main.cpp", "Open the player in GUI mode (default is no GUI).") );
	parser.addOption(openGUI);
	
	QCommandLineOption logLevel(QStringList() << "l" << "loglevel", QCoreApplication::translate("main.cpp", "Set the log level from 0 to 2 (default is 1)."), QCoreApplication::translate("main.cpp", "loglevel"), "1" );
	parser.addOption(logLevel);  
  
  QCommandLineOption enableVisualizer(QStringList() << "V" << "visualizer", QCoreApplication::translate("main.cpp", "Enable a visualizer when playing audio tracks (default is no visualizer).") );
  parser.addOption(enableVisualizer);
  
  QCommandLineOption enableSubtitles(QStringList() << "s" << "subtitles", QCoreApplication::translate("main.cpp", "Enable display of subtitles if a subtitle stream is found (default is no subtitles).") );
  parser.addOption(enableSubtitles);  
  
  QCommandLineOption connectionSpeed(QStringList() << "c" << "connection-speed", QCoreApplication::translate("main.cpp", "Specify a network connection speed in kbps (default is 0)."), QCoreApplication::translate("main.cpp", "connection-speed"), "0" );  
  parser.addOption(connectionSpeed);
  
  // setup translations 
	#ifdef USE_TRANSLATIONS
		QTranslator qtTranslator;
		qtTranslator.load("qt_" + QLocale::system().name(),
		QLibraryInfo::location(QLibraryInfo::TranslationsPath));
		app.installTranslator(&qtTranslator);

		QTranslator connmanqtTranslator;
		connmanqtTranslator.load("connmanqt_" + QLocale::system().name());
		app.installTranslator(&connmanqtTranslator);
  #endif

	parser.process(app);   
	QStringList sl = parser.unknownOptionNames();
	if (sl.size() > 0 ) parser.showHelp(1);
  
  PlayerControl pctl(parser);
  pctl.show();
   
  return app.exec();

}


