#ifndef _WDECODE_H_
#define _WDECODE_H_
#include "global.h"

class WDecode
{
public:
	WDecode();
	virtual ~WDecode();
public:
	//�򿪽�����
	virtual bool open(AVCodecParameters *para);

	//�ر�
	virtual void close();

	//���͵������̣߳����ܳɹ�����ͷ�pkt�ռ䣨�����ý�����ݣ�
	virtual bool send(AVPacket *pkt);

	//��ȡ��������,һ��send������Ҫ���Recv����ȡ�����е�����Send NULL��Recv���
	virtual AVFrame* recv();

	//�����Դ
	virtual void clear();

public:
	AVCodecContext *m_pCodecCtx = 0;

	//��ǰ���뵽��pts
	long long m_pts = 0;

	std::mutex m_mux;
};

#endif // 

