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
#	utility seems to create the annotation entry for signals with QVariantMap
# wrong, this corrects it
sed -i 's|Out0|In0|g' ./org.monkey_business_enterprises.ipcagent.xml
