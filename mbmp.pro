#  We need the qt libraries, we want compiler warnings off, and this is a release version of the program  
#CONFIG += qt warn_off release 
CONFIG += qt warn_on release

#  Widgets needed for QT5, 
QT += widgets

TARGET = mbmp
TEMPLATE = app
target.path = /usr/bin/
INSTALLS += target

#	header files
HEADERS		+= ./code/resource.h
HEADERS 	+= ./code/playerctl/playerctl.h
HEADERS 	+= ./code/keymap/keymap.h
HEADERS 	+= ./code/playlist/playlist.h
HEADERS 	+= ./code/playlist/playlistitem.h
HEADERS 	+= ./code/gstiface/gstiface.h
HEADERS		+= ./code/streaminfo/streaminfo.h
HEADERS		+= ./code/videowidget/videowidget.h
HEADERS		+= ./code/scrollbox/scrollbox.h

#	forms
FORMS		+= ./code/playerctl/ui/playerctl.ui
FORMS		+= ./code/playlist/ui/playlist.ui
FORMS		+= ./code/streaminfo/ui/streaminfo.ui
FORMS		+= ./code/scrollbox/ui/scrollbox.ui

#	sources
SOURCES	+= ./code/main.cpp
SOURCES	+= ./code/playerctl/playerctl.cpp
SOURCES	+= ./code/keymap/keymap.cpp
SOURCES	+= ./code/playlist/playlist.cpp
SOURCES	+= ./code/playlist/playlistitem.cpp
SOURCES	+= ./code/gstiface/gstiface.cpp
SOURCES += ./code/streaminfo/streaminfo.cpp
SOURCES += ./code/videowidget/videowidget.cpp
SOURCES += ./code/scrollbox/scrollbox.cpp

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

# gstreamer libraries
CONFIG += link_pkgconfig
PKGCONFIG += gstreamer-1.0
PKGCONFIG += gstreamer-video-1.0
PKGCONFIG += gstreamer-pbutils-1.0

##  Place all object files in their own directory and moc files in their own directory
##  This is not necessary but keeps things cleaner.
OBJECTS_DIR = ./object_files
MOC_DIR = ./moc_files

sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro

