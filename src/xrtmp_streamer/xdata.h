#pragma once
class XData
{
public:
	XData();
	// �����ؼ�������data����
	XData(char *data, int size, long long pts = 0);
	~XData();

	void Drop();

public:
	char *data = nullptr;
	int size = 0;
	long long pts = 0;
};

// ��ȡ��ǰʱ�������λ��΢��
long long GetCurTime();

