#include "XFilter.h"
#include "XBilateralFilter.h"

#include <iostream>


using namespace std;

XFilter::XFilter()
{
}


XFilter::~XFilter()
{
}


XFilter *XFilter::Get(XFilterType type)
{
	static XBilateralFilter xbf;
	switch (type)
	{
	case XBILATERAL:
		return &xbf;
		break;
	default:
		break;
	}

	return nullptr;
}

bool XFilter::Set(std::string key, double value)
{
	if (paras.find(key) == paras.end())
	{
		cout << "para " << key << " is not support!" << endl;
		return false;
	}
	paras[key] = value;
	return true;
}
