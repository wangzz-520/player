#include "WImageButton.h"

WImageButton::WImageButton(QWidget *parent)
	: QPushButton(parent)
{

}

void WImageButton::setImage(QPixmap pixMap)
{
	m_pixmap = pixMap;
	setFixedSize(m_pixmap.size());
	update();
}

void WImageButton::paintEvent(QPaintEvent* event) 
{
	QPainter painter(this);
	painter.drawPixmap(rect(), m_pixmap);
}