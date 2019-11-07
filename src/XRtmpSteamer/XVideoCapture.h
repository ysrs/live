#pragma once

#include "XDataThread.h"
#include "XFilter.h"
#include <vector>


class XVideoCapture : public XDataThread
{
public:
	virtual ~XVideoCapture();

	static XVideoCapture *Get(unsigned char index = 0);

	virtual bool Init(int camIndex) = 0;
	virtual bool Init(const char *url) = 0;
	virtual void Stop() override = 0;
	void ADDFilter(XFilter *f)
	{
		fmutex.lock();
		filters.push_back(f);
		fmutex.unlock();
	}

public:
	int width = 0;
	int height = 0;
	int fps = 0;

protected:
	XVideoCapture();
	std::vector<XFilter*> filters;
	QMutex fmutex;
};

