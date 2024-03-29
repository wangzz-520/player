#pragma once

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QDebug>
#include <QShowEvent>
#include <QPainter>

class WProgressBar : public QWidget
{
	Q_OBJECT
public:
	WProgressBar(QWidget *parent = nullptr);
	~WProgressBar();

signals:
	void sigCustomSliderValueChanged(double pos);//自定义的鼠标单击信号，用于捕获并处理

public:
	//获取pos
	double getPos();

public slots:
	//设置0~1
	void slotSetValue(double pos);

protected:
	void mousePressEvent(QMouseEvent *ev);
	void mouseMoveEvent(QMouseEvent *ev);
	void mouseReleaseEvent(QMouseEvent *ev);
	void paintEvent(QPaintEvent *);

private:
	double m_pos = 0;
};

