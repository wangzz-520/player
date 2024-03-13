#ifndef _WDECODETHREAD_H_
#define _WDECODETHREAD_H_

#include <QThread>
#include <QQueue>
#include <QMutex>
#include <queue>
#include "global.h"

class WDecode;
class WDecodeThread : public QThread
{
public:
	WDecodeThread(QObject *parent = Q_NULLPTR);
	virtual ~WDecodeThread();

public:
	virtual void push(AVPacket *pkt);

	virtual AVPacket *pop();

	//关闭解码器上下文，释放
	virtual void close();

	//清理队列
	virtual void clear();

protected:
	WDecode *m_decode = nullptr;
	bool m_isExit = false;
	std::queue <AVPacket *> m_queue;
	int m_maxList = 100;

private:
	QMutex m_mutex;
};

#endif // 

