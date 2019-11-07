#pragma once

#include <QtWidgets/QWidget>
#include "ui_xrtmp_streamer.h"

class XRtmpSteamer : public QWidget
{
	Q_OBJECT

public:
	XRtmpSteamer(QWidget *parent = Q_NULLPTR);

public slots:
	void Stream();

private:
	Ui::XRtmpStreamerClass ui;
};
