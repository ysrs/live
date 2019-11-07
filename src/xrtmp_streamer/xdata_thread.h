#pragma once
#include <list>
#include <QThread>
#include <QMutex>
#include "XData.h"



class XDataThread : public QThread
{
public:
	XDataThread();
	virtual ~XDataThread();

	// �����߳�
	virtual bool Start();
	// �˳��̣߳����ȴ��߳��˳���������
	virtual void Stop();
	// ���б��β����
	virtual void Push(XData d);
	// ��ȡ�б�����������ݣ�����������Ҫ����XData.Drop����
	virtual XData Pop();

	virtual void Clear();

public:
	// �����壩�б����ֵ������ɾ�����������
	int maxList = 100;

protected:
	std::list<XData> datas;	// ��Ž������ݣ�������ԣ��Ƚ��ȳ���
	int dataCount = 0;		// ���������б��С
	QMutex mutex;			// �������datas
	bool isExit = false;	// �߳��˳���־
};

