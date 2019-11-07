#include "XData.h"
#include <cstring>

extern "C"
{
#include <libavutil/time.h>
}


XData::XData()
{
	
}


XData::XData(char *data, int size, long long pts)
{
	this->data = new char[size];
	memcpy(this->data, data, size);
	this->size = size;
	this->pts = pts;
}


XData::~XData()
{
}


void XData::Drop()
{
	if (data)
	{
		delete data;
	}
	data = nullptr;
	size = 0;
}


long long GetCurTime()
{
	return av_gettime();
}