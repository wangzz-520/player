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

	//�رս����������ģ��ͷ�
	virtual void close();

	//�������
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

