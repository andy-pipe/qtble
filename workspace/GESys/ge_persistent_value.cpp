// Melchior FRANZ <melchior.franz@ginzinger.com>  2014-12-11

#include <fcntl.h>      // open
#include <stdio.h>      // perror
#include <sys/stat.h>   // open
#include <sys/types.h>  // open

#include "ge_persistent_value.h"


QString GESys::PersistentValueBase::_root_dir("/data/");


bool GESys::PersistentValueBase::dirsync(const QString &dirpath) const
{
	// opening directory as file and calling fsync on it flushes dir catalog
	int fh = open(qPrintable(dirpath), O_RDONLY);
	if (fh == -1) {
		perror("open");
		return false;
	}

	bool ret = true;
	if (fsync(fh) == -1) {
		perror("fsync");
		ret = false;
	}

	if (close(fh) == -1) {
		perror("close");
		ret = false;
	}

	return ret;
}
