#!/bin/bash
#
#	script to convert the header file to cpp
#
#	-M	all public slots
#	-P	all properties
# -S  all signals
# -s	all scriptable signals
qdbuscpp2xml -M -s ipcagent.h -o org.monkey_business_enterprises.ipcagent.xml
