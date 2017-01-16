#!/bin/bash
cd workspace/qtble
if [ -a .gdbinit ]; then
	TMP=$(mktemp .gdbinit.XXX) && trap 'rm -f $TMP' EXIT HUP INT QUIT PIPE TERM || exit 1
	sed -e "s:^\(set[ 	]\+sysroot[ 	]\+\).*:\1$GELIN_SYSROOT:" <.gdbinit >$TMP && mv $TMP .gdbinit
else
	cat >.gdbinit <<-EOT
	source $HOME/.gdbinit
	set sysroot $GELIN_SYSROOT
	handle SIGILL nostop
	EOT
fi
