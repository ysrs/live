#include "xaudio_record.h"
#include <QAudioInput>
#include <QMutex>
#include <iostream>
#include <list>


using namespace std;

class CXAudioRecord : public XAudioRecord
{
public:
	void run() override
	{
		cout << "������Ƶ¼���߳�" << endl;
		// һ�ζ�ȡһ֡��Ƶ���ֽ���
		int readSize = nbSamples*channels*sampleByte;
		char *buf = new char[readSize];
		while (!isExit)
		{
			// ��ȡ��¼�Ƶ���Ƶ
			if (input->bytesReady() < readSize)
			{
				QThread::msleep(1);
				continue;
			}
			
			int size = 0;
			while (size != readSize)
			{
				int len = io->read(buf + size, readSize - size);
				if (len < 0)
				{
					break;
				}
				size += len;
			}
			if (size != readSize)
			{
				continue;
			}
			// �Ѿ���ȡһ֡Դ����
			Push(XData(buf, readSize, GetCurTime()));
		}
		delete[] buf;
		cout << "�˳���Ƶ¼���߳�" << endl;
	}


	bool Init() override
	{
		Stop();

		/// 1. qt��Ƶ��ʼ¼��
		QAudioFormat fmt;
		fmt.setSampleRate(sampleRate);
		fmt.setChannelCount(channels);
		fmt.setSampleSize(sampleByte * 8);
		fmt.setCodec("audio/pcm");
		fmt.setByteOrder(QAudioFormat::LittleEndian);
		fmt.setSampleType(QAudioFormat::UnSignedInt);
		QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();
		if (!info.isFormatSupported(fmt))
		{
			cout << "Audio format not support!" << endl;
			fmt = info.nearestFormat(fmt);
		}
		input = new QAudioInput(fmt);

		//��ʼ¼����Ƶ
		io = input->start();
		if (!io)
		{
			return false;
		}
		return true;
	}

	void Stop() override
	{
		XDataThread::Stop();

		if (input)
		{
			input->stop();
			input = nullptr;
		}
		if (io)
		{
			io->close();
			io = nullptr;
		}
	}

private:
	QAudioInput *input = nullptr;
	QIODevice *io = nullptr;
};



XAudioRecord::XAudioRecord()
{
}


XAudioRecord::~XAudioRecord()
{
}


XAudioRecord *XAudioRecord::Get(XAUDIOTYPE type, unsigned char index)
{
	static CXAudioRecord record[255];
	return &record[index];
}


