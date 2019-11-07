#include "XVideoCapture.h"

#include <opencv2/highgui.hpp>
#include <iostream>

#pragma comment(lib, "opencv_world320.lib")


using namespace cv;
using namespace std;

class CXVideoCapture : public XVideoCapture
{
public:
	void run() override
	{
		//namedWindow("cam");

		cout << "进入视频录制线程" << endl;
		Mat frame;
		while (!isExit)
		{
			if (!cam.read(frame))
			{
				msleep(1);
				continue;
			}
			if (frame.empty())
			{
				msleep(1);
				continue;
			}
			// 确保数据是连续的
			//imshow("cam", frame);
			///waitKey(1);
			fmutex.lock();
			for (int i=0; i<filters.size(); ++i)
			{
				Mat des;
				filters[i]->Filter(&frame, &des);
				frame = des;
			}
			fmutex.unlock();

			Push(XData((char *)frame.data, frame.cols*frame.rows*frame.elemSize(), GetCurTime()));
		}
		cout << "退出视频录制线程" << endl;
	}


	bool Init(int camIndex) override
	{
		/// 1 使用OpenCV打开rtsp相机
		cam.open(camIndex);

		if (!cam.isOpened())
		{
			cout << "cam open failed!" << endl;
			return false;
		}
		cout << camIndex << " cam open success!" << endl;

		width = cam.get(CAP_PROP_FRAME_WIDTH);
		height = cam.get(CAP_PROP_FRAME_HEIGHT);
		fps = cam.get(CAP_PROP_FPS);
		if (fps == 0)
		{
			fps = 25;
		}
	
		return true;
	}

	bool Init(const char *url) override
	{
		/// 1 使用OpenCV打开rtsp相机
		cam.open(url);

		if (!cam.isOpened())
		{
			cout << "cam open failed!" << endl;
			return false;
		}
		cout << url << " cam open success!" << endl;

		width = cam.get(CAP_PROP_FRAME_WIDTH);
		height = cam.get(CAP_PROP_FRAME_HEIGHT);
		fps = cam.get(CAP_PROP_FPS);
		if (fps == 0)
		{
			fps = 25;
		}

		return true;
	}
	void Stop() override
	{
		XDataThread::Stop();

		if (cam.isOpened())
		{
			cam.release();
		}
	}

private:
	VideoCapture cam;
};



XVideoCapture::XVideoCapture()
{
}


XVideoCapture::~XVideoCapture()
{
}


XVideoCapture *XVideoCapture::Get(unsigned char index)
{
	static CXVideoCapture videos[255];
	return &videos[index];
}


