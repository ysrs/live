#include "XRtmp.h"
#include <iostream>

extern "C"
{
#include <libavformat/avformat.h>
}

#pragma comment(lib, "avformat.lib")


using namespace std;

class CXRtmp : public XRtmp
{
public:
	void Close() override
	{
		if (ic)
		{
			avformat_close_input(&ic);
			vs = nullptr;
		}
		vc = nullptr;
		url = "";
	}

	bool Init(const char *url) override
	{
		/// 5 封装器和视频流配置
		// a 创建输出封装器上下文
		this->url = url;
		int re = avformat_alloc_output_context2(&ic, nullptr, "flv", url);
		if (re < 0)
		{
			char buf[1024] = { 0 };
			av_strerror(re, buf, sizeof(buf) - 1);
			cout << buf << endl;
			return false;
		}

		return true;
	}

	int AddStream(const AVCodecContext *c) override
	{
		if (!c)
		{
			return -1;
		}

		// b 添加视频流
		AVStream *st = avformat_new_stream(ic, nullptr);
		if (!st)
		{
			cout << "avformat_new_stream failed!" << endl;
			return -1;
		}
		st->codecpar->codec_tag = 0;
		// 从编码器复制参数
		avcodec_parameters_from_context(st->codecpar, c);
		av_dump_format(ic, 0, url.c_str(), 1);

		if (c->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			vc = c;
			vs = st;
		}
		else if (c->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			ac = c;
			as = st;
		}

		return st->index;
	}

	bool SendHead() override
	{
		/// 6 打开rtmp的网络输出IO
		int re = avio_open(&ic->pb, url.c_str(), AVIO_FLAG_WRITE);
		if (re < 0)
		{
			char buf[1024] = { 0 };
			av_strerror(re, buf, sizeof(buf) - 1);
			cout << buf <<endl;
			return false;
		}
		// 写入封装头
		re = avformat_write_header(ic, nullptr);
		if (re < 0)
		{
			char buf[1024] = { 0 };
			av_strerror(re, buf, sizeof(buf) - 1);
			cout << buf << endl;
			return false;
		}

		return true;
	}

	bool SendFrame(XData d, int streamIndex) override
	{
		if (!d.data || d.size <= 0)
		{
			return false;
		}
		AVPacket *pkt = (AVPacket *)d.data;
		pkt->stream_index = streamIndex;
		AVRational stime;
		AVRational dtime;

		// 判断是音频还是视频
		if (vs && vc && pkt->stream_index == vs->index)
		{
			stime = vc->time_base;
			dtime = vs->time_base;
		}
		else if (ac && as && pkt->stream_index == as->index)
		{
			stime = ac->time_base;
			dtime = as->time_base;
		}
		else
		{
			return false;
		}

		// 推流
		pkt->pts = av_rescale_q(pkt->pts, stime, dtime);
		pkt->dts = av_rescale_q(pkt->dts, stime, dtime);
		pkt->duration = av_rescale_q(pkt->duration, stime, dtime);
		int re = av_interleaved_write_frame(ic, pkt);
		if (re < 0)
		{
			return false;
		}

		return true;
	}

private:
	AVFormatContext *ic = nullptr;		// rtmp flv 封装器
	const AVCodecContext *vc = nullptr;	// 视频编码器
	const AVCodecContext *ac = nullptr;	// 音频编码器
	AVStream *vs = nullptr;				// 音频流
	AVStream *as = nullptr;				// 视频流
	string url = "";
};



XRtmp::XRtmp()
{
}


XRtmp::~XRtmp()
{
}


XRtmp *XRtmp::Get(unsigned char index)
{
	static CXRtmp cxr[255];

	static bool isFirst = true;
	if (isFirst)
	{
		// 注册所有的封装器
		av_register_all();

		// 初始化网络协议
		avformat_network_init();
		isFirst = false;
	}
	
	return &cxr[index];
}


