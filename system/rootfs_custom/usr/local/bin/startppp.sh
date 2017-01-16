#!/bin/sh
# Wait for Modem to be up and running, then start ppp
INTERFACE="/dev/$(head -1 /etc/ppp/options-mobile)"
while [ true ]; do
    if [ -c ${INTERFACE} ] ; then
        echo "${INTERFACE} OK"
        pon $1
        break
    fi
done


