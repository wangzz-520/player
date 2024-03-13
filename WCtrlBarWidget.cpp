#include "WCtrlBarWidget.h"
#include "GlobalHelper.h"

WCtrlBarWidget::WCtrlBarWidget(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	ui.btnVolume->setToolTip("静音");
	ui.btnForward->setToolTip("下一个");
	ui.btnBackward->setToolTip("上一个");
	ui.btnStop->setToolTip("停止");
	ui.btnPlayOrPause->setToolTip("播放");

	//防止出现锯齿
	m_playImage = QPixmap::fromImage(QImage(":/image/image/play.png").scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));
	m_pauseImage = QPixmap::fromImage(QImage(":/image/image/pause.png").scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));
	m_volumnOnImgae = QPixmap::fromImage(QImage(":/image/image/voice_on.png").scaled(18, 18, Qt::KeepAspectRatio, Qt::SmoothTransformation));
	m_volumnOffImage = QPixmap::fromImage(QImage(":/image/image/voice_off.png").scaled(18, 18, Qt::KeepAspectRatio, Qt::SmoothTransformation));

	ui.btnStop->setImage(QPixmap::fromImage(QImage(":/image/image/stop.png").scaled(30, 30, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
	ui.btnBackward->setImage(QPixmap::fromImage(QImage(":/image/image/pre.png").scaled(30, 30, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
	ui.btnForward->setImage(QPixmap::fromImage(QImage(":/image/image/next.png").scaled(30, 30, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
	ui.btnVolume->setImage(m_volumnOnImgae);
	ui.btnPlayOrPause->setImage(m_playImage);

	setStyleSheet(GlobalHelper::GetQssStr(":/qss/qss/WCtrlBarWidget.qss"));

	ui.videoPlayTimeTimeEdit->setFocusPolicy(Qt::NoFocus);
	ui.videoTotalTimeTimeEdit->setFocusPolicy(Qt::NoFocus);

	ui.progressBar->slotSetValue(0);
	ui.volumnBar->slotSetValue(1);

	connect(ui.btnBackward, &QPushButton::clicked, this, &WCtrlBarWidget::sigBackward);
	connect(ui.btnForward, &QPushButton::clicked, this, &WCtrlBarWidget::sigForward);
	connect(ui.btnPlayOrPause, &QPushButton::clicked, this, &WCtrlBarWidget::slotPlayOrPause);
	connect(ui.btnStop, &QPushButton::clicked, this, &WCtrlBarWidget::sigStop);
	connect(ui.btnVolume, &QPushButton::clicked, this, &WCtrlBarWidget::slotVolume);

	connect(ui.progressBar, &WProgressBar::sigCustomSliderValueChanged, this, &WCtrlBarWidget::sigSeek);
	connect(ui.volumnBar, &WProgressBar::sigCustomSliderValueChanged, this, &WCtrlBarWidget::sigVolumn);
}

WCtrlBarWidget::~WCtrlBarWidget()
{

}

void WCtrlBarWidget::slotSetTime(int curSec)
{
	int thh, tmm, tss;
	thh = curSec / 3600;
	tmm = (curSec % 3600) / 60;
	tss = (curSec % 60);
	QTime TotalTime2(thh, tmm, tss);

	ui.progressBar->slotSetValue(curSec * 1.0 / m_totalTime);

	ui.videoPlayTimeTimeEdit->setTime(TotalTime2);
}

void WCtrlBarWidget::slotStartPlay(int totalSec)
{
	m_totalTime = totalSec;
	m_isStartPlay = true;

	int thh, tmm, tss;
	thh = totalSec / 3600;
	tmm = (totalSec % 3600) / 60;
	tss = (totalSec % 60);
	QTime TotalTime(thh, tmm, tss);

	ui.videoTotalTimeTimeEdit->setTime(TotalTime);

	ui.btnPlayOrPause->setImage(m_pauseImage);

	m_isStartPlay = true;
}

void WCtrlBarWidget::clear()
{
	QTime TotalTime(0,0,0);
	//ui.playSlider->setValue(0);
	ui.videoTotalTimeTimeEdit->setTime(TotalTime);
	ui.videoPlayTimeTimeEdit->setTime(TotalTime);

	m_isStartPlay = false;
	ui.btnPlayOrPause->setImage(m_playImage);
	ui.btnPlayOrPause->setToolTip("播放");
}

void WCtrlBarWidget::slotPlayOrPause()
{
	m_isStartPlay = !m_isStartPlay;

	if (m_isStartPlay)
	{
		ui.btnPlayOrPause->setImage(m_pauseImage);
		ui.btnPlayOrPause->setToolTip("暂停");
	}
	else
	{
		ui.btnPlayOrPause->setImage(m_playImage);
		ui.btnPlayOrPause->setToolTip("播放");
	}
	
	emit sigPause(!m_isStartPlay);
}

void WCtrlBarWidget::slotVolume()
{
	if (m_isVolumnAll)
	{
		emit sigVolumn(0);
		ui.volumnBar->slotSetValue(0);
		ui.btnVolume->setImage(m_volumnOffImage);
	}
	else
	{
		emit sigVolumn(1);
		ui.volumnBar->slotSetValue(1);
		ui.btnVolume->setImage(m_volumnOnImgae);
	}
	m_isVolumnAll = !m_isVolumnAll;
}
