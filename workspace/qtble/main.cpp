#ifdef __arm__
#include <QWSServer>
#endif

#include <QtGui>
#include <GESys/ge_persistent_value.h>

#include "qtble.h"

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	app.setApplicationName("qtble");

#ifdef __arm__
	QWSServer::setBackground(Qt::NoBrush);
#else
	GESys::PersistentValueBase::set_root("/tmp/qtble/");
#endif // __arm__

	Qtble w;

	w.show();
	return app.exec();
}
