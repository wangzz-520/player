#include "WAudioThread.h"
#include "WDecode.h"
#include "WAudioPlay.h"
#include <QElapsedTimer>
#include <QDebug>
#include <qmath.h>


WAudioThread::WAudioThread(QObject *parent /*= Q_NULLPTR*/)
	: WDecodeThread(parent)
{
	if (!m_play)
		m_play = WAudioPlay::getInstance();
}

WAudioThread::~WAudioThread()
{
}

bool WAudioThread::open(AVCodecParameters *para)
{
	if (!para)
		return false;

	clear();

	m_audioMutex.lock();

	int ret = true;

	if (!m_play->open())
	{
		cout << "WAudioPlay open failed!" << endl;
		ret = false;
	}

	//�򿪽�����
	if (!m_decode)
		m_decode = new WDecode();

	if (!m_decode->open(para))
	{
		cout << "audio decode open failed!" << endl;
		ret = false;
	}

	if (m_swrContext)
	{
		swr_close(m_swrContext);
		swr_free(&m_swrContext);
	}

	// �����ز���������
	m_swrContext = swr_alloc();
	if (!m_swrContext)
	{
		std::cerr << "Failed to allocate resampler context." << std::endl;
	}

	// �����ز�������
	m_swrContext = swr_alloc_set_opts(NULL,												 //ctx
		AV_CH_LAYOUT_STEREO,																	//���channel����
		AV_SAMPLE_FMT_S16,																		 //����Ĳ�����ʽ
		44100,																									//������
		av_get_default_channel_layout(m_decode->m_pCodecCtx->channels),	//����channel����
		m_decode->m_pCodecCtx->sample_fmt,														//����Ĳ�����ʽ
		m_decode->m_pCodecCtx->sample_rate,														//����Ĳ�����
		0, NULL);

	// ��ʼ���ز���������
	if (swr_init(m_swrContext) < 0)
	{
		std::cerr << "Failed to initialize resampler context." << std::endl;
		swr_free(&m_swrContext);
	}

	m_pts = 0;

	qDebug("WAudioThread open synpts = %lf", m_decode->m_pts);

	m_audioMutex.unlock();

	return ret;
}

void WAudioThread::close()
{
	WDecodeThread::close();
	if (m_swrContext)
	{
		m_audioMutex.lock();
		swr_close(m_swrContext);
		swr_free(&m_swrContext);
		m_swrContext = NULL;

		if (m_play)
			m_play->close();

		m_audioMutex.unlock();
	}
}

void WAudioThread::setPause(bool isPause)
{
	m_isPause = isPause;
	if (m_play)
		m_play->setPause(isPause);
}

void WAudioThread::clear()
{
	WDecodeThread::clear();
	m_audioMutex.lock();
	if (m_play)
		m_play->clear();
	m_audioMutex.unlock();
}

void WAudioThread::run()
{
	qDebug() << "*****WAudioThread run";
	// ���������Ƶ����
	uint8_t **out_data = NULL;

	while (!m_isExit)
	{
		m_audioMutex.lock();
		if (m_isPause || !m_decode)
		{
			m_audioMutex.unlock();
			msleep(5);
			continue;
		}

		AVPacket *pkt = pop();
		bool ret = m_decode->send(pkt);
		if (!ret)
		{
			m_audioMutex.unlock();
			msleep(1);
			continue;
		}
		//һ��send ���recv
		while (!m_isExit)
		{
			AVFrame * frame = m_decode->recv();
			if (!frame)
				break;

			m_pts = m_decode->m_pts - m_play->getNoPlayMs();
			
			//�����������
			int in_nb_samples = frame->nb_samples;//1024

			//�����������
			int out_linesize;
			int dst_nb_samples = av_rescale_rnd(in_nb_samples, 44100, frame->sample_rate, AV_ROUND_UP);

			av_samples_alloc_array_and_samples(&out_data, &out_linesize,2, dst_nb_samples, AV_SAMPLE_FMT_S16, 0);

			//����ÿ��ͨ�������������������ʱΪ��ֵ
			int sampleCount = swr_convert(m_swrContext, out_data, dst_nb_samples,
				(const uint8_t**)frame->data, in_nb_samples);

			if (sampleCount <= 0)
			{
				std::cerr << "Error while resampling." << std::endl;
				break;
			}
			
			int outSize = sampleCount * 2 * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
			//������Ƶ
			while (!m_isExit)
			{
				if (outSize <= 0)
					break;

				int freeByte = m_play->getFree();
				if(freeByte < 0)
					break;

				//������ʣ���С���ռ䲻��
				if (freeByte < outSize)
				{
					msleep(1);
					continue;
				}
				m_play->write(out_data[0], outSize);
				break;
			}

			av_frame_free(&frame);
			av_free(out_data[0]);
			break;
		}
		m_audioMutex.unlock();
		msleep(5);
	}

	qDebug() << "*****WAudioThread stop";
}