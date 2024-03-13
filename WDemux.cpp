#include "WDemux.h"
#include <QDebug>
#include <QElapsedTimer>

WDemux::WDemux()
{
	av_register_all();

	avformat_network_init();
}

WDemux::~WDemux()
{

}

bool WDemux::open(const char *url, VideoInfoFunc videoInfoFunc)
{
	//�ر���һ�δ򿪵�
	close();

	//��������
	AVDictionary *opts = NULL;
	//����rtsp����tcpЭ���
	av_dict_set(&opts, "rtsp_transport", "tcp", 0);
	av_dict_set(&opts, "max_delay", "500", 0);
	av_dict_set(&opts, "buffer_size", "1024000", 0);
	av_dict_set(&opts, "probsize", "4096", 0);
	av_dict_set(&opts, "fps", "25", 0);

	m_mux.lock();
	//��������Ƶ�ļ�
	int ret = avformat_open_input(&m_pFormatCtx, url, NULL, &opts);
	if (ret != 0)
	{
		m_mux.unlock();
		char buf[1024] = { 0 };
		av_strerror(ret, buf, sizeof(buf) - 1);
		cout << "open " << url << " failed! :" << buf << endl;
		return false;
	}

	ret = avformat_find_stream_info(m_pFormatCtx, NULL);
	if (ret < 0)
	{
		m_mux.unlock();
		char buf[1024] = { 0 };
		av_strerror(ret, buf, sizeof(buf) - 1);
		cout << "find stream  information " << url << " failed! :" << buf << endl;
		return false;
	}

	for (int i = 0; i < m_pFormatCtx->nb_streams/*����Ƶ���ĸ���*/; i++)
	{
		//������Ƶ
		if (m_pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			m_videoIndex = i;
		}
		else if (m_pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			m_audioIndex = i;
		}
	}

	//===================video=================
	//��Ƶ��
	m_width = m_pFormatCtx->streams[m_videoIndex]->codecpar->width;

	//��Ƶ��
	m_height = m_pFormatCtx->streams[m_videoIndex]->codecpar->height;

	//��Ƶ��ʱ��s
	m_totalTime = static_cast<double>(m_pFormatCtx->duration) / AV_TIME_BASE;

	videoInfoFunc(m_width, m_height, m_totalTime);

	//��ȡ֡��;
	m_fps = r2d(m_pFormatCtx->streams[m_videoIndex]->avg_frame_rate);
	if (m_fps == 0)
	{
		m_fps = 25;
	}

	m_max_frame_duration = (m_pFormatCtx->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;

	m_vTimeBase = r2d(m_pFormatCtx->streams[m_videoIndex]->time_base);

	cout << "=======================================================" << endl;
	cout << m_videoIndex << " video info" << endl;
	cout << "codec_id = " << m_pFormatCtx->streams[m_videoIndex]->codecpar->codec_id << endl;
	cout << "format = " << m_pFormatCtx->streams[m_videoIndex]->codecpar->format << endl;
	cout << "width=" << m_width << endl;
	cout << "height=" << m_height << endl;
	cout << "fps=" << m_fps << endl;
	cout << "totalTime=" << m_totalTime << endl;
	cout << "=======================================================" << endl;

	//===================video=================

	//===================audio=================
	if (m_audioIndex >= 0)
	{
		m_sampleRate = m_pFormatCtx->streams[m_audioIndex]->codec->sample_rate;
		m_channels = m_pFormatCtx->streams[m_audioIndex]->codec->channels;
		m_aTimeBase = r2d(m_pFormatCtx->streams[m_audioIndex]->time_base);

		cout << "=======================================================" << endl;
		cout << m_audioIndex << " audio info" << endl;
		cout << "codec_id = " << m_pFormatCtx->streams[m_audioIndex]->codecpar->codec_id << endl;
		cout << "format = " << m_pFormatCtx->streams[m_audioIndex]->codecpar->format << endl;
		cout << "sample_rate = " << m_sampleRate << endl;
		cout << "channels = " << m_channels << endl;
		cout << "=======================================================" << endl;
	}
	//===================audio=================

	m_mux.unlock();

	return true;
}

AVPacket * WDemux::read()
{
	m_mux.lock();
	if (!m_pFormatCtx)
	{
		m_mux.unlock();
		return 0;
	}

	AVPacket *pkt = av_packet_alloc();
	//��ȡһ֡��������ռ�
	int ret = av_read_frame(m_pFormatCtx, pkt);
	if (ret != 0)
	{
		av_packet_free(&pkt);
		m_mux.unlock();
		return 0;
	}
	//ptsת��Ϊ����
	pkt->pts = pkt->pts*(1000 * (r2d(m_pFormatCtx->streams[pkt->stream_index]->time_base)));
	pkt->dts = pkt->dts*(1000 * (r2d(m_pFormatCtx->streams[pkt->stream_index]->time_base)));
	m_mux.unlock();
	return pkt;
}

bool WDemux::isAudio(AVPacket *packet)
{
	if(!packet)
		return false;

	if (packet->stream_index == m_videoIndex)
		return false;
		
	return true;
}

AVCodecParameters * WDemux::videoPara()
{
	m_mux.lock();
	if (!m_pFormatCtx || m_videoIndex < 0)
	{
		m_mux.unlock();
		return nullptr;
	}

	AVCodecParameters *pa = avcodec_parameters_alloc();
	avcodec_parameters_copy(pa, m_pFormatCtx->streams[m_videoIndex]->codecpar);
	m_mux.unlock();
	return pa;
}

AVCodecParameters * WDemux::audioPara()
{
	m_mux.lock();
	if (!m_pFormatCtx || m_audioIndex < 0)
	{
		m_mux.unlock();
		return nullptr;
	}

	AVCodecParameters *pa = avcodec_parameters_alloc();
	avcodec_parameters_copy(pa, m_pFormatCtx->streams[m_audioIndex]->codecpar);
	m_mux.unlock();
	return pa;
}

bool WDemux::seek(double pos)
{
	m_mux.lock();
	if (!m_pFormatCtx)
	{
		m_mux.unlock();
		return false;
	}
	//�����ȡ����
	avformat_flush(m_pFormatCtx);

	long long seekPos = 0;
	seekPos = m_pFormatCtx->streams[m_videoIndex]->duration * pos;
	int re = av_seek_frame(m_pFormatCtx, m_videoIndex, seekPos, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME);
	m_mux.unlock();
	if (re < 0) 
		return false;

	return true;
}

void WDemux::clear()
{
	m_mux.lock();
	if (!m_pFormatCtx)
	{
		m_mux.unlock();
		return;
	}
	//�����ȡ����
	avformat_flush(m_pFormatCtx);
	m_mux.unlock();
}

void WDemux::close()
{
	m_mux.lock();
	if (!m_pFormatCtx)
	{
		m_mux.unlock();
		return;
	}
	avformat_close_input(&m_pFormatCtx);
	//ý����ʱ�������룩
	m_totalTime = 0;
	m_mux.unlock();
}

AVPacket * WDemux::readVideo()
{
	m_mux.lock();
	if (!m_pFormatCtx) //�ݴ�
	{
		m_mux.unlock();
		return 0;
	}
	m_mux.unlock();

	AVPacket *pkt = NULL;
	//��ֹ����
	for (int i = 0; i < 50; i++)
	{
		pkt = read();
		if (!pkt)
			continue;

		if (pkt->stream_index == m_videoIndex)
		{
			break;
		}

		av_packet_free(&pkt);
	}
	return pkt;
}

double WDemux::r2d(AVRational r)
{
	return r.den == 0 ? 0 : (double)r.num / (double)r.den;
}

