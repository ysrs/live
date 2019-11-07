#pragma once
class XData
{
public:
	XData();
	// 创建控件并复制data内容
	XData(char *data, int size, long long pts = 0);
	~XData();

	void Drop();

public:
	char *data = nullptr;
	int size = 0;
	long long pts = 0;
};

// 获取当前时间戳，单位：微妙
long long GetCurTime();

