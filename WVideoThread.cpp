#include "WVideoThread.h"
#include "WDecode.h"
#include <QDebug>
#include <QElapsedTimer>

WVideoThread::WVideoThread(QObject *parent /*= Q_NULLPTR*/)
	: WDecodeThread(parent)
{

}

WVideoThread::~WVideoThread()
{
	m_func = 0;
	m_isExit = true;
	wait();
}

bool WVideoThread::open(AVCodecParameters *para, VideoDataFunc func, TimeFunc timeFunc)
{
	if (!para)
		return false;

	//���������
	clear();

	m_videoMutex.lock();
	m_synpts = 0;
	m_curPts = 0;
	m_prePts = 0;
	m_delay = 0;
	int ret = true;
	//�򿪽�����
	if (!m_decode)
		m_decode = new WDecode();

	if (!m_decode->open(para))
	{
		m_videoMutex.unlock();
		cout << "video decode open failed!" << endl;
		ret = false;

		return ret;
	}

	m_func = func;
	m_timeFunc = timeFunc;

	qDebug("WVideoThread open synpts = %lf,curPts = %lf,prePts = %lf , delay  = %lf",
		m_synpts, m_curPts, m_prePts, m_delay);
	m_videoMutex.unlock();

	return ret;
}

void WVideoThread::setSynPts(long long pts)
{
	m_synpts = pts;
}

void WVideoThread::setPause(bool isPause)
{
	m_isPause = isPause;
}

bool WVideoThread::repaintPts(AVPacket *pkt)
{
	m_videoMutex.lock();
	bool re = m_decode->send(pkt);
	if (!re)
	{
		m_videoMutex.unlock();
		return true; //��ʾ��������
	}
	AVFrame *frame = m_decode->recv();
	if (!frame)
	{
		m_videoMutex.unlock();
		return false;
	}
	
	if (m_func)
	{
		uint8_t* buffer = NULL;
		int width = frame->width;
		int height = frame->height;

		int ySize = frame->width * frame->height;
		int uvSize = ySize / 4;
		int totalSize = ySize + 2 * uvSize;
	
		buffer = new uint8_t[totalSize];
		memset(buffer, 0, sizeof(buffer));
		// ����YUV420P���ݵ�������
		memcpy(buffer, frame->data[0], ySize); // ����Y����
		memcpy(buffer + ySize, frame->data[1], uvSize); // ����U����
		memcpy(buffer + ySize + uvSize, frame->data[2], uvSize); // ����V����

		int second = frame->pts / 1000;
		//qDebug() << "second = " << second << "  frame->pts = " << frame->pts << "m_timeBase  = " << m_timeBase;

		m_timeFunc(second);
		m_func(buffer);

		delete[]buffer;
	}
		
	av_frame_free(&frame);
	m_videoMutex.unlock();

	return true;
}

void WVideoThread::run()
{
	qDebug() << "*****WVideoThread run";
	while (!m_isExit)
	{
		m_videoMutex.lock();
		if (!m_decode || m_isPause)
		{
			m_videoMutex.unlock();
			msleep(5);
			continue;
		}

		m_curPts = (double)m_decode->m_pts / 1000;	//s

		double delay = m_curPts - m_prePts; //s

		if (delay <= 0 || delay >= 1.0)
		{
			delay = m_delay;
		}

		//��ȡ��ǰ��Ƶ����Ƶ�Ĳ�ֵ
		double diff = m_curPts - m_synpts/1000;// diff < 0 :video slow,diff > 0 :video fast

		//0.04~0.1s
		double sync_threshold = FFMAX(MIN_SYNC_THRESHOLD, FFMIN(MAX_SYNC_THRESHOLD, delay));

		//֡��������ʱ�� - �������ֵ��������Ϊ��Ծ��ʱ����Ĳ�������
		if (!isnan(diff) && fabs(diff) < m_max_frame_duration)
		{
			if (diff <= -sync_threshold)//��Ƶ����Ƶ�����ӿ�
			{
				delay = FFMAX(0, delay + diff);
				static int last_delay_zero_counts = 0;
				if (m_delay <= 0)
				{
					last_delay_zero_counts++;
				}
				else
				{
					last_delay_zero_counts = 0;
				}
				if (diff < -1.0 && last_delay_zero_counts >= 10)
				{
					printf("maybe video codec too slow, adjust video\n");

					//���ϴ���Ҫ��Ƶ��֡׷��
					AVPacket *pkt = pop();
					if (pkt)
					{
						av_packet_free(&pkt);	
					}
					m_videoMutex.unlock();
					continue;
				}
			}	
			//��Ƶ����Ƶ�죬����
			else if (diff >= sync_threshold && delay > SYNC_FRAMEDUP_THRESHOLD)
				delay = delay + diff;//����Ƶ���ϴ���һ֡�ĳ���֡�ʱ�䣬һ����λ
			else if (diff >= sync_threshold)
				delay = 2 * delay;//����Ƶ����С���ӱ��ӳ٣�����С
		}

		AVPacket *pkt = pop();
		if (!pkt)
		{
			m_videoMutex.unlock();
			msleep(1);
			continue;
		}

		bool ret = m_decode->send(pkt);
		if (!ret)
		{
			m_videoMutex.unlock();
			msleep(1);
			continue;
		}

		double curr_time = static_cast<double>(av_gettime()) / 1000000.0;
		
		//һ��send ���recv
		while (!m_isExit)
		{
			AVFrame * frame = m_decode->recv();
			if (!frame)
				break;

			switch (frame->format)
			{
			case AV_PIX_FMT_YUV420P:
			{
				int width = frame->width;
				int height = frame->height;

				int ySize = frame->width * frame->height;
				int uvSize = ySize / 4;
				int totalSize = ySize + 2 * uvSize;

				uint8_t* buffer = new uint8_t[totalSize];
				memset(buffer, 0, sizeof(buffer));

				if (width == frame->linesize[0]) //�������
				{
					// ����YUV420P���ݵ�������
					memcpy(buffer, frame->data[0], ySize); // ����Y����
					memcpy(buffer + ySize, frame->data[1], uvSize); // ����U����
					memcpy(buffer + ySize + uvSize, frame->data[2], uvSize); // ����V����
				}
				else//�ж�������
				{
					int j = 0;
					for (int i = 0; i < height; i++)
					{
						memcpy(buffer + j, frame->data[0] + i * frame->linesize[0], width);
						j += width;
					}
					for (int i = 0; i < height / 2; i++)
					{
						memcpy(buffer + j, frame->data[1] + i * frame->linesize[1], width / 2);
						j += width / 2;
					}
					for (int i = 0; i < height / 2; i++)
					{
						memcpy(buffer + j, frame->data[2] + i * frame->linesize[2], width / 2);
						j += width / 2;
					}
				}

				int second = frame->pts / 1000;
				//qDebug() << "second = " << second << "  frame->pts = " << frame->pts << "m_timeBase  = " << m_timeBase;
				m_timeFunc(second);
				//�����źţ�yuv����
				m_func(buffer);

				delete[]buffer;
			}break;
			default:
			{
				return;
			}
			}
			av_frame_free(&frame);
		}

		m_delay = delay;
		m_prePts = m_curPts;

		double curr_time2 = static_cast<double>(av_gettime()) / 1000000.0;
		double timeDiff = curr_time2 - curr_time;

		double actual_delay = (delay * 1000000 - timeDiff) / 1000000.0;
		if (actual_delay <= MIN_REFRSH_S)
		{
			actual_delay = MIN_REFRSH_S;
		}

		m_videoMutex.unlock();

		usleep(static_cast<int>(actual_delay * 1000 * 1000));	
	}

	qDebug() << "*****WVideoThread stop";
}
