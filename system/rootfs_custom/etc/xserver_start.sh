#!/bin/sh

#
# This example-script is called by xinit
# if this script exits, the xserver will be stopped also
#

# stop splashscreen, if running and wait for exit
while [ "${SPLASHSCREEN_RUNNING_FLAG_FILE}" ] && [ -e "${SPLASHSCREEN_RUNNING_FLAG_FILE}" ] ; do
	echo "splashscreen running -> stop"
	touch "${SPLASHSCREEN_EXIT_FLAG_FILE}" > /dev/null 2>&1
	sleep 1
done

# redraw
/usr/bin/xsetroot -solid white

# start of example-application
# /usr/bin/xeyes -geometry 320x240

