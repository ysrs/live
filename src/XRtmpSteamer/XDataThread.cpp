#include "XDataThread.h"


XDataThread::XDataThread()
{
}


XDataThread::~XDataThread()
{
}


bool XDataThread::Start()
{
	isExit = false;
	QThread::start();

	return true;
}



void XDataThread::Stop()
{
	isExit = true;
	wait();
}

void XDataThread::Push(XData d)
{
	mutex.lock();
	if (datas.size() >= maxList)
	{
		datas.front().Drop();
		datas.pop_front();
	}
	datas.push_back(d);
	mutex.unlock();
}

XData XDataThread::Pop()
{
	mutex.lock();
	if (datas.empty())
	{
		mutex.unlock();
		return XData();
	}
	XData d = datas.front();
	datas.pop_front();
	mutex.unlock();
	return d;
}


void XDataThread::Clear()
{
	mutex.lock();
	while (!datas.empty())
	{
		datas.front().Drop();
		datas.pop_front();
	}
	mutex.unlock();
}

