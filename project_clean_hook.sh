SUBDIRS="GESys qtble"

rm -rf \
	${GELIN_ROOTFS_CUSTOM}/usr/local/bin/qtble

for i in $SUBDIRS; do (
	DIR=${GELIN_ECLIPSE_WORKSPACE}/$i
	cd $DIR
	[ -e Makefile ] && make distclean
	rm -rvf \
		debug/ \
		release/ \
		Makefile* \
		callgrind.out.* \
		cachegrind.out.* \
		massif.out.*
) done
