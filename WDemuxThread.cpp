#include "WDemuxThread.h"
#include "WDemux.h"
#include "WAudioThread.h"
#include "WVideoThread.h"
#include <QElapsedTimer>

WDemuxThread::WDemuxThread(QObject *parent /*= Q_NULLPTR*/)
	: QThread(parent)
{
	if (!m_demux)
		m_demux = new WDemux();
}

WDemuxThread::~WDemuxThread()
{

}

bool WDemuxThread::open(const char *url, VideoDataFunc func, VideoInfoFunc infoFunc, TimeFunc timeFunc)
{
	if (!m_demux)
		return false;

	m_mutex.lock();

	if (!m_audioThread)
		m_audioThread = new WAudioThread();

	if (!m_videoThread)
		m_videoThread = new WVideoThread();

	//打开解封装
	bool ret = m_demux->open(url, infoFunc);
	if (!ret)
	{
		cout << "demux->Open(url) failed!" << endl;
		return false;
	}

	//打开视频解码器和处理线程
	if (!m_videoThread->open(m_demux->videoPara(), func,timeFunc))
	{
		ret = false;
		cout << "m_videoThread->Open failed!" << endl;
	}
	else
	{
		if (m_videoThread && !m_videoThread->isRunning())
		{
			m_videoThread->m_max_frame_duration = m_demux->m_max_frame_duration;
			m_videoThread->start();
		}
			
	}

	//打开音频解码器和处理线程
	if (!m_audioThread->open(m_demux->audioPara()))
	{
		ret = false;
		cout << "m_audioThread->Open failed!" << endl;
	}
	else
	{
		if (m_audioThread && !m_audioThread->isRunning())
			m_audioThread->start();
	}

	m_isPause = false;
	m_mutex.unlock();
	return true;
}

void WDemuxThread::close()
{
	m_isExit = true;
	m_mutex.lock();
    if (m_audioThread)
        m_audioThread->close();

	if (m_videoThread)
		m_videoThread->close();

	delete m_audioThread;
	m_audioThread = NULL;

	delete m_videoThread;
	m_videoThread = NULL;
	m_mutex.unlock();

    wait();
}

void WDemuxThread::setPause(bool isPause)
{
	m_isPause = isPause;

	if (m_audioThread)
		m_audioThread->setPause(isPause);
	if (m_videoThread) 
		m_videoThread->setPause(isPause);
}

void WDemuxThread::seek(double pos)
{
	//清理缓存
	clear();

	bool status = this->m_isPause;

	//暂停
	setPause(true);

	if (m_demux)
		m_demux->seek(pos);

	//实际要显示的位置pts
	double seekPts = pos * m_demux->m_totalTime;
	seekPts *= 1000;	//ms

	while (!m_isExit)
	{
		AVPacket *pkt = m_demux->readVideo();
		if (!pkt)
			break;
		//如果解码到seekPts
		if (m_videoThread->repaintPts(pkt))
		{
			this->m_pts = seekPts;
			break;
		}
	}

	//seek是非暂停状态
	if (!status)
		setPause(false);
}

void WDemuxThread::clear()
{
	m_mutex.lock();
	if (m_demux)
		m_demux->clear();
	if (m_videoThread)
		m_videoThread->clear();
	if (m_audioThread)
		m_audioThread->clear();
	m_mutex.unlock();
}

void WDemuxThread::run()
{
	qDebug() << "*****WDemuxThread run";
	m_isExit = false;
	while (!m_isExit)
	{
		m_mutex.lock();
		if (m_isPause || !m_demux)
		{
			m_mutex.unlock();
			msleep(5);
			continue;
		}

		//视频同步音频
		if (m_audioThread && m_videoThread)
		{
			m_pts = m_audioThread->m_pts;
			m_videoThread->setSynPts(m_audioThread->m_pts);
		}

		AVPacket *pkt = m_demux->read();
		if (!pkt)
		{
			m_mutex.unlock();
			msleep(5);
			continue;
		}

		//判断数据是音频
		if (m_demux->isAudio(pkt))
		{
			if (m_audioThread)
				m_audioThread->push(pkt);
		}
		else //视频
		{
			if (m_videoThread)
				m_videoThread->push(pkt);
		}
		m_mutex.unlock();
	}

	qDebug() << "*****WDemuxThread stop";
}
