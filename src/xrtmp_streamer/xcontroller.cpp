#include "xcontroller.h"
#include "xvideo_capture.h"
#include "xaudio_record.h"
#include "xmedia_encode.h"
#include "xrtmp.h"
#include <iostream>


using namespace std;
XController::XController()
{
}


XController::~XController()
{
}


XController *XController::Get()
{
	static XController xc;
	return &xc;
}

bool XController::Start()
{
	///1 ����ĥƤ������
	XVideoCapture::Get()->ADDFilter(XFilter::Get());
	cout << "1 ����ĥƤ������" << endl;

	///2 �����
	if (camIndex >= 0)
	{
		if (!XVideoCapture::Get()->Init(camIndex))
		{
			err = "2 ������Ϊ ";
			err += camIndex;
			err += " ��ϵͳ���ʧ��";
			cout << err << endl;
			return false;
		}
	}
	else if (!inUrl.empty())
	{
		if (!XVideoCapture::Get()->Init(inUrl.c_str()))
		{
			err = "2 �� ";
			err += inUrl;
			err += " ���������ʧ��";
			cout << err << endl;
			return false;
		}
	}
	else
	{
		err = "2 �������������";
		cout << err << endl;
		return false;
	}
	cout << "2 ����򿪳ɹ���" << endl;

	///3 qt����Ƶ��ʼ¼��
	if (!XAudioRecord::Get()->Init())
	{
		err = "3 ¼���豸��ʧ��";
		cout << err << endl;
		return false;
	}
	cout << "3 ¼���豸�򿪳ɹ���" << endl;

	///��������Ƶ¼���߳�
	XAudioRecord::Get()->Start();
	XVideoCapture::Get()->Start();

	///4 ����Ƶ������
	///��ʼ����ʽת��������
	///��ʼ����ʽ��������ݽṹ
	XMediaEncode::Get()->inWidth = XVideoCapture::Get()->width;
	XMediaEncode::Get()->inHeight = XVideoCapture::Get()->height;
	XMediaEncode::Get()->outWidth = XVideoCapture::Get()->width;
	XMediaEncode::Get()->outHeight = XVideoCapture::Get()->height;
	if (!XMediaEncode::Get()->InitScale())
	{
		err = "4 ��Ƶ���ظ�ʽת��ʧ�ܣ�";
		cout << err << endl;
		return false;
	}
	cout << "4 ��Ƶ���ظ�ʽת���ɹ���" << endl;

	///5 ��Ƶ�ز��������ĳ�ʼ��
	XMediaEncode::Get()->channels = XAudioRecord::Get()->channels;
	XMediaEncode::Get()->nbSamples = XAudioRecord::Get()->nbSamples;
	XMediaEncode::Get()->sampleRate = XAudioRecord::Get()->sampleRate;
	if (!XMediaEncode::Get()->InitResample())
	{
		err = "5 ��Ƶ�ز��������ĳ�ʼ��ʧ�ܣ�";
		cout << err << endl;
		return false;
	}
	cout << "5 ��Ƶ�ز��������ĳ�ʼ���ɹ���" << endl;

	///6 ��ʼ����Ƶ������
	if (!XMediaEncode::Get()->InitAudioCodec())
	{
		err = "6 ��Ƶ��������ʼ��ʧ�ܣ�";
		cout << err << endl;
		return false;
	}
	cout << "6 ��Ƶ��������ʼ���ɹ���" << endl;

	///7 ��ʼ����Ƶ������
	if (!XMediaEncode::Get()->InitVideoCodec())
	{
		err = "7 ��Ƶ��������ʼ��ʧ�ܣ�";
		cout << err << endl;
		return false;
	}
	cout << "7 ��Ƶ��������ʼ���ɹ���" << endl;

	///8 ���������װ��������
	if (!XRtmp::Get()->Init(outUrl.c_str()))
	{
		err = "8 ���������װ��������ʧ�ܣ�";
		cout << err << endl;
		return false;
	}
	cout << "8 ���������װ�������ĳɹ���" << endl;

	///9 �������Ƶ��
	vIndex = XRtmp::Get()->AddStream(XMediaEncode::Get()->vc);
	aIndex = XRtmp::Get()->AddStream(XMediaEncode::Get()->ac);
	if (vIndex < 0 || aIndex < 0)
	{
		err = "9 �������Ƶ��ʧ�ܣ�";
		cout << err << endl;
		return false;
	}
	cout << "9 �������Ƶ���ɹ���" << endl;

	///10 д���װͷ
	if (!XRtmp::Get()->SendHead())
	{
		err = "10 ���ͷ�װͷʧ�ܣ�";
		cout << err << endl;
		return false;
	}
	cout << "10 ���ͷ�װͷ�ɹ���" << endl;

	XAudioRecord::Get()->Clear();
	XVideoCapture::Get()->Clear();
	XDataThread::Start();
	
	return true;
}

void XController::Stop()
{
	XDataThread::Stop();
	XAudioRecord::Get()->Stop();
	XVideoCapture::Get()->Stop();
	XMediaEncode::Get()->Close();
	XRtmp::Get()->Close();

	camIndex = -1;
	inUrl = "";
}

bool XController::Set(std::string key, double val)
{
	return XFilter::Get()->Set(key, val);
}

void XController::run()
{
	long long beginTime = GetCurTime();

	while (!isExit)
	{
		// һ�ζ�ȡһ֡��Ƶ
		XData ad = XAudioRecord::Get()->Pop();
		XData vd = XVideoCapture::Get()->Pop();

		if (ad.size <= 0 && vd.size <= 0)
		{
			msleep(1);
			continue;
		}
		// ������Ƶ
		if (ad.size > 0)
		{
			ad.pts = ad.pts - beginTime;
			// �ز���Դ����
			XData pcm = XMediaEncode::Get()->Resample(ad);
			ad.Drop();

			XData pkt = XMediaEncode::Get()->EncodeAudio(pcm);
			if (pkt.size > 0)
			{
				// ����
				if (XRtmp::Get()->SendFrame(pkt, aIndex))
				{
					cout << "#" << flush;
				}
			}
		}

		// ������Ƶ
		if (vd.size > 0)
		{
			vd.pts = vd.pts - beginTime;

			XData yuv = XMediaEncode::Get()->RGBToYUV(vd);
			vd.Drop();

			XData pkt = XMediaEncode::Get()->EncodeVideo(yuv);
			if (pkt.size > 0)
			{
				// ����
				if (XRtmp::Get()->SendFrame(pkt, vIndex))
				{
					cout << "@" << flush;
				}
			}
		}
	}
}
