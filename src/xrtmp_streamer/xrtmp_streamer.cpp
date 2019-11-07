#include "xrtmp_streamer.h"
#include <iostream>
#include "XController.h"


static bool isStream = false;

using namespace std;
XRtmpSteamer::XRtmpSteamer(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
}

void XRtmpSteamer::Stream()
{
	cout << "Stream" << endl;
	if (isStream)
	{
		isStream = false;
		ui.startButton->setText(QString::fromLocal8Bit("¿ªÊ¼"));
		XController::Get()->Stop();
	}
	else
	{
		isStream = true;
		ui.startButton->setText(QString::fromLocal8Bit("Í£Ö¹"));
		QString url = ui.inUrl->text();
		bool ok = false;
		int camIndex = url.toInt(&ok);
		if (ok)
		{
			XController::Get()->camIndex = camIndex;
		}
		else
		{
			XController::Get()->inUrl = url.toStdString();
		}
		XController::Get()->outUrl = ui.outUrl->text().toStdString();
		XController::Get()->Set("b", (ui.face->currentIndex() + 1) * 3);

		XController::Get()->Start();
	}
}


