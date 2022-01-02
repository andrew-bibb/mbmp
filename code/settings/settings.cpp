/**************************** settings.cpp ***************************

Dialog to select program settings

Copyright (C) 2015-2022
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
# include <QProcess>
# include <QDir>
# include <QInputDialog>
# include <QFile>
# include <QColorDialog>
# include <QProcessEnvironment>
# include <QDirIterator>

# include "./code/settings/settings.h"
# include "./code/resource.h"

// constructor
// Most of the interface is defined in the ui
//
// The UI dialog button box accept button is connected to the writeSettings
// so slot so the settings on disk are always current to what is selected
// and set using the UI
//
Settings::Settings(QWidget *parent)
    : QDialog(parent)
{
  // setup the user interface
  ui.setupUi(this);

  // data members
  editor_string = QString();

  // read the settings
  settings = new QSettings(ORG, APP, this);

  // preferences
	settings->beginGroup("Preferences");
	ui.checkBox_usestartoptions->setChecked(settings->value("use_startoptions").toBool() );
	ui.groupBox_startoptions->setEnabled(settings->value("use_startoptions").toBool() );
	ui.checkBox_retainstate->setChecked(settings->value("retain_state").toBool() );
	ui.checkBox_retainplaylist->setChecked(settings->value("retain_playlist").toBool() );
	ui.checkBox_disabletooltips->setChecked(settings->value("disable_tooltips").toBool() );
	ui.checkBox_disablexscreensaver->setChecked(settings->value("disable_xscreensaver").toBool() );
	ui.checkBox_disabledpms->setChecked(settings->value("disable_dpms").toBool() );
	ui.lineEdit_colorize->setText(settings->value("colorize_icons").toString() );
	ui.checkBox_disableinternet->setChecked(settings->value("disable_internet").toBool() );
	ui.checkBox_useyoutubedl->setChecked(settings->value("use_youtube-dl").toBool() );
	ui.spinBox_youtubedl_timeout->setValue(settings->value("youtube-dl_timeout", 9).toInt() );
  ui.checkBox_alsasink->setChecked(settings->value("use_alsa_sink").toBool() );
  ui.lineEdit_alsasink->setText(settings->value("alsa_device").toString() );

  QDir res(":/stylesheets/stylesheets/");
	QStringList styles = res.entryList(QDir::Files);
	styles << tr("None");
	QString str = settings->value("style").toString();
	styles.replaceInStrings(".qss", "");
	if (! str.isEmpty() && ! styles.contains(str)) styles << str;
	styles.sort(Qt::CaseSensitive);
	ui.comboBox_style->clear();
	ui.comboBox_style->addItems(styles);
	ui.comboBox_style->setCurrentText(str);
	settings->endGroup();

	// notification settings
	settings->beginGroup("Notifications");
	ui.checkBox_notifydaemon->setChecked(settings->value("use_notifications").toBool() );
	settings->endGroup();

	// start options
	settings->beginGroup("StartOptions");
	ui.checkBox_fullscreen->setChecked(settings->value("start_fullscreen").toBool() );
	ui.checkBox_shademode->setChecked(settings->value("start_shademode").toBool() );
	ui.checkBox_gui->setChecked(settings->value("start_gui").toBool() );
	ui.checkBox_icontheme->setChecked(settings->value("use_icon_theme").toBool() );
	if (ui.checkBox_icontheme->isChecked() ) ui.lineEdit_icontheme->setEnabled(true);
	ui.lineEdit_icontheme->setText(settings->value("icon_theme_name").toString() );
	ui.spinBox_loglevel->setValue(settings->value("log_level").toInt() );
	ui.checkBox_visualizer->setChecked(settings->value("start_visualizer").toBool() );
	ui.checkBox_nohardwaredecoding->setChecked(settings->value("no_hardware_decoding").toBool() );
	ui.checkBox_subtitles->setChecked(settings->value("start_subtitles").toBool() );
	ui.checkBox_streambuffering->setChecked(settings->value("use_stream_buffering").toBool() );
	ui.checkBox_downloadbuffering->setChecked(settings->value("use_download_buffering").toBool() );
	ui.lineEdit_audiocd->setText(settings->value("audio_cd_drive").toString() );
	ui.lineEdit_dvd->setText(settings->value("dvd_drive").toString() );
	ui.spinBox_connectionspeed->setValue(settings->value("connection_speed").toInt() );
	ui.lineEdit_promoted->setText(settings->value("promoted-elements").toString() );
	ui.lineEdit_blacklisted->setText(settings->value("blacklisted-elements").toString() );
	settings->endGroup();

	// external programs
	settings->beginGroup("ExternalPrograms");
	editor_string = settings->value("text_editor").toString();
	settings->endGroup();

	// Set up the buttonGroup for the editing buttons
	bg01 = new QButtonGroup(this);
	bg01->addButton(ui.pushButton_editiconfile);
	bg01->addButton(ui.pushButton_editkeyfile);

	// Connect signals and slots
	connect(bg01, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(openEditor(QAbstractButton*)));
	connect(ui.toolButton_colorize, SIGNAL(clicked()), this, SLOT(callColorDialog()));
	connect(ui.lineEdit_colorize, SIGNAL(textChanged(const QString&)), this, SLOT(iconColorChanged(const QString&)));
	connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(writeSettings()));

	// See if we can find XScreenSaver and enable or disable controls
	// based on what we can find.  If XScreenSaver is running it controls
	// blanking and powersaving.
	if (isProcessRunning("xscreensaver") ) {
		ui.checkBox_disablexscreensaver->setEnabled(true);
		ui.checkBox_disabledpms->setEnabled(false);
	}
	else {
		ui.checkBox_disablexscreensaver->setEnabled(false);
		ui.checkBox_disabledpms->setEnabled(true);
	}	// else

	// See if we can find youtube-dl and disable settings if we can't
	if (isProgramAvailable("youtube-dl") ) {
		ui.checkBox_useyoutubedl->setEnabled(true);
	}
	else {
		ui.checkBox_useyoutubedl->setEnabled(false);
	}

	// We may have changed settings finding XScreensaver and youtube-dl so
	// make sure everything is in sync.
	this->writeSettings();

	return;
}

// Function to save GUI start options and preferences
void Settings::writeSettings()
{
  settings->beginGroup("Preferences");
  settings->setValue("use_startoptions", ui.checkBox_usestartoptions->isChecked() );
  settings->setValue("retain_state", ui.checkBox_retainstate->isChecked() );
  settings->setValue("retain_playlist", ui.checkBox_retainplaylist->isChecked() );
  settings->setValue("disable_tooltips", ui.checkBox_disabletooltips->isChecked() );
  settings->setValue("disable_xscreensaver", ui.checkBox_disablexscreensaver->isChecked() );
  settings->setValue("disable_dpms", ui.checkBox_disabledpms->isChecked() );
  settings->setValue("colorize_icons", ui.lineEdit_colorize->text() );
  settings->setValue("style", ui.comboBox_style->currentText() );
  settings->setValue("disable_internet", ui.checkBox_disableinternet->isChecked() );
  settings->setValue("use_youtube-dl", ui.checkBox_useyoutubedl->isChecked() );
  settings->setValue("youtube-dl_timeout", ui.spinBox_youtubedl_timeout->value() );
  settings->setValue("use_alsa_sink", ui.checkBox_alsasink->isChecked() );
  settings->setValue("alsa_device", ui.lineEdit_alsasink->text() );
  settings->endGroup();

  settings->beginGroup("Notifications");
  settings->setValue("use_notifications", ui.checkBox_notifydaemon->isChecked() );
  settings->endGroup();

  settings->beginGroup("StartOptions");
  settings->setValue("start_fullscreen", ui.checkBox_fullscreen->isChecked() );
  settings->setValue("start_shademode", ui.checkBox_shademode->isChecked() );
	settings->setValue("start_gui", ui.checkBox_gui->isChecked() );
  settings->setValue("use_icon_theme", ui.checkBox_icontheme->isChecked() );
  settings->setValue("icon_theme_name", ui.lineEdit_icontheme->text()  );
  settings->setValue("log_level", ui.spinBox_loglevel->value() );
  settings->setValue("start_visualizer", ui.checkBox_visualizer->isChecked() );
  settings->setValue("no_hardware_decoding", ui.checkBox_nohardwaredecoding->isChecked() );
  settings->setValue("start_subtitles", ui.checkBox_subtitles->isChecked() );
  settings->setValue("use_stream_buffering", ui.checkBox_streambuffering->isChecked() );
  settings->setValue("use_download_buffering", ui.checkBox_downloadbuffering->isChecked() );
  settings->setValue("audio_cd_drive", ui.lineEdit_audiocd->text() );
  settings->setValue("dvd_drive", ui.lineEdit_dvd->text() );
  settings->setValue("connection_speed", ui.spinBox_connectionspeed->value() );
  settings->setValue("promoted-elements", ui.lineEdit_promoted->text() );
  settings->setValue("blacklisted-elements", ui.lineEdit_blacklisted->text() );
  settings->endGroup();

 	settings->beginGroup("ExternalPrograms");
	settings->setValue("text_editor", editor_string);
	settings->endGroup();

  return;
}

//
// Function to save the geometry of a window
void Settings::saveElementGeometry(const QString& elem, const bool& vis, const QSize& size, const QPoint& point)
{
	settings->beginGroup("State");
	settings->setValue(QString("%1_vis").arg(elem), vis);
	settings->setValue(QString("%1_size").arg(elem), size);
	settings->setValue(QString("%1_pos").arg(elem), point);
	settings->endGroup();
}

//
// function to save the state of a widget
void Settings::saveElementState(const QString& elem, const QString& key, const QVariant& val)
{
	settings->beginGroup("State");
	settings->setValue(QString("%1_%2").arg(elem).arg(key), val);
	settings->endGroup();

	return;
}

//
// Function to restore the geometry of a window
void Settings::restoreElementGeometry(const QString& elem, QWidget* win)
{
	settings->beginGroup("State");
	if (settings->value(QString("%1_vis").arg(elem)).toBool() ) {
		win->show();
		win->resize(settings->value(QString("%1_size").arg(elem)).toSize() );
		win->move(settings->value(QString("%1_pos").arg(elem)).toPoint() );
	}
	settings->endGroup();
}

//
// Function to return a setting as a QVariant
QVariant Settings::getSetting(const QString& group, const QString& elem)
{
	QVariant v;
	v.clear();

	settings->beginGroup(group);
	v = settings->value(elem);
	settings->endGroup();

	return v;
}

//
// Function to set ui elements while trying to connect to a notification daemon
void Settings::setNotificationsTrying(const QString& s)
{
	ui.label_serverstatus->setText(s);
	ui.checkBox_notifydaemon->setToolTip("");

	return;
}

//
// Function to set ui elements if a notification daemon is connected
void Settings::setNotificationsConnected(const QString& ttip)
{
	ui.label_serverstatus->clear();
  ui.label_serverstatus->setDisabled(true);
  ui.checkBox_notifydaemon->setToolTip(ttip);
	return;
}

//
// function to set ui elements if we failed to connect to a notification daemon
void Settings::setNotificationsFailed()
{
  ui.checkBox_notifydaemon->setEnabled(false);

	return;
}

//
// Function to return the playlist
QStringList Settings::getPlaylist()
{
	// string list to return
	QStringList sl;
	sl.clear();

	settings->beginGroup("Playlist");
	sl = settings->value("entries").toStringList();
	settings->endGroup();

	return sl;
}

//////////////////////////////////// Private Slots /////////////////
//
void Settings::openEditor(QAbstractButton* button)
{
	QString filename;
	QString qrc;

	if (button == ui.pushButton_editiconfile) {
		filename = "mbmp.icon";
		qrc = QString(":/text/text/icon_def.txt");
	}
	else if (button == ui.pushButton_editkeyfile) {
		filename = "mbmp.keys";
		qrc = QString(":/text/text/sc_def.txt");
	}

	// Convert filename to full path and name
	filename = QString(QDir::homePath() + "/.config/mbmp/" + filename);

	// Initialize the text file if requested
	if (ui.checkBox_startfresh->isChecked() ) {
		ui.checkBox_startfresh->setChecked(false);

		// The target directory must already exist since that is checked when
		// the iconmanager and scmanager are initialized so no need to check here.
		QFile s(qrc);

		if (s.remove(filename) ) {
			if (s.copy(filename) )
				QFile::setPermissions(filename, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
			else
			#if QT_VERSION >= 0x050400
				qCritical("Failed copying %s to %s", qUtf8Printable(qrc), qUtf8Printable(filename) );
			#else
				qCritical("Failed copying %s to %s", qPrintable(qrc), qPrintable(filename) );
			#endif
		}	// if s.copy
		else
		#if QT_VERSION >= 0x050400
			qCritical("Failed removing: %s", qUtf8Printable(filename) );
		#else
			qCritical("Failed removing: %s", qPrintable(filename) );
		#endif
	}	// if startfresh

	// Prompt for an editor to use.
		bool ok;
		QString text = QInputDialog::getText(this, tr("Supply Text Editor Command"),
				tr("A text editor start command needs to be specified if you wish to continue."
				"<p>For GUI editors entering the name of the editor should be sufficient."
				"<br>If you want a console editor enter the terminal command<br>which you wish to run the editor in. (example: xterm -e vi)"
				"<p>Please enter editor start command below."),
				QLineEdit::Normal, editor_string, &ok);

		if (ok && ! text.isEmpty() ) {
			text = text.simplified();
			editor_string = text;	// save for future use
			QStringList args = text.split(' ');
			QString editor = args.first();
			args.removeFirst();
			args << filename;
			QProcess* proc = new QProcess(this);
			proc->startDetached(editor, args);
		}	// if ok and text not empty

	return;
}

//
// Slot to open the color selection dialog and request input.
void Settings::callColorDialog()
{
	QColor color = QColorDialog::getColor(QColor(ui.lineEdit_colorize->text()), this, tr("Colorize Icons"));
	if (color.isValid() ) ui.lineEdit_colorize->setText(color.name() );

	return;
}

//
// Slot to process things when the user changes the icon color
void Settings::iconColorChanged(const QString& col)
{
	// can't change icons in QActions once they are set so just return
	(void) col;
	return;
}

//
//  Slot to find if a specified process is running
bool Settings::isProcessRunning(const QByteArray& str)
{
	QProcess prss;
	prss.start("ps -e", QStringList() );
	prss.waitForFinished();
	QByteArray ba = prss.readAllStandardOutput();

	return ba.contains(QByteArray(" " + str + "\n"));
}

//
// Slot to find if a specified program is available in the environment path
bool Settings::isProgramAvailable(const QString& prog_name)
{
	// get search paths
	QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  QStringList sl_dirs = env.value("PATH").split(':');

	// return false if there are no paths to search
	if (sl_dirs.size() < 1) return false;

	// iterate over the search paths
	for (int i = 0; i < sl_dirs.size(); ++i) {
		QDirIterator dit(QString(sl_dirs.at(i) ), QDirIterator::Subdirectories);
		while (dit.hasNext()) {
			QFileInfo fi(dit.next());
			if (fi.completeBaseName() == prog_name) return true;
		}	// while
	}	// for

	// no found, return false
	return false;
}
