#include "WProgressBar.h"


WProgressBar::WProgressBar(QWidget *parent)
	: QWidget(parent)
{
	this->setWindowFlags(Qt::FramelessWindowHint);          //隐藏窗口
	this->setAttribute(Qt::WA_TranslucentBackground, true); //窗口透明
}

WProgressBar::~WProgressBar()
{
}

double WProgressBar::getPos()
{
	return m_pos;
}

void WProgressBar::slotSetValue(double pos)
{
	m_pos = pos;
	update();
}

void WProgressBar::mousePressEvent(QMouseEvent *ev)
{
	//double pos = (double)ev->pos().x() / (double)width();
	//if (pos >= 1)
	//	pos = 1;

	//if (pos <= 0)
	//	pos = 0;

	//m_pos = pos;
	//update();
	//qDebug() << "seek pos = " << pos;
	//emit sigCustomSliderValueChanged(pos);
}

void WProgressBar::mouseMoveEvent(QMouseEvent *ev)
{
	double pos = (double)ev->pos().x() / (double)width();
	if (pos >= 1)
		pos = 1;

	if (pos <= 0)
		pos = 0;

	m_pos = pos;
	update();
}

void WProgressBar::mouseReleaseEvent(QMouseEvent *ev)
{
	double pos = (double)ev->pos().x() / (double)width();
	emit sigCustomSliderValueChanged(pos);
}

void WProgressBar::paintEvent(QPaintEvent *e)
{
	QWidget::paintEvent(e);
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	//绘制底图矩形
	QBrush brush;
	brush.setColor(QColor(233,233,233));
	brush.setStyle(Qt::SolidPattern);
	painter.setBrush(brush);
	painter.drawRoundedRect(this->rect(), 5, 5);

	//绘制播放进度
	QLinearGradient radial;

	radial.setStart(0, 0);
	radial.setFinalStop(0, 1);

	//设置起始点颜色，0表示起始
	radial.setColorAt(0, QColor("#87CEFA"));

	//设置终点颜色 1表示终点
	radial.setColorAt(1, QColor("#1E90FF"));

	//设置延展方式
	radial.setSpread(QGradient::PadSpread);

	QPen pen(QBrush("#1E90FF"), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
	painter.setPen(pen);

	//设置画刷
	painter.setBrush(radial);

	QRect rect = this->rect();
	rect.setWidth(rect.width() * m_pos);

	//画矩形
	painter.drawRoundedRect(rect, 5, 5);
}
