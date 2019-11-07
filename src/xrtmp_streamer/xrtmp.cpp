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
		/// 5 ��װ������Ƶ������
		// a ���������װ��������
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

		// b �����Ƶ��
		AVStream *st = avformat_new_stream(ic, nullptr);
		if (!st)
		{
			cout << "avformat_new_stream failed!" << endl;
			return -1;
		}
		st->codecpar->codec_tag = 0;
		// �ӱ��������Ʋ���
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
		/// 6 ��rtmp���������IO
		int re = avio_open(&ic->pb, url.c_str(), AVIO_FLAG_WRITE);
		if (re < 0)
		{
			char buf[1024] = { 0 };
			av_strerror(re, buf, sizeof(buf) - 1);
			cout << buf <<endl;
			return false;
		}
		// д���װͷ
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

		// �ж�����Ƶ������Ƶ
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

		// ����
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
	AVFormatContext *ic = nullptr;		// rtmp flv ��װ��
	const AVCodecContext *vc = nullptr;	// ��Ƶ������
	const AVCodecContext *ac = nullptr;	// ��Ƶ������
	AVStream *vs = nullptr;				// ��Ƶ��
	AVStream *as = nullptr;				// ��Ƶ��
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
		// ע�����еķ�װ��
		av_register_all();

		// ��ʼ������Э��
		avformat_network_init();
		isFirst = false;
	}
	
	return &cxr[index];
}


