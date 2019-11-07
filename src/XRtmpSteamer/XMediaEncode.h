#pragma once
#include "XData.h"


struct SwsContext;
struct AVCodecContext;

enum XSampleFMT
{
	X_S16	= 1,
	X_FLTP	= 8
};

/// ����Ƶ����ӿ��ࣨ���麯����
class XMediaEncode
{
public:
	virtual ~XMediaEncode();
	// ������������
	static XMediaEncode *Get(unsigned char index = 0);

	// ���ظ�ʽת���������ĳ�ʼ��
	virtual bool InitScale() = 0;
	// ��Ƶ��������ʼ��
	virtual bool InitVideoCodec() = 0;
	// ��Ƶ��������ʼ��
	virtual bool InitAudioCodec() = 0;
	// ��Ƶ�ز��������ĳ�ʼ��
	virtual bool InitResample() = 0;
	// ��Ƶ���룬����ֵ�������������
	virtual XData EncodeVideo(XData frame) = 0;
	// ��Ƶ���룬����ֵ�������������
	virtual XData EncodeAudio(XData frame) = 0;
	// rgbת����yuv
	virtual XData RGBToYUV(XData d) = 0;
	// ��Ƶ�ز���
	virtual XData Resample(XData d) = 0;
	virtual void Close() = 0;

public:
	/// �������
	int inWidth			= 1280;
	int inHeight		= 720;
	int inPixSize		= 3;
	int channels		= 2;
	int sampleRate		= 44100;
	int inSampleFmt		= X_S16;

	/// �������
	int outWidth		= 1280;
	int outHeight		= 720;
	int outBitRate		= 50 * 1024 * 8;	// ѹ���ʣ�ѹ����ÿ����Ƶ��bitλ��С 50KB
	int fps				= 25;
	int nbSamples		= 1024;
	int outSampleFmt	= X_FLTP;

	AVCodecContext *vc = nullptr;	// ��Ƶ������������
	AVCodecContext *ac = nullptr;	// ��Ƶ������������

protected:
	XMediaEncode();
};

