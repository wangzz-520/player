#include "WDecode.h"

WDecode::WDecode()
{

}

WDecode::~WDecode()
{

}

bool WDecode::open(AVCodecParameters *para)
{
	if (!para) 
		return false;

	//�ȹر�֮ǰ�Ľ�����
	close();

	///�ҵ�������
	AVCodec *vcodec = avcodec_find_decoder(para->codec_id);
	if (!vcodec)
	{
		avcodec_parameters_free(&para);
		cout << "can't find the codec id " << para->codec_id << endl;
		return false;
	}
	cout << "find the AVCodec " << para->codec_id << endl;

	m_mux.lock();

	m_pCodecCtx = avcodec_alloc_context3(vcodec);
	if (m_pCodecCtx == NULL)
	{
		m_mux.unlock();
		printf("Could not allocate AVCodecContext\n");
		return false;
	}

	///���ý����������Ĳ���
	avcodec_parameters_to_context(m_pCodecCtx, para);
	avcodec_parameters_free(&para);

	///�򿪽�����������
	int ret = avcodec_open2(m_pCodecCtx, 0, 0);
	if (ret != 0)
	{
		avcodec_free_context(&m_pCodecCtx);
		m_mux.unlock();
		char buf[1024] = { 0 };
		av_strerror(ret, buf, sizeof(buf) - 1);
		cout << "avcodec_open2  failed! :" << buf << endl;
		return false;
	}

	m_pCodecCtx->thread_count = 1; // �����̻߳�Ӱ���ڲ����漴�� codec_ctx_->delay = codec_ctx_->thread_count - 1���򿪽�����֮ǰ��codec_ctx_->thread_count����Ϊ1���ڲ��������л���

	m_mux.unlock();
	return true;
}

void WDecode::close()
{
	m_mux.lock();
	if (m_pCodecCtx)
	{
		avcodec_close(m_pCodecCtx);
		avcodec_free_context(&m_pCodecCtx);
		m_pCodecCtx = NULL;
	}

	m_pts = 0;

	m_mux.unlock();
}

bool WDecode::send(AVPacket *pkt)
{
	m_mux.lock();
	if (!pkt || pkt->size <= 0 || !pkt->data)
	{
		m_mux.unlock();
		return false;
	}
	
	if (!m_pCodecCtx)
	{
		m_mux.unlock();
		return false;
	}

	int ret = avcodec_send_packet(m_pCodecCtx, pkt);

	m_mux.unlock();
	av_packet_free(&pkt);
	
	if (ret != 0)
		return false;

	return true;
}

AVFrame* WDecode::recv()
{
	m_mux.lock();
	if (!m_pCodecCtx)
	{
		m_mux.unlock();
		return NULL;
	}

	AVFrame *frame = av_frame_alloc();
	int ret = avcodec_receive_frame(m_pCodecCtx, frame);
	m_mux.unlock();

	if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
	{
		av_frame_free(&frame);
		return NULL;
	}
	else if (ret < 0)
	{
		av_frame_free(&frame);
		return NULL;
	}

	m_pts = frame->pts;
	return frame;
}

void WDecode::clear()
{
	m_mux.lock();
	//������뻺��
	if (m_pCodecCtx)
		avcodec_flush_buffers(m_pCodecCtx);

	m_mux.unlock();
}
