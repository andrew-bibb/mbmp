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
qdbuscpp2xml -m -s -P ipcagent.h -o org.monkey_business_enterprises.ipcagent.xml
#
#	utility seems to modify the annotation entry for signals with QVariantMap
# This is not portable.  It changes the first instance and it only works 
# because I know the first instance is the one I want to change.
sed -i '0,/Out0/{s/Out0/In0/}' ./org.monkey_business_enterprises.ipcagent.xml
