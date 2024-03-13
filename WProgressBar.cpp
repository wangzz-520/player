#include "WProgressBar.h"


WProgressBar::WProgressBar(QWidget *parent)
	: QWidget(parent)
{
	this->setWindowFlags(Qt::FramelessWindowHint);          //���ش���
	this->setAttribute(Qt::WA_TranslucentBackground, true); //����͸��
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

	//���Ƶ�ͼ����
	QBrush brush;
	brush.setColor(QColor(233,233,233));
	brush.setStyle(Qt::SolidPattern);
	painter.setBrush(brush);
	painter.drawRoundedRect(this->rect(), 5, 5);

	//���Ʋ��Ž���
	QLinearGradient radial;

	radial.setStart(0, 0);
	radial.setFinalStop(0, 1);

	//������ʼ����ɫ��0��ʾ��ʼ
	radial.setColorAt(0, QColor("#87CEFA"));

	//�����յ���ɫ 1��ʾ�յ�
	radial.setColorAt(1, QColor("#1E90FF"));

	//������չ��ʽ
	radial.setSpread(QGradient::PadSpread);

	QPen pen(QBrush("#1E90FF"), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
	painter.setPen(pen);

	//���û�ˢ
	painter.setBrush(radial);

	QRect rect = this->rect();
	rect.setWidth(rect.width() * m_pos);

	//������
	painter.drawRoundedRect(rect, 5, 5);
}
