#ifndef _GE_SYSFILE_H_
#define _GE_SYSFILE_H_

#include <QFile>
#include <QTimer>


namespace GESys {

/*
 * class that wraps a single sysfs file and provides easy read/write access
 */
class SysFile : public QObject {
	Q_OBJECT
public:
	/*
	 * parameters:
	 *     path: sysfs path, e.g. "/sys/class/backlight/pwm-backlight.0/brightness"
	 *     interval: optional watching interval in milliseconds
	 */
	SysFile(const char *path, int interval = 100, bool readonly = false);
	virtual ~SysFile();

	/*
	 * set watching interval (can be done in constructor)
	 * parameters:
	 *     interval: interval
	 */
	inline void setInterval(int interval) { _interval = interval; }

	/*
	 * return current sysfs value, or -1 if file couldn't get opened
	 */
	int value();

public slots:
	/*
	 * set sysfs value; emits "valueChanged" signal
	 */
	void setValue(int);

	/*
	 * start watching; runs timer that emits "valueChanged"
	 */
	void watch();

	/*
	 * stop watching
	 */
	void unwatch();

signals:
	/*
	 * emitted if setValue() actually changes the sysfs file contents or
	 * if the file is being watched and a change has been noticed
	 */
	void valueChanged(int);

private:
	QString _path;
	QFile _file;
	QTimer _timer;
	int _interval;
	int _lastval;
	bool _watching;
	bool _valid;
	bool _readonly;

private slots:
	void timeout();
};

} // namespace GESys

#endif // _GE_SYSFILE_H_
