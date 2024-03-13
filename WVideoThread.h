#ifndef _WVIDEOTHREAD_H_
#define _WVIDEOTHREAD_H_

#include "WDecodeThread.h"
#include "global.h"

class WDecode;

class WVideoThread : public WDecodeThread
{
public:
	WVideoThread(QObject *parent = Q_NULLPTR);
	virtual ~WVideoThread();

public:
	virtual bool open(AVCodecParameters *para, VideoDataFunc func,TimeFunc timeFunc);

	virtual void setSynPts(long long pts);
	
	void setPause(bool isPause);

	//����pts��������յ��Ľ�������pts >= seekpts return true ������ʾ����
	virtual bool repaintPts(AVPacket *pkt);

public:
	double m_max_frame_duration = 0;

protected:
	void run();

private:
	QMutex m_videoMutex;
	bool m_isPause = false;

	VideoDataFunc m_func;

	TimeFunc m_timeFunc;

	double m_synpts = 0;
	
	double m_curPts = 0;
	double m_prePts = 0;

	double m_delay = 0;
};

#endif // !_WVIDEOTHREAD_H_

