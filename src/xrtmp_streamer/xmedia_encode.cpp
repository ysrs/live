#include "xmedia_encode.h"
#include <iostream>

extern "C"
{
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}

#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "swresample.lib")

#if defined WIN32 || defined _WIN32
#include <Windows.h>
#endif


using namespace std;

// 获取CPU数量
static int XGetCPUNum()
{
#if defined WIN32 || defined _WIN32
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	return (int)systemInfo.dwNumberOfProcessors;
#elif defined __linux__
	return (int)sysconf(_SC_NPROCESSORS_ONLN);
#elif defined __APPLE__
	int numCPU = 0;
	int mib[4];
	size_t len = sizeof(numCPU);

	// set the mib for hw.ncpu
	mib[0] = CTL_HW;
	mib[1] = HW_AVAILCPU;	// alternatively, try HW_NCPU;
	// get the number of CPUs from the system
	sysctl(mib, 2, &numCPU, &len, NULL, 0);

	if (numCPU < 1)
	{
		mib[1] = HW_NCPU;
		sysctl(mib, 2, &numCPU, &len, NULL, 0);

		if (numCPU < 1)
		{
			numCPU = 1;
		}
	}

	return (int)numCPU;
#else
	return 1;
#endif
}

class CXMediaEncode : public XMediaEncode
{
public:
	void Close() override
	{
		if (vsc)
		{
			sws_freeContext(vsc);
			vsc = nullptr;
		}
		if (asc)
		{
			swr_free(&asc);
		}
		if (yuv)
		{
			av_frame_free(&yuv);
		}
		if (pcm)
		{
			av_frame_free(&pcm);
		}
		if (vc)
		{
			avcodec_free_context(&vc);
		}
		vpts = 0;
		apts = 0;
		av_packet_unref(&vpacket);
		av_packet_unref(&apacket);
	}

	bool InitScale() override
	{
		/// 2 初始化格式转换上下文
		vsc = sws_getCachedContext(vsc,
			inWidth, inHeight, AV_PIX_FMT_BGR24,		// 源宽、高
			outWidth, outHeight, AV_PIX_FMT_YUV420P,	// 目标宽、高、像素格式
			SWS_BICUBIC,								// 尺寸变化使用算法
			nullptr, nullptr, nullptr
		);
		if (!vsc)
		{
			cout << "sws_getCachedContext failed!" << endl;
			return false;
		}

		/// 3 初始化输出的数据结构
		yuv = av_frame_alloc();
		yuv->format = AV_PIX_FMT_YUV420P;
		yuv->width = inWidth;
		yuv->height = inHeight;
		yuv->pts = 0;
		// 分配yuv空间
		int re = av_frame_get_buffer(yuv, 32);
		if (re < 0)
		{
			char buf[1024] = { 0 };
			av_strerror(re, buf, sizeof(buf) - 1);
			throw exception(buf);
		}

		return true;
	}

	bool InitVideoCodec() override
	{
		/// 4 初始化编码上下文
		vc = CreateCodec(AV_CODEC_ID_H264);
		if (!vc)
		{
			return false;
		}

		vc->bit_rate = outBitRate;	// 压缩率，压缩后每秒视频的bit位大小 200KB
		vc->width = outWidth;
		vc->height = outHeight;
		//vc->time_base = { 1, outFps };
		vc->framerate = { fps, 1 };
		// 画面组的大小，即多少帧一个关键帧，具体值要在现实效果和压缩大小之间做一个权衡
		vc->gop_size = 50;
		vc->max_b_frames = 0;	// 没有b帧，b帧设置为0的好处：pts和dts就一致了
		vc->pix_fmt = AV_PIX_FMT_YUV420P;

		return OpenCodec(&vc);
	}

	bool InitAudioCodec() override
	{
		/// 4 初始化音频编码器
		ac = CreateCodec(AV_CODEC_ID_AAC);
		if (!ac)
		{
			return false;
		}

		ac->bit_rate = 40000;
		ac->sample_rate = sampleRate;
		ac->sample_fmt = AV_SAMPLE_FMT_FLTP;
		ac->channels = channels;
		ac->channel_layout = av_get_default_channel_layout(channels);

		return OpenCodec(&ac);
	}

	bool InitResample() override
	{
		/// 2. 音频重采样 上下文初始化
		asc = nullptr;
		asc = swr_alloc_set_opts(asc,
			av_get_default_channel_layout(channels), (AVSampleFormat)outSampleFmt, sampleRate,	// 输出格式
			av_get_default_channel_layout(channels), (AVSampleFormat)inSampleFmt, sampleRate,		// 输入格式
			0, nullptr
		);
		if (!asc)
		{
			cout << "swr_alloc_set_opts failed!" << endl;
			return false;
		}
		cout << "swr_alloc_set_opts success!" << endl;

		int re = swr_init(asc);
		if (re != 0)
		{
			char buf[1024] = { 0 };
			av_strerror(re, buf, sizeof(buf) - 1);
			cout << buf << endl;
			return false;
		}
		cout << "swr_init success!" << endl;

		/// 3 音频重采样输出空间分配
		pcm = av_frame_alloc();
		pcm->format = outSampleFmt;
		pcm->channels = channels;
		pcm->channel_layout = av_get_default_channel_layout(channels);
		pcm->nb_samples = nbSamples;			// 一帧音频一通道的采样数量
		re = av_frame_get_buffer(pcm, 0);	// 给pcm分配存储空间
		if (re < 0)
		{
			char buf[1024] = { 0 };
			av_strerror(re, buf, sizeof(buf) - 1);
			cout << buf << endl;
			return false;
		}
		cout << "av_frame_get_buffer success!" << endl;

		return true;
	}

	XData EncodeVideo(XData frame) override
	{
		av_packet_unref(&vpacket);
		XData r;
		if (frame.size <= 0 || !frame.data)
		{
			return r;
		}

		AVFrame *p = (AVFrame *)frame.data;
		int re = avcodec_send_frame(vc, p);
		if (re < 0)
		{
			return r;
		}

		re = avcodec_receive_packet(vc, &vpacket);
		if (re < 0 || vpacket.size <= 0)
		{
			return r;
		}

		r.data = (char *)&vpacket;
		r.size = vpacket.size;
		r.pts = frame.pts;

		return r;
	}

	XData EncodeAudio(XData frame) override
	{
		XData r;
		if (frame.size <= 0 || !frame.data)
		{
			return r;
		}
		AVFrame *p = (AVFrame *)frame.data;
		if (lasta == p->pts)
		{
			p->pts += 1000;
		}
		lasta = p->pts;

		int re = avcodec_send_frame(ac, p);
		if (re < 0)
		{
			return r;
		}
		av_packet_unref(&apacket);
		re = avcodec_receive_packet(ac, &apacket);
		if (re < 0)
		{
			return r;
		}

		r.data = (char *)&apacket;
		r.size = apacket.size;
		r.pts = frame.pts;

		return r;
	}

	XData RGBToYUV(XData d) override
	{
		XData r;
		r.pts = d.pts;

		/// rgb to yuv
		// 输入的数据结构
		uint8_t *indata[AV_NUM_DATA_POINTERS] = { nullptr };
		// bgrbgrbgrbgr
		// plane indata[0] bbbb    indata[1] gggg    indata[2] rrrr
		indata[0] = (uint8_t *)d.data;
		int insize[AV_NUM_DATA_POINTERS] = { 0 };
		// 一行（宽）数据的字节数
		insize[0] = inWidth * inPixSize;

		int h = sws_scale(vsc, indata, insize, 0, inHeight,	// 源数据
			yuv->data, yuv->linesize);
		if (h <= 0)
		{
			return r;
		}
		yuv->pts = d.pts;
		r.data = (char *)yuv;
		int *p = yuv->linesize;
		while (*p)
		{
			r.size += (*p)*outHeight;
			++p;
		}

		return r;
	}

	XData Resample(XData d) override
	{
		XData r;
		const uint8_t *indata[AV_NUM_DATA_POINTERS] = { nullptr };
		indata[0] = (uint8_t *)d.data;
		int len = swr_convert(asc,
			pcm->data, pcm->nb_samples,	//输出参数，输出存储地址和样本数量
			indata, pcm->nb_samples);
		if (len <= 0)
		{
			return r;
		}
		pcm->pts = d.pts;
		r.data = (char *)pcm;
		r.size = pcm->nb_samples*pcm->channels * 2;
		r.pts = d.pts;
		return r;
	}

private:
	static AVCodecContext *CreateCodec(AVCodecID cid)
	{
		/// 4 初始化编码器
		AVCodec *codec = avcodec_find_encoder(cid);
		if (!codec)
		{
			cout << "avcodec_find_encoder failed!" << endl;
			return nullptr;
		}
		// 音频编码器上下文
		AVCodecContext *c = avcodec_alloc_context3(codec);
		if (!c)
		{
			cout << "avcodec_alloc_context3 failed!" << endl;
			return nullptr;
		}
		// c 配置编码器参数
		//c->flags |= AV_CODEC_FLAG2_LOCAL_HEADER;	// 全局参数
		c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;	// 全局参数
		c->thread_count = XGetCPUNum();
		c->time_base = { 1, 1000000 };

		return c;
	}

	static bool OpenCodec(AVCodecContext **c)
	{
		// 打开编码器
		int re = avcodec_open2(*c, nullptr, nullptr);
		if (re < 0)
		{
			char buf[1024] = { 0 };
			av_strerror(re, buf, sizeof(buf) - 1);
			cout << buf << endl;
			avcodec_free_context(c);
			return false;
		}
		cout << "avcodec_open2 success!" << endl;
		
		return true;
	}

private:
	SwsContext *vsc = nullptr;			// 像素格式转换上下文
	SwrContext *asc = nullptr;			// 音频重采样上下文
	AVFrame *yuv = nullptr;				// 输出的YUV
	AVFrame *pcm = nullptr;				// 输出的pcm
	AVPacket vpacket = { nullptr };		// 视频帧
	AVPacket apacket = { nullptr };		// 音频帧
	int vpts = 0;
	int apts = 0;
	long long lasta = -1;
};


XMediaEncode::XMediaEncode()
{
}


XMediaEncode::~XMediaEncode()
{
}


XMediaEncode *XMediaEncode::Get(unsigned char index)
{
	static bool isFirst = true;
	if (isFirst)
	{
		// 注册所有的编解码器
		avcodec_register_all();
		isFirst = false;
	}
	static CXMediaEncode cxm[255];
	return &cxm[index];
}


