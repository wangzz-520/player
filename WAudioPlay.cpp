#include "WAudioPlay.h"
#include <QAudioFormat>
#include <QAudioOutput>
#include <mutex>

class CWAudioPlay :public WAudioPlay
{
public:
	QAudioOutput *output = NULL;
	QIODevice *io = NULL;
	std::mutex m_mux;

	virtual long long getNoPlayMs()
	{
		m_mux.lock();
		if (!output)
		{
			m_mux.unlock();
			return 0;
		}
		long long pts = 0;
		//��δ���ŵ��ֽ���
		double size = output->bufferSize() - output->bytesFree();
		//һ����Ƶ�ֽڴ�С
		double secSize = m_sampleRate * (m_sampleSize / 8) *m_channels;
		if (secSize <= 0)
		{
			pts = 0;
		}
		else
		{
			pts = (size/secSize) * 1000;
		}
		m_mux.unlock();
		return pts;
	}

	void clear()
	{
		m_mux.lock();
		if (io)
		{
			io->reset();
		}
		m_mux.unlock();
	}
	virtual void close()
	{
		m_mux.lock();
		if (io)
		{
			io->close();
			io = NULL;
		}
		if (output)
		{
			output->stop();
			delete output;
			output = NULL;
		}
		m_mux.unlock();
	}
	virtual bool open()
	{
		QAudioFormat fmt;
		fmt.setSampleRate(m_sampleRate);
		fmt.setSampleSize(m_sampleSize);
		fmt.setChannelCount(m_channels);
		fmt.setCodec("audio/pcm");
		fmt.setByteOrder(QAudioFormat::LittleEndian);
		fmt.setSampleType(QAudioFormat::UnSignedInt);
		m_mux.lock();
		output = new QAudioOutput(fmt);
		output->setBufferSize(32768);  // ���û�������СΪ32768�ֽ�
		io = output->start(); //��ʼ����
		m_mux.unlock();
		if(io)
			return true;
		return false;
	}
	void setPause(bool isPause)
	{
		m_mux.lock();
		if (!output)
		{
			m_mux.unlock();
			return;
		}
		if (isPause)
		{
			output->suspend();
		}
		else
		{
			output->resume();
		}
		m_mux.unlock();
	}
	virtual bool write(const unsigned char *data, int datasize)
	{
		if (!data || datasize <= 0)
			return false;
		m_mux.lock();
		if (!output || !io)
		{
			m_mux.unlock();
			return false;
		}
		int size = io->write((char *)data, datasize);
		m_mux.unlock();
		if (datasize != size)
			return false;
		return true;
	}

	virtual void setVolume(qreal volume)
	{
		m_mux.lock();
		if (!output)
		{
			m_mux.unlock();
			return;
		}
		output->setVolume(volume);
		m_mux.unlock();
	}
	//������Ƶ������ʣ���С
	virtual int getFree()
	{
		m_mux.lock();
		if (!output || !io)
		{
			m_mux.unlock();
			return -1;
		}
		int free = output->bytesFree();
		m_mux.unlock();
		return free;
	}
};
WAudioPlay *WAudioPlay::getInstance()
{
	static CWAudioPlay play;
	return &play;
}

WAudioPlay::WAudioPlay()
{
}


WAudioPlay::~WAudioPlay()
{
}
