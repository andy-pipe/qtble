#!/bin/bash

set -e

############################################################################################
# BEGIN: ARGUMENT PARSING AND INSTALL HELPER FUNCTIONS                                     #
############################################################################################

# use this as flags for build
GELIN_SCRIPT_DEBUG="no"
GELIN_SCRIPT_VERBOSE="no"
GELIN_SCRIPT_JOBS=1

args=$(getopt -a -l debug -o vj: -- "$@")
if [ $? -ne 0 ] ; then
	usage
	exit 1
fi
eval set -- "$args"
while [ $# -gt 0 ]
do
	case "$1" in
	--debug)
		GELIN_SCRIPT_DEBUG="yes"
		;;
	-v)
		GELIN_SCRIPT_VERBOSE="yes"
		;;
	-j)
		GELIN_SCRIPT_JOBS=$2
		shift
		;;
	--) 
		shift
		break
		;;
	*)
		perr "Invalid option: $1"
		exit 1
		;;
	esac
	shift
done


# 
# helper for installing files/dirs from $GELIN_TARGET to $GELIN_ROOTFS_CUSTOM
#
# usage example: gelin_install /usr/bin/mpg123
# installs the binary mpg123 from $GELIN_TARGET/usr/bin/ to $GELIN_ROOTFS_CUSTOM/usr/bin
#
gelin_install() {
	src=${GELIN_TARGET}/$1
	dest=${GELIN_ROOTFS_CUSTOM}/$1

	if ! [ -e "${src}" ] ; then
		echo "ERROR: $0: gelin_install: ${src} does not exist!"
		exit 1
	fi

	mkdir $(dirname ${dest}) -p
	cp -a ${src} ${dest}
}


############################################################################################
# END: ARGUMENT PARSING AND INSTALL HELPER FUNCTIONS                                       #
############################################################################################



############################################################################################
# BEGIN: CUSTOM CODE                                                                       # 
############################################################################################

INSTALL=${INSTALL:-install --strip-program=${CROSS_COMPILE}strip}
SUBDIRS="GESys qtble"

# install version.txt
mkdir -p ${GELIN_ROOTFS_CUSTOM}/usr/local/bin ${GELIN_ROOTFS_CUSTOM}/boot
cp ${GELIN_PROJECT_PATH}/version.txt ${GELIN_ROOTFS_CUSTOM}/boot/version.txt

# (re)create makefiles
for i in $SUBDIRS; do
	(cd ${GELIN_ECLIPSE_WORKSPACE}/$i && qmake CONFIG+=debug_and_release)
done

# remove app, because otherwise target changes may not trigger a new build
rm -f ${GELIN_ECLIPSE_WORKSPACE}/qtble/qtble

# install application
if [ "$GELIN_SCRIPT_DEBUG" = "yes" ]; then
	echo "compile and install qtble (debug)"
	for i in $SUBDIRS; do
		make -C ${GELIN_ECLIPSE_WORKSPACE}/$i -j ${GELIN_SCRIPT_JOBS} debug
	done

	${INSTALL} -m 755 ${GELIN_ECLIPSE_WORKSPACE}/qtble/qtble ${GELIN_ROOTFS_CUSTOM}/usr/local/bin/
	if ! [ -e ${GELIN_ROOTFS_CUSTOM}/usr/local/bin/debug ]; then
		touch ${GELIN_ROOTFS_CUSTOM}/usr/local/bin/debug
	fi
else
	echo "compile and install qtble (release)"
	for i in $SUBDIRS; do
		make -C ${GELIN_ECLIPSE_WORKSPACE}/$i -j ${GELIN_SCRIPT_JOBS} release
	done

	${INSTALL} -s -m 755 ${GELIN_ECLIPSE_WORKSPACE}/qtble/qtble ${GELIN_ROOTFS_CUSTOM}/usr/local/bin/
	if [ -e ${GELIN_ROOTFS_CUSTOM}/usr/local/bin/debug ]; then
		rm ${GELIN_ROOTFS_CUSTOM}/usr/local/bin/debug
	fi
fi

#gelin_install usr/bin/hciconfig
#gelin_install usr/bin/hcitool
#gelin_install usr/bin/hcidump
#gelin_install usr/bin/gatttool

gelin_install etc/init.d/S30dbus
gelin_install usr/bin/dbus-daemon
gelin_install usr/bin/dbus-uuidgen
gelin_install usr/bin/dbus-monitor
gelin_install usr/share/dbus-1/system.conf
gelin_install usr/share/dbus-1/session.conf
gelin_install etc/dbus-1/system.d/bluetooth.conf

#gelin_install usr/libexec/bluetooth/bluetoothd

############################################################################################
# END: CUSTOM CODE                                                                         # 
############################################################################################

