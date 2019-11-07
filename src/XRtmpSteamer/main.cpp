#include "xrtmpsteamer.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	XRtmpSteamer w;
	w.show();
	return a.exec();
}
