#pragma once
#include <string>
#include "xdata_thread.h"


class XController : public XDataThread
{
public:
	virtual ~XController();

	static XController *Get();
	bool Start() override;
	void Stop() override;
	// 设定美颜参数
	virtual bool Set(std::string key, double val);
	void run() override;

public:
	std::string outUrl;
	int camIndex = -1;
	std::string inUrl = "";
	std::string err = "";

protected:
	XController();

protected:
	int vIndex = 0;	// 视频流索引
	int aIndex = 1;	// 音频流索引
};

