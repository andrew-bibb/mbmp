/**************************** scman.cpp ******************************

Class to manage shortcuts and allow the user to provide substitutions

Copyright (C) 2015
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

# include "./scman.h"

# include <QDir>
# include <QFile>
# include <QFileInfo>
# include <QTextStream>
# include <QDebug>
# include <QList>
# include <QStringList>

// Constructor
ShortCutManager::ShortCutManager(QObject* parent) : QObject(parent) 
{
	// Set the cfg member (path to ${home}/.config/mbmp
	// APP defined in resource.h
	cfg = QDir::homePath().append(QString("/.config/%1/%1.keys").arg(QString(APP).toLower()) );	
	
	// Set the qrc data member
	qrc = QString(":/text/text/sc_def.txt");
	
	//// Initialize key_map
	key_map.clear();
	
	//// Make the local conf file if necessary
	this->makeLocalFile();	
	
	// Create the key_map.   
	QFile f1(cfg);
	if (!f1.open(QIODevice::ReadOnly | QIODevice::Text)) {
		#if QT_VERSION >= 0x050400 
			qCritical("Error opening key map file: %s", qUtf8Printable(cfg) );
		# else	
			qCritical("Error opening key map file: %s", qPrintable(cfg) );
		# endif
		f1.setFileName(qrc);
		if (!f1.open(QIODevice::ReadOnly | QIODevice::Text)) {
			#if QT_VERSION >= 0x050400 
				qCritical("Error opening qresource: %s", qUtf8Printable(cfg) );
			# else	
				qCritical("Error opening qresource: %s", qPrintable(cfg) );
			# endif	
			return;
		}	// failed reading resource file
	}	// if failed reading local file
	
	QTextStream in(&f1);
	QString line;
	while (!in.atEnd()) {
		line = in.readLine();
		line = line.simplified();
		line = line.section("#", 0, 0);
		if (line.size() > 0 ) {	
			QString cmd = line.section(' ', 0, 0);
			QStringList sl_shortcuts = line.section(' ', 1, 1).split(',');	
			QList<QKeySequence> keyseq;
			keyseq.clear();
			for (int j = 0; j < sl_shortcuts.size(); ++j) {
				keyseq.append(QKeySequence::fromString(sl_shortcuts.at(j)) );		
			}	// j loop
			key_map[cmd] = keyseq;
		} // if size > 0		
	}	// while not atEnd()
	f1.close();	
		
	return;
}

//////////////////////////////// Public Functions ////////////////////////////
//
//	Function to return the QKeySequence associated with a command.  
//	The argument "cmd" is a text string starting with the: "cmd_" as
// 	defined in the configuration file.  
QList<QKeySequence> ShortCutManager::getKeySequence(const QString& cmd)
{	
	if (key_map.contains(cmd) ) return key_map.value(cmd);
	
	// cmd not found, return an empty QKeySequence list
	QList<QKeySequence> list;
	list.clear();	
	return list;
}

//
//	Slot to return a cheatsheet of key bindings
QString ShortCutManager::getCheatSheet()
{
	QString s = QString();
	s.append(QString("<table><tr><td width=110><b>%1</b></td><td><b>%2</b></td></tr>").arg(tr("KEY(S)")).arg(tr("COMMAND")) );
			
	QMap<QString, QList<QKeySequence> >::const_iterator itr = key_map.constBegin();
	while (itr != key_map.constEnd()) {
		QStringList sl;
		for (int i = 0; i < itr.value().size(); ++i) {
			sl.append(itr.value().at(i).toString().toHtmlEscaped() );
		}	// for
		if (! sl.at(0).isEmpty() ) {
			QString t = itr.key();
			s.append(QString("<tr><td>%1</td><td>%2</td></tr>").arg(sl.join(',')).arg(t.remove(0,4)) );
		}	// if
    ++itr;
	}
	s.append("</table>");
	return s;
}


//////////////////////////////// Private Functions ////////////////////////////
//
//
// Function to make a local version of the configuration fiqle
void ShortCutManager::makeLocalFile()
{
	// if the conf file exists return now
	if (QFileInfo::exists(cfg) )
		return;	
		
	// make the directory if it does not exist and copy the hardconded
	// conf file to the directory
	QDir d;
	if (d.mkpath(QFileInfo(cfg).path()) ) {
		QFile s(qrc);	
		if (s.copy(cfg) ) 
			QFile::setPermissions(cfg, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
		else	
		#if QT_VERSION >= 0x050400 
			qCritical("Failed copying the shortcut definition file from %s to %s", qUtf8Printable(qrc), qUtf8Printable(cfg) );
		#else	
			qCritical("Failed copying the shortcut definition file from %s to %s", qPrintable(qrc), qPrintable(cfg) );		
		#endif
	}	// if mkpath returned ture			
  
	return;
}
