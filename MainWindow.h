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
	void region(const QPoint &currentGlobalPoint);  //����λ��,�ı���

private slots:
	void slotActionOpen();
	void slotActionOpenMore();
	void slotActionOpenNet();
	void slotActionExit();
	void slotSetPause(bool isPause);
	void slotSeek(double pos);
	void slotVolumn(double pos);

protected:
	//˫��ȫ��
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
	bool m_isStop = false;	//�Ƿ���ֹͣ����

	QPoint m_movePoint;  //����λ��
	bool isLeftPressDown;  // �ж�����Ƿ���
	Direction dir;        // ���ڴ�С�ı�ʱ����¼�ı䷽��
	bool m_isShowMax = false;

	QMenu *m_menu = nullptr;
	QAction *m_open = nullptr;
	QAction *m_openMore = nullptr;
	QAction *m_openNet = nullptr;

	WNetDialog *m_dlg = nullptr;
};
