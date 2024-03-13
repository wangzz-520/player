#ifndef _WAUDIOPLAY_H_
#define _WAUDIOPLAY_H_

class WAudioDevice;

class WAudioPlay
{
public:
	static WAudioPlay *getInstance();
	virtual ~WAudioPlay();

	WAudioPlay();

public:
	//����Ƶ����
	virtual bool open() = 0;
	virtual void close() = 0;
	virtual void clear() = 0;
	virtual void setVolume(double pos) = 0;

	//���ػ����л�û�в��ŵ�ʱ�䣨���룩
	virtual long long getNoPlayMs() = 0;

	//������Ƶ
	virtual bool write(const unsigned char *data, int datasize) = 0;


	virtual int getFree() = 0;
	virtual void setPause(bool isPause) = 0;

protected:
	int m_sampleRate = 44100;
	int m_sampleSize = 16;
	int m_channels = 2;
};

#endif // !_WAUDIOPLAY_H_