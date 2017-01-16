// Melchior FRANZ <melchior.franz@ginzinger.com>  2013-08-18

#ifndef _GE_PERSISTENT_VALUE_H_
#define _GE_PERSISTENT_VALUE_H_

#include <cerrno>
#include <unistd.h>    // fsync

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QTextCodec>
#include <QTextStream>


namespace GESys {

/*
 * helper class for defining a global root directory that is prepended to relative paths
 * The directory is automatically created.
 */
class PersistentValueBase {
public:
	static void set_root(const QString &dir) { _root_dir = dir; }

protected:
	bool dirsync(const QString &dirpath) const;

private:
	static QString _root_dir;
};


/*
 * template class that maintains a simple data type as a file with the given path.
 *
 * The class instance can be used much like a regular variable. Note, however, that
 * any read or write access may immediately read from or write to the disk! Values
 * are only written if they differ from the existing value. If the file doesn't exist,
 * then the default value is assumed and returned on reading. As a consequence, the
 * file won't be created as long as the value hasn't been changed after initialization.
 *
 * Example:
 *
 *     GESys::PersistentValueBase::set_root("/tmp/");         // prefix for relative paths
 *
 *     GESys::PersistentValue<unsigned>("counts", 0) counts;  // writes to /tmp/counts
 *     counts++;
 *
 *     GESys::PersistentValue<QString> screensaver("/data/screensaver", "none");
 *     qDebug("%s", qPrintable(screensaver));
 *     screensaver = "clock";
 */
template<class T>
class PersistentValue : public PersistentValueBase {
public:
	PersistentValue(const QString &path, T default_value);
	virtual ~PersistentValue() {}

	// value getter, used by overloaded operators
	T value(bool *ok = nullptr);

	// value setter, used by overloaded operators
	bool set_value(T value);

	// access value x as:  x() or T(x) or, if the context is unambiguous, just as x
	inline operator T() { return value(); }
	inline T operator()() { return value(); }
	inline T operator=(T v) { set_value(v); return v; }
	inline T operator+(T v) { return value() + v; }
	inline T operator-(T v) { return value() - v; }
	inline T operator*(T v) { return value() * v; }
	inline T operator/(T v) { return value() / v; }
	inline T operator%(T v) { return value() % v; }
	inline T operator+=(T v) { v = value() + v; set_value(v); return v; }
	inline T operator-=(T v) { v = value() - v; set_value(v); return v; }
	inline T operator*=(T v) { v = value() * v; set_value(v); return v; }
	inline T operator/=(T v) { v = value() / v; set_value(v); return v; }
	inline T operator++(int) { T v = value(); set_value(v + 1); return v; }
	inline T operator--(int) { T v = value(); set_value(v - 1); return v; }
	inline T operator++() { T v = value() + 1; set_value(v); return v; }
	inline T operator--() { T v = value() - 1; set_value(v); return v; }
	inline bool operator!() { return !value(); }
	inline bool operator==(T v) { return value() == v; }
	inline bool operator!=(T v) { return value() != v; }
	inline bool operator<(T v) { return value() < v; }
	inline bool operator>(T v) { return value() > v; }
	inline bool operator<=(T v) { return value() <= v; }
	inline bool operator>=(T v) { return value() >= v; }

private:
	static QMutex _mutex;
	static QTextCodec *_codec;
	QString _path;
	QString _dir;
	T _default_value;
};


template<class T> QMutex PersistentValue<T>::_mutex;
template<class T> QTextCodec *PersistentValue<T>::_codec = QTextCodec::codecForName("UTF-8");


template<class T>
PersistentValue<T>::PersistentValue(const QString &path, T default_value) :
	_default_value(default_value)
{
	Q_ASSERT(_codec);

	QString p(path);
	if (!p.startsWith('/'))
		p.prepend(_root_dir);

	QFileInfo info(p);
	QDir::root().mkpath(info.absolutePath());
	_path = info.absoluteFilePath();

	// remember directory path for dir fsync()
	QDir d(_path);
	d.cdUp();
	_dir = d.path();
}


template<class T>
T PersistentValue<T>::value(bool *ok)
{
	QMutexLocker lock(&_mutex);
	if (!QFile::exists(_path))
		return _default_value;

	QFile file(_path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		qDebug("%s: reading from file '%s': %s", __PRETTY_FUNCTION__,
				qPrintable(_path), strerror(errno));
		if (ok)
			*ok = false;
		return _default_value;
	}

	QTextStream in(&file);
	in.setCodec(_codec);
	T val;
	in >> val;
	if (ok)
		*ok = true;
	return val;
}


template<class T>
bool PersistentValue<T>::set_value(T val)
{
	QMutexLocker lock(&_mutex);
	QFile file(_path);
	T lastval;

	if (file.exists()) {
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
			qDebug("%s: reading file '%s': %s", __PRETTY_FUNCTION__,
					qPrintable(_path), strerror(errno));
			return false;
		}
		QTextStream in(&file);
		in.setCodec(_codec);
		in >> lastval;
	} else {
		lastval = _default_value;
	}

	if (val == lastval)
		return true;

	file.close();
	file.setFileName(_path + ".tmp");
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		qDebug("%s: writing to file '%s': %s", __PRETTY_FUNCTION__,
				qPrintable(_path), strerror(errno));
		return false;
	}

	QTextStream out(&file);
	out.setRealNumberNotation(QTextStream::FixedNotation);
	out.setRealNumberPrecision(20);
	out.setCodec(_codec);
	out << val << '\n';

	// flush file contents and directory catalog data to storage device
	file.flush();
	if (fsync(file.handle()) == -1)
		return false;
	file.close();

	if (!dirsync(_dir))
		return false;

	if (rename(qPrintable(_path + ".tmp"), qPrintable(_path)) == -1) {
		qDebug("%s: renaming file '%s': %s", __PRETTY_FUNCTION__,
				qPrintable(_path), strerror(errno));
		return false;
	}

	if (!dirsync(_dir))
		return false;

	return true;
}

} // namespace GESys

#endif // _GE_PERSISTENT_VALUE_H_
