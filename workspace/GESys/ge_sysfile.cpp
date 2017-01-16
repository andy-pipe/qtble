#include <QDebug>

#include "ge_sysfile.h"


namespace GESys {

SysFile::SysFile(const char *path, int interval, bool readonly) :
	_path(path),
	_file(path),
	_timer(this),
	_interval(interval),
	_watching(false),
	_valid(false),
	_readonly(readonly)
{
	if (!_file.open((readonly ? QIODevice::ReadOnly : QIODevice::ReadWrite) | QIODevice::Text)) {

		qWarning() << "cannot open file '" << path << '\'';
		return;
	}

	connect(&_timer, SIGNAL(timeout()), this, SLOT(timeout()));
	_lastval = value();
	_valid = true;
}


SysFile::~SysFile()
{
	if (_valid) {
		unwatch();
		_file.close();
	}
}


int SysFile::value()
{
	if (!_valid)
		return -1;
	_file.seek(0);
	QTextStream in(&_file);
	int v;
	in >> v;
	return v;
}


void SysFile::setValue(int v)
{
	if (!_valid || _readonly)
		return;
	_lastval = value();
	_file.seek(0);
	QTextStream out(&_file);
	out << v;
	if (v != _lastval)
		emit valueChanged(_lastval = v);
}


void SysFile::watch()
{
	if (_watching || !_valid)
		return;

	_timer.start(_interval);
	_watching = true;
	return;
}


void SysFile::unwatch()
{
	if (!_watching || !_valid)
		return;

	_timer.stop();
	_watching = false;
	return;
}


void SysFile::timeout()
{
	int v = value();
	if (v != _lastval)
		emit valueChanged(_lastval = v);
}

} // namespace GESys
