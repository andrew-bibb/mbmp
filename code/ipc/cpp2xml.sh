#!/bin/bash
#
#	script to convert the header file to cpp
#
#	-M	all public slots
# -m  all scriptable slots
#	-P	all properties
# -p  all scriptable properties
# -S  all signals
# -s	all scriptable signals

# Adaptor and interface code is generated here because the QT autogenerator
# creates namespaces.  Normally this if fine, but the compiler complains about
# declaring duplicate namespaces. Fix that with subsequent commands. This is not
# portable, only works since I know the format of the file
# 
qdbuscpp2xml -m -s -P ipcagent.h -o org.monkey_business_enterprises.ipcagent.xml
qdbusxml2cpp org.monkey_business_enterprises.ipcagent.xml -a ipcagent_adaptor
qdbusxml2cpp org.monkey_business_enterprises.ipcagent.xml -p ipcagent_interface

qdbuscpp2xml -m -s -P ipcplayer.h -o org.monkey_business_enterprises.ipcplayer.xml
qdbusxml2cpp org.monkey_business_enterprises.ipcplayer.xml -a ipcplayer_adaptor
qdbusxml2cpp org.monkey_business_enterprises.ipcplayer.xml -p ipcplayer_interface

# remove last 8 lines and put in lines we need
sed -i -e :a -e '$d;N;2,8ba' -e 'P;D' ipcplayer_interface.h 
echo  >> ipcplayer_interface.h
echo 'typedef ::OrgMprisMediaPlayer2PlayerInterface Player;' >> ipcplayer_interface.h
echo  >> ipcplayer_interface.h
echo '#endif' >> ipcplayer_interface.h

#
#	utility seems to modify the annotation entry for signals with QVariantMap
# This is not portable.  It changes the first instance and it only works 
# because I know the first instance is the one I want to change.
#sed -i '0,/Out0/{s/Out0/In0/}' ./org.monkey_business_enterprises.ipcagent.xml


