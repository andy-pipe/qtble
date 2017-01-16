#!/bin/sh 
if ! [ -e /etc/etc_data/ppp ] ; then
# directory ppp does not exist
# copy ppp-configuration directory to temporary directory
cp -R /etc/ppp.conf /etc/etc_data/ppp.tmp
# make sure content and metadata were written to disk
fsync /etc/etc_data/ppp.tmp /etc/etc_data
# perform atomic rename
mv /etc/etc_data/ppp.tmp /etc/etc_data/ppp
# make sure metadata was written to disk
fsync /etc/etc_data
fi
