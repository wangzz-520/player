#pragma once

#include <QPushButton>
#include <QPixmap>
#include <QPainter>

class WImageButton : public QPushButton
{
public:
	WImageButton(QWidget *parent = 0);

	void setImage(QPixmap pixMap);

protected:
	void paintEvent(QPaintEvent* event);

private:
	QPixmap m_pixmap;
};

