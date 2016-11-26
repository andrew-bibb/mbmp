mak#  We need the qt libraries, we want compiler warnings on, and this is a release version of the program  
CONFIG += qt
CONFIG += warn_on
CONFIG += release

# define a path for the man page
MBMP_DOC_PATH = $$(USE_MANPATH)
isEmpty ( MBMP_DOC_PATH ) {
	MBMP_DOC_PATH = "/usr/share/man"
}

# documentation (manpage)
documentation.path = $$MBMP_DOC_PATH/man1
documentation.files = ./misc/manpage/mbmp.1.gz
documentation.CONFIG = no_check_exist
documentation.extra = gzip --force --keep ./misc/manpage/mbmp.1
INSTALLS += documentation

# program icon
LIST = 16 20 22 24 32 36 40 48 64 72 96 128 192 256 384 512
for(a, LIST) {
	icon$${a}.path = /usr/share/icons/hicolor/$${a}x$${a}/apps
	icon$${a}.files = ./images/application/$${a}x$${a}/mbmp.png
	INSTALLS += icon$${a}
}
exists(./images/application/scalable/mbmp.svg) {
	iconsvg.path = /usr/share/icons/hicolor/scalable/apps
	iconsvg.files = ./images/application/scalable/mbmp.svg
	INSTALLS += iconsvg
}


# desktop file
desktop.path = /usr/share/applications
desktop.files = ./misc/desktop/mbmp.desktop
desktop.extra = gtk-update-icon-cache /usr/share/icons/hicolor
INSTALLS += desktop

#  Widgets needed for QT5, 
QT += widgets
QT += core
QT += dbus
QT += network

TARGET = mbmp
TEMPLATE = app
target.path = /usr/bin/
INSTALLS += target

# dbus
DBUS_ADAPTORS		+= ./code/ipc/org.monkey_business_enterprises.ipcagent.xml
DBUS_INTERFACES	+= ./code/ipc/org.monkey_business_enterprises.ipcagent.xml

#	header files
HEADERS		+= ./code/resource.h
HEADERS 	+= ./code/playerctl/playerctl.h
HEADERS 	+= ./code/playlist/playlist.h
HEADERS 	+= ./code/playlist/playlistitem.h
HEADERS 	+= ./code/gstiface/gstiface.h
HEADERS		+= ./code/streaminfo/streaminfo.h
HEADERS		+= ./code/videowidget/videowidget.h
HEADERS		+= ./code/scrollbox/scrollbox.h
HEADERS		+= ./code/settings/settings.h
HEADERS		+= ./code/iconman/iconman.h
HEADERS		+= ./code/notify/notify.h
HEADERS		+= ./code/scman/scman.h
HEADERS		+= ./code/ipc/ipcagent.h
HEADERS		+= ./code/mbman/mbman.h

#	forms
FORMS		+= ./code/playerctl/ui/playerctl.ui
FORMS		+= ./code/playlist/ui/playlist.ui
FORMS		+= ./code/streaminfo/ui/streaminfo.ui
FORMS		+= ./code/scrollbox/ui/scrollbox.ui
FORMS		+= ./code/settings/ui/settings.ui

#	sources
SOURCES	+= ./code/main.cpp
SOURCES	+= ./code/playerctl/playerctl.cpp
SOURCES	+= ./code/playlist/playlist.cpp
SOURCES	+= ./code/playlist/playlistitem.cpp
SOURCES	+= ./code/gstiface/gstiface.cpp
SOURCES += ./code/streaminfo/streaminfo.cpp
SOURCES += ./code/videowidget/videowidget.cpp
SOURCES += ./code/scrollbox/scrollbox.cpp
SOURCES += ./code/settings/settings.cpp
SOURCES += ./code/iconman/iconman.cpp
SOURCES += ./code/notify/notify.cpp
SOURCES += ./code/scman/scman.cpp
SOURCES += ./code/ipc/ipcagent.cpp
SOURCES += ./code/mbman/mbman.cpp

#	resource files
RESOURCES 	+= mbmp.qrc

# following 4 lines if we want the source the QT dynamic libraries
# with our exec file (or exepath/lib or exepath/libs)
#QMAKE_LFLAGS += -Wl,-rpath=\\\$\$ORIGIN
#QMAKE_LFLAGS += -Wl,-rpath=\\\$\$ORIGIN/lib
#QMAKE_LFLAGS += -Wl,-rpath=\\\$\$ORIGIN/libs
#QMAKE_RPATH=

# files needed for gstreamer
INCLUDEPATH += /usr/include/gstreamer-1.0
INCLUDEPATH += /usr/include/glib-2.0
INCLUDEPATH += /usr/lib/glib-2.0/include
INCLUDEPATH += /usr/lib/gstreamer-1.0

# gstreamer libraries
CONFIG += link_pkgconfig
PKGCONFIG += gstreamer-1.0
PKGCONFIG += gstreamer-video-1.0
PKGCONFIG += gstreamer-pbutils-1.0
PKGCONFIG += x11
PKGCONFIG += xext

# translations
TRANSLATIONS += ./translations/mbmp_en_US.ts
TRANSLATIONS += ./translations/mbmp_ru_RU.ts


##  Place all object files in their own directory and moc files in their own directory
##  This is not necessary but keeps things cleaner.
OBJECTS_DIR = ./object_files
MOC_DIR = ./moc_files

sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro

