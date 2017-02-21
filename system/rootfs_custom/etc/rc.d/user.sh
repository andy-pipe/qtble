#!/bin/sh

fifoin="/tmp/eingang"
fifoout="/tmp/ausgang"
btadr="B0:B4:48:BE:2B:02"

echo "User startup script"

mkdir -p /tmp/run
/etc/init.d/S30dbus start
/usr/libexec/bluetooth/bluetoothd &

ntpd -gq &
sh /usr/local/bin/startppp.sh provider &

if ! [ -p "$fifoin" ]
then
	mkfifo "$fifoin"
fi

if ! [ -p "$fifoout" ]
then
	mkfifo "$fifoout"
fi

if ! [ -p "$tAMB" ]
then
	mkfifo "$tAMB"
fi

if ! [ -p "$tOBJ" ]
then
	mkfifo "$tOBJ"
fi

gatttool -b "$btadr" -I <> "$fifoin" > "$fifoout" &
/usr/local/bin/qtble -qws



