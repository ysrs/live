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

	// 启动线程
	virtual bool Start();
	// 退出线程，并等待线程退出（阻塞）
	virtual void Stop();
	// 在列表结尾插入
	virtual void Push(XData d);
	// 读取列表中最早的数据，返回数据需要调用XData.Drop清理
	virtual XData Pop();

	virtual void Clear();

public:
	// （缓冲）列表最大值，超出删除最早的数据
	int maxList = 100;

protected:
	std::list<XData> datas;	// 存放交互数据（插入策略：先进先出）
	int dataCount = 0;		// 交互数据列表大小
	QMutex mutex;			// 互斥访问datas
	bool isExit = false;	// 线程退出标志
};

