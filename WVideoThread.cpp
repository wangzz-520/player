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

	//清理缓冲队列
	clear();

	m_videoMutex.lock();
	m_synpts = 0;
	m_curPts = 0;
	m_prePts = 0;
	m_delay = 0;
	int ret = true;
	//打开解码器
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
		return true; //表示结束解码
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
		// 拷贝YUV420P数据到缓冲区
		memcpy(buffer, frame->data[0], ySize); // 拷贝Y分量
		memcpy(buffer + ySize, frame->data[1], uvSize); // 拷贝U分量
		memcpy(buffer + ySize + uvSize, frame->data[2], uvSize); // 拷贝V分量

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

		//获取当前视频和音频的差值
		double diff = m_curPts - m_synpts/1000;// diff < 0 :video slow,diff > 0 :video fast

		//0.04~0.1s
		double sync_threshold = FFMAX(MIN_SYNC_THRESHOLD, FFMIN(MAX_SYNC_THRESHOLD, delay));

		//帧的最大持续时间 - 超过这个值，我们认为跳跃是时间戳的不连续性
		if (!isnan(diff) && fabs(diff) < m_max_frame_duration)
		{
			if (diff <= -sync_threshold)//视频比音频慢，加快
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

					//差距较大，需要视频丢帧追上
					AVPacket *pkt = pop();
					if (pkt)
					{
						av_packet_free(&pkt);	
					}
					m_videoMutex.unlock();
					continue;
				}
			}	
			//视频比音频快，减慢
			else if (diff >= sync_threshold && delay > SYNC_FRAMEDUP_THRESHOLD)
				delay = delay + diff;//音视频差距较大，且一帧的超过帧最常时间，一步到位
			else if (diff >= sync_threshold)
				delay = 2 * delay;//音视频差距较小，加倍延迟，逐渐缩小
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
		
		//一次send 多次recv
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

				if (width == frame->linesize[0]) //无需对齐
				{
					// 拷贝YUV420P数据到缓冲区
					memcpy(buffer, frame->data[0], ySize); // 拷贝Y分量
					memcpy(buffer + ySize, frame->data[1], uvSize); // 拷贝U分量
					memcpy(buffer + ySize + uvSize, frame->data[2], uvSize); // 拷贝V分量
				}
				else//行对齐问题
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
				//发送信号，yuv数据
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
