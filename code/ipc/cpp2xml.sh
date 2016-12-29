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
qdbuscpp2xml -m -s -P mediaplayer2.h -o org.monkey_business_enterprises.mediaplayer2.xml
qdbusxml2cpp org.monkey_business_enterprises.mediaplayer2.xml -a mediaplayer2_adaptor
qdbusxml2cpp org.monkey_business_enterprises.mediaplayer2.xml -p mediaplayer2_interface

qdbuscpp2xml -m -s -P mediaplayer2player.h -o org.monkey_business_enterprises.mediaplayer2player.xml
qdbusxml2cpp org.monkey_business_enterprises.mediaplayer2player.xml -a mediaplayer2player_adaptor
qdbusxml2cpp org.monkey_business_enterprises.mediaplayer2player.xml -p mediaplayer2player_interface

# remove last 8 lines and put in lines we need
#sed -i -e :a -e '$d;N;2,8ba' -e 'P;D' mediaplayer2player_interface.h 
#echo  >> mediaplayer2player_interface.h
#echo 'typedef ::OrgMprisMediaPlayer2PlayerInterface Player;' >> mediaplayer2player_interface.h
#echo  >> mediaplayer2player_interface.h
#echo '#endif' >> mediaplayer2player_interface.h



