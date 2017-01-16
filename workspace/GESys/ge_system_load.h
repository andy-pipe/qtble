#ifndef _GE_SYSTEM_LOAD_H_
#define _GE_SYSTEM_LOAD_H_

#include <QVector>


namespace GESys {

/*
 * class that offers system load as normalized floating point number for the whole
 * system or a specific CPU
 * Usage:
 *         SystemLooad load;
 *         while (1) {
 *                 sleep(1);
 *                 load.update();
 *                 cerr << 100 * load.load(0) << '%' << endl; // -1: system, 0: cpu0, 1: cpu1 etc.
 *         }
 */
class SystemLoad {
public:
	SystemLoad();

	/*
	 * reads in new parameter set, returns true on success and false on error
	 */
	bool update();

	/*
	 * returns system or cpu load as nomalized number for the interval between the last two
	 * calls to the update() method; the constructor calls update() once already
	 * returns NAN on error
	 */
	double load(int cpu = -1) const;
	double percent(int cpu = -1) const { return 100.0 * load(cpu); }

	/*
	 * returns number of online CPUs
	 */
	int nproc() const { return _num_cpu; }

private:
	int _num_cpu;
	struct data_t {
		unsigned long prev_idle = -1;
		unsigned long prev_busy = -1;
		unsigned long idle = 0;
		unsigned long busy = 0;
	};
	QVector<struct data_t> _cpu;
};

} // namespace GESys

#endif // _GE_SYSTEM_LOAD_H_
