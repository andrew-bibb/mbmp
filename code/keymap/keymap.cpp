/**************************** keymap.cpp *****************************

Code to manage the mappings from key presses to commands.

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

# include "./code/keymap/keymap.h"	
# include "./code/resource.h"

# include <QtCore/QDebug>
# include <QTextStream>
# include <QRegExp>
# include <QDir>
# include <QFile>
# include <QFileInfo>


KeyMap::KeyMap(QObject* parent) : QObject(parent)
{
	// setup the user key map
	usermap.clear();
	
	// set the filepath data member
	// APP defined in resource.h
	filepath = QDir::homePath();
	filepath.append(QString("/.config/%1").arg(QString(APP).toLower()) );

	// make the local conf file if necessary
	this->makeLocalFile();
	
	// Read the local file, if the returned stringlist is empty, meaning
	// we could not read the file, fallback to the hardcoded conf file.
	QStringList sl_rawdata = readTextFile(qPrintable(QString(filepath + "/%1.keys").arg(QString(APP).toLower())) );
	if (sl_rawdata.isEmpty()) QStringList sl_rawdata = readTextFile(":/text/text/mbmp.keys");
	
	// set up the usermap
	for (int i =0; i < sl_rawdata.size(); ++i) {
		QString s = sl_rawdata.at(i);
		s = s.simplified();
		s = s.left(s.indexOf('#'));
		if (s.size() > 0 ) {	
			QString cmd = s.section(' ', 0, 0);
			QStringList sl_shortcuts = s.section(' ', 1, 1).split(',');	
			QList<QKeySequence> keyseq;
			keyseq.clear();
			for (int j = 0; j < sl_shortcuts.size(); ++j) {	
				keyseq.append(QKeySequence::fromString(sl_shortcuts.at(j)) );		
			}	// j loop
			usermap[cmd] = keyseq;
		} // if size > 0			
	}	// i loop
}

////////////////////////////// Public Slots ////////////////////////////
//
//	Slot to return the QKeySequence associated with a command.  
//	The argument "cmd" is a text string starting with the: "cmd_" as
// 	defined in the configuration file.  
QList<QKeySequence> KeyMap::getKeySequence(const QString& cmd)
{	
	if (usermap.contains(cmd) ) return usermap.value(cmd);
	
	QList<QKeySequence> list;
	list.clear();	
	return list;
}

//
//	Slot to return a cheatsheet of key bindings
QString KeyMap::getCheatSheet()
{
	QString s = QString();
	s.append(QString("<table><tr><td width=110><b>%1</b></td><td><b>%2</b></td></tr>").arg(tr("KEY(S)")).arg(tr("COMMAND")) );
			
	QMap<QString, QList<QKeySequence> >::const_iterator itr = usermap.constBegin();
	while (itr != usermap.constEnd()) {
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
  
////////////////////////////// Private Functions ////////////////////////////
//
// Function to make a local version of the configuration fiqle
void KeyMap::makeLocalFile()
{
	// if the conf file exists return now
	if (QFileInfo::exists(qPrintable(QString(filepath + "/%1.keys").arg(QString(APP).toLower()))) )
		return;	
	
	// make the directory if it does not exist and copy the hardconded
	// conf file to the directory
	QDir d;
	if (d.mkpath(filepath)) {
		QFile s(qPrintable(QString(":/text/text/mbmp.keys")) );	
		if (s.copy(qPrintable(QString(filepath + "/%1.keys").arg(QString(APP).toLower()))) )
			QFile::setPermissions(qPrintable(QString(filepath + "/%1.keys").arg(QString(APP).toLower())), QFileDevice::ReadOwner | QFileDevice::WriteOwner);
		else	
			qDebug("Failed copying the key definition file");
	}	// if mkpath returned ture
  
	return;	
}

//
// Function to read text contained in a text or resource file.  Input is a 
// const char* to the file. Return value is a QStringList where each entry 
// is a single line from the source file
QStringList KeyMap::readTextFile(const char* textfile)
{
	QStringList sl = QStringList();
		
	QFile file(textfile);
  if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QTextStream in(&file);
		while (!in.atEnd()) {
			QString s = in.readLine();
			sl << s.trimmed();
		}	// while
	}	// if
	
	return sl;
}   



	
