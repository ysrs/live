#pragma once

#include <QtWidgets/QWidget>
#include "ui_xrtmpsteamer.h"

class XRtmpSteamer : public QWidget
{
	Q_OBJECT

public:
	XRtmpSteamer(QWidget *parent = Q_NULLPTR);

public slots:
	void Stream();

private:
	Ui::XRtmpSteamerClass ui;
};
