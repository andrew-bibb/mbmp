/**************************** scman.cpp ******************************

Class to manage shortcuts and allow the user to provide substitutions

Copyright (C) 2015-2020
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
# include <QCryptographicHash>
# include <QSettings>
# include <QMessageBox>
# include <QProcessEnvironment>


// Constructor
ShortCutManager::ShortCutManager(QObject* parent) : QObject(parent) 
{
  // Setup the config path and filename (where we store shortcuts)
  // APP defined in resource.h
  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  QString home = env.value("HOME");
  cfg = QString(env.value("XDG_CONFIG_HOME", QString(QDir::homePath()) + "/.config") + "/%1/%1.keys").arg(QString(APP).toLower() );
	
	// Set the qrc data member
	qrc = QString(":/text/text/sc_def.txt");
	
	// Initialize key_map and shifted key string
	key_map.clear();
	
	// Make the local conf file if necessary
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
		line = line.simplified();				// remove extra white space
		line = line.section("#", 0, 0);	// remove comments
		if (line.size() > 0 ) {	
			QString cmd = line.section('=', 0, 0).simplified();
			key_map[cmd] = line.section('=', 1, -1).simplified().split(' ');
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
	// initialize variables
	QList<QKeySequence> keyseq;
	keyseq.clear();
	
	if (key_map.contains(cmd) ) {
		QStringList sl = key_map.value(cmd);
		for (int i = 0; i < sl.count(); ++i) {			
			keyseq.append(QKeySequence::fromString(sl.at(i)) );		
		}	// for
	}	// if
		
	// return (empty list if cmd not in key_map)	
	return keyseq;
}

//
//	Slot to return am html formatted cheatsheet of key bindings
QString ShortCutManager::getCheatSheet()
{
	QString s = QString();
	s.append(QString("<table><tr><td width=110><b>%1</b></td><td><b>%2</b></td></tr>").arg(tr("KEY(S)")).arg(tr("COMMAND")) );
			
	QMap<QString, QStringList >::const_iterator itr = key_map.constBegin();
	while (itr != key_map.constEnd()) {
		QStringList sl = itr.value();
		
		if (! sl.at(0).isEmpty() ) {
			QString t = itr.key();
			s.append(QString("<tr><td>%1</td><td>%2</td></tr>").arg(sl.join(' ').toHtmlEscaped()).arg(t.remove(0,4)) );
		}	// if
		
    ++itr;
	}
	s.append("</table>");
	
	return s;
}


//////////////////////////////// Private Functions ////////////////////////////
//
// Function to make a local version of the configuration fiqle
void ShortCutManager::makeLocalFile()
{
	// constants
	const int maxloop = 50;
	
	// Get information about the last installed shortcut def file from the settings
	QSettings* settings = new QSettings(ORG, APP, this);
	settings->beginGroup("ShortcutManager");
  QString lastmd5 = settings->value("last_installed_sc_def_file").toString();
  settings->endGroup();
  
  // Get the MD5 sum of the current
  QFile src(qrc);	
  src.open(QIODevice::ReadOnly);
  QCryptographicHash hash(QCryptographicHash::Md5);
  hash.addData(&src);
  src.close();
  QString currentmd5 = QString::fromLatin1(hash.result().toHex() ); 
  
	// If the user's local conf file exists
	if (QFileInfo::exists(cfg) ) {
		if (lastmd5 == currentmd5) {	// this should be the typical case
			settings->deleteLater();
			return;		
		}
		
		// MD5 sums don't match so make a backup of the existing local file
		else {
			// Find a backup name we can use
			int ctr = 0;
			QString bak;
			do {
				bak = QString(cfg + ".%1").arg(++ctr, 2, 10, QChar('0'));
			} while (QFileInfo::exists(bak) && ctr <= maxloop);
			
			// Now make the backup
			QFile f_cfg(cfg);
			if (ctr <= maxloop && f_cfg.copy(bak) ) { 
				QMessageBox::StandardButton dia_rtn = QMessageBox::information(0, QString(APP),
					tr("A new shortcut definition file will be installed to <b>%1</b> and a backup of the old definition file has been created as <b>%2</b> \
						<p>If the original definition file was customized you wish to retain those changes you will need to manually merge them into the new file.	\
						<p>If the original was never customized or you just wish to delete the backup now you may select <i>Discard</i> to delete the backup or <i>Save</i> to retain it.").arg(cfg).arg(bak),
					QMessageBox::Save | QMessageBox::Discard,
					QMessageBox::Save);
				if (dia_rtn == QMessageBox::Discard)
					if (! QFile::remove(bak))
					#if QT_VERSION >= 0x050400 
						qCritical("Failed to remove the backup file: %s", qUtf8Printable(bak) );
					#else	
						qCritical("Failed to remove the backup file: %s", qPrintable(bak) );		
					#endif
			}	// if creating a backup copy worked		
			else {	
			#if QT_VERSION >= 0x050400 
				qCritical("Failed creating the shortcut definition backup file: %s", qUtf8Printable(bak) );
			#else	
				qCritical("Failed creating the shortcut definition backup file: %s", qPrintable(bak) );		
			#endif
				settings->deleteLater();
				return;
			}	// else creating a backup failed so return, don't continue
			
			// Have a backup, now create the new file
			QFile::remove(cfg);
			QFile s(qrc);
			if (s.copy(cfg) ) { 
				QFile::setPermissions(cfg, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
				settings->beginGroup("ShortcutManager");
				settings->setValue("last_installed_sc_def_file", currentmd5);
				settings->endGroup();
			}	// if creating new file worked
			else {	
			#if QT_VERSION >= 0x050400 
				qCritical("Failed creating a new shortcut definition file: %s", qUtf8Printable(qrc) );
			#else	
				qCritical("Failed creating a new shortcut definition file: %s", qPrintable(qrc) );		
			#endif
			}	// failed creating the new file (next step is return so no reason to call it here)
		}	// qrc is different than the last installed
	}	// if local sc_def exists
	
	// Local sc_def does not exist so create the directory (if need be) and copy the sc_def file
	else {
		QDir d;
		if (d.mkpath(QFileInfo(cfg).path()) ) {
			QFile s(qrc);
			if (s.copy(cfg) ) { 
				QFile::setPermissions(cfg, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
				settings->beginGroup("ShortcutManager");
				settings->setValue("last_installed_sc_def_file", currentmd5);
				settings->endGroup();
			}	// if creating new file worked
			else	
			#if QT_VERSION >= 0x050400 
				qCritical("Failed creating a new shortcut definition file: %s", qUtf8Printable(qrc) );
			#else	
				qCritical("Failed creating a new shortcut definition file: %s", qPrintable(qrc) );		
			#endif
		}	// if mkpath returned true
		else 
		#if QT_VERSION >= 0x050400 
			qCritical("Failed creating directory %s for the short definition file.", qUtf8Printable(QFileInfo(cfg).path()) );
		#else	
			qCritical("Failed creating directory %s for the short definition file.", qPrintable(QFileInfo(cfg).path()) );
		#endif
	}	// else local sc_def did not exist			
  
  settings->deleteLater();
	return;
}
