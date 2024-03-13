#pragma once

#include <QMainWindow>
#include <QMouseEvent>
#include <QMenu>
#include <QAction>
#include "ui_MainWindow.h"
#include "global.h"

#define PADDING 2

class WDemuxThread;
class SlideAnimationWidget;
class WNetDialog;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = nullptr);
	~MainWindow();

	enum Direction { UP = 0, DOWN = 1, LEFT, RIGHT, LEFTTOP, LEFTBOTTOM, RIGHTBOTTOM, RIGHTTOP, NONE };

public:
	void region(const QPoint &currentGlobalPoint);  //鼠标的位置,改变光标

private slots:
	void slotActionOpen();
	void slotActionOpenMore();
	void slotActionOpenNet();
	void slotActionExit();
	void slotSetPause(bool isPause);
	void slotSeek(double pos);
	void slotVolumn(double pos);

protected:
	//双击全屏
	virtual void mouseDoubleClickEvent(QMouseEvent *e);
	virtual void resizeEvent(QResizeEvent *event);
	virtual void showEvent(QShowEvent *event);
	virtual void mouseReleaseEvent(QMouseEvent *event);
	virtual void mouseMoveEvent(QMouseEvent *event);
	virtual void mousePressEvent(QMouseEvent *event);

private slots:
	void slotShowVideo(const QString &fileName);
	void slotBackward();
	void slotForward();
	void slotStop();

	void slotReceiveVideoInfo(int width,int height, int64_t totalTime);
	void slotMin();
	void slotMax();
	void slotClose();

private:
	Ui::MainWindowClass ui;

private:
	WDemuxThread *m_demuxThread = nullptr;

	SlideAnimationWidget *m_animationWidget = nullptr;

	bool m_isInit = false;
	bool m_isStop = false;	//是否点击停止播放

	QPoint m_movePoint;  //鼠标的位置
	bool isLeftPressDown;  // 判断左键是否按下
	Direction dir;        // 窗口大小改变时，记录改变方向
	bool m_isShowMax = false;

	QMenu *m_menu = nullptr;
	QAction *m_open = nullptr;
	QAction *m_openMore = nullptr;
	QAction *m_openNet = nullptr;

	WNetDialog *m_dlg = nullptr;
};
