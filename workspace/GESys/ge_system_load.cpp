#include <unistd.h> // sysconf

#include <QFile>
#include <QTextStream>

#include "ge_system_load.h"


static const int CPU_MAX = 512;


namespace GESys {

SystemLoad::SystemLoad()
{
	_num_cpu = sysconf(_SC_NPROCESSORS_ONLN);
	_cpu.resize(_num_cpu + 1); // one for the first, global values
	update();
}


bool SystemLoad::update()
{
	QFile file("/proc/stat");
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;

	QTextStream stat(&file);
	for (int i = _num_cpu + 1; i--; ) { // atEnd() doesn't work on /proc/ files
		QString cpu;
		stat >> cpu;
		if (!cpu.startsWith("cpu"))
			break;

		cpu = cpu.mid(3);
		int n = cpu.isEmpty() ? 0 : cpu.toInt() + 1;

		unsigned long user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;
		stat >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal >> guest >> guest_nice;

		if (n > _cpu.size())
			return false;

		struct data_t &data = _cpu[n];
		data.prev_idle = data.idle;
		data.prev_busy = data.busy;

		data.idle = idle + iowait;
		data.busy = user + nice + system + irq + softirq + steal;
	}
	return true;
}


double SystemLoad::load(int cpu) const
{
	if (cpu < 0)
		cpu = 0;
	else if (cpu > _num_cpu)
		return NAN;
	else
		cpu++;

	const struct data_t &data = _cpu[cpu];
	double total = (data.idle + data.busy) - (data.prev_idle + data.prev_busy);
	double busy = data.busy - data.prev_busy;
	return busy / total;
}

} // namespace GESys
