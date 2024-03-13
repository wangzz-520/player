#include "MainWindow.h"
#include "GlobalHelper.h"
#include <QFileDialog>
#include <QAction>
#include "WDemuxThread.h"
#include "SlideAnimationWidget.h"
#include "WAudioPlay.h"
#include "WNetDialog.h"
#include <QAudio>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	this->setWindowFlag(Qt::FramelessWindowHint);
	this->menuBar()->hide();

	m_menu = new QMenu(this);
	m_open = m_menu->addAction("打开");
	m_openMore = m_menu->addAction("打开多个文件");
	m_openNet = m_menu->addAction("打开网络流");

	m_dlg = new WNetDialog(this);

	//加载样式
	QString qss = GlobalHelper::GetQssStr(":/qss/qss/MainWidget.qss");
	setStyleSheet(qss);

	this->setWindowTitle("ZPlayer");
	this->setWindowIcon(QIcon(":/image/image/player.png"));
	this->removeToolBar(ui.mainToolBar);
	this->statusBar()->hide();
	this->setMinimumSize(800, 600);

	connect(m_open, &QAction::triggered, this, &MainWindow::slotActionOpen);
	connect(m_openMore, &QAction::triggered, this, &MainWindow::slotActionOpenMore);
	connect(m_openNet, &QAction::triggered, this, &MainWindow::slotActionOpenNet);
	connect(ui.actionExit, &QAction::triggered, this, &MainWindow::slotActionExit);
	connect(ui.ctrlBarWidget, &WCtrlBarWidget::sigPause, this, &MainWindow::slotSetPause);
	connect(ui.ctrlBarWidget, &WCtrlBarWidget::sigSeek, this, &MainWindow::slotSeek);
	connect(ui.ctrlBarWidget, &WCtrlBarWidget::sigBackward, this, &MainWindow::slotBackward);
	connect(ui.ctrlBarWidget, &WCtrlBarWidget::sigForward, this, &MainWindow::slotForward);
	connect(ui.ctrlBarWidget, &WCtrlBarWidget::sigStop, this, &MainWindow::slotStop);
	connect(ui.ctrlBarWidget, &WCtrlBarWidget::sigVolumn, this, &MainWindow::slotVolumn);

	connect(ui.btnMin, &QPushButton::clicked, this, &MainWindow::slotMin);
	connect(ui.btnMax, &QPushButton::clicked, this, &MainWindow::slotMax);
	connect(ui.btnClose, &QPushButton::clicked, this, &MainWindow::slotClose);

	if (!m_demuxThread)
	{
		m_demuxThread = new WDemuxThread(this);
	}

	m_animationWidget = new SlideAnimationWidget(this);
	connect(m_animationWidget, &SlideAnimationWidget::sigShowVideo, this, &MainWindow::slotShowVideo);
	m_animationWidget->setPos(this->width()-POS_X, POS_Y);
}

MainWindow::~MainWindow()
{
	if (m_demuxThread)
	{
		m_demuxThread->close();
		delete m_demuxThread;
	}
}

void MainWindow::region(const QPoint &currentGlobalPoint)
{
	// 获取窗体在屏幕上的位置区域，topLeft为坐上角点，rightButton为右下角点
	QRect rect = this->rect();

	QPoint topLeft = this->mapToGlobal(rect.topLeft()); //将左上角的(0,0)转化为全局坐标
	QPoint rightButton = this->mapToGlobal(rect.bottomRight());

	int x = currentGlobalPoint.x(); //当前鼠标的坐标
	int y = currentGlobalPoint.y();

	if (((topLeft.x() + PADDING >= x) && (topLeft.x() <= x))
		&& ((topLeft.y() + PADDING >= y) && (topLeft.y() <= y)))
	{
		// 左上角
		dir = LEFTTOP;
		this->setCursor(QCursor(Qt::SizeFDiagCursor));  // 设置光标形状
	}
	else if (((x >= rightButton.x() - PADDING) && (x <= rightButton.x()))
		&& ((y >= rightButton.y() - PADDING) && (y <= rightButton.y())))
	{
		// 右下角
		dir = RIGHTBOTTOM;
		this->setCursor(QCursor(Qt::SizeFDiagCursor));
	}
	else if (((x <= topLeft.x() + PADDING) && (x >= topLeft.x()))
		&& ((y >= rightButton.y() - PADDING) && (y <= rightButton.y())))
	{
		//左下角
		dir = LEFTBOTTOM;
		this->setCursor(QCursor(Qt::SizeBDiagCursor));
	}
	else if (((x <= rightButton.x()) && (x >= rightButton.x() - PADDING))
		&& ((y >= topLeft.y()) && (y <= topLeft.y() + PADDING)))
	{
		// 右上角
		dir = RIGHTTOP;
		this->setCursor(QCursor(Qt::SizeBDiagCursor));
	}
	else if ((x <= topLeft.x() + PADDING) && (x >= topLeft.x()))
	{
		// 左边
		dir = LEFT;
		this->setCursor(QCursor(Qt::SizeHorCursor));
	}
	else if ((x <= rightButton.x()) && (x >= rightButton.x() - PADDING))
	{
		// 右边
		dir = RIGHT;
		this->setCursor(QCursor(Qt::SizeHorCursor));
	}
	else if ((y >= topLeft.y()) && (y <= topLeft.y() + PADDING))
	{
		// 上边
		dir = UP;
		this->setCursor(QCursor(Qt::SizeVerCursor));
	}
	else if ((y <= rightButton.y()) && (y >= rightButton.y() - PADDING))
	{
		// 下边
		dir = DOWN;
		this->setCursor(QCursor(Qt::SizeVerCursor));
	}
	else
	{
		// 默认
		dir = NONE;
		this->setCursor(QCursor(Qt::ArrowCursor));
	}
}

void MainWindow::slotActionOpen()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
		"./",
		tr("videos (*.mp4 *.mkv *.flv)"));

	if (fileName.isEmpty())
		return;

	m_animationWidget->addList(fileName);
	slotShowVideo(fileName);
}

void MainWindow::slotActionOpenMore()
{
	QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Open File"),
		"./",
		tr("videos (*.mp4 *.mkv *.flv)"));

	if (fileNames.isEmpty())
		return;

	m_animationWidget->addList(fileNames);
	slotShowVideo(fileNames.at(0));
}

void MainWindow::slotActionOpenNet()
{
	if (m_dlg->exec() == QDialog::Accepted)
	{
		slotShowVideo(m_dlg->getNetPath());
	}
}

void MainWindow::slotActionExit()
{
	qApp->exit();
}

void MainWindow::slotSetPause(bool isPause)
{
	if (m_animationWidget->getCurPlayFileName().isEmpty())
		return;

	if (!m_isStop)
	{
		if (m_demuxThread)
			m_demuxThread->setPause(isPause);

	}
	else
	{
		m_isStop = false;
		slotShowVideo(m_animationWidget->getCurPlayFileName());
	}
}

void MainWindow::slotSeek(double pos)
{
	if (m_demuxThread)
		m_demuxThread->seek(pos);
}

void MainWindow::slotVolumn(double pos)
{
	pos *= 100;
	qreal linearVolume = QAudio::convertVolume(pos  / qreal(100.0),
		QAudio::LogarithmicVolumeScale,
		QAudio::LinearVolumeScale);

	//qDebug() << "pos = " << pos << " linearVolume = " << linearVolume << " qRound(linearVolume * 100) = " << qRound(linearVolume * 100);

	WAudioPlay::getInstance()->setVolume(qRound(linearVolume * 100));
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *e)
{
	if (e->button() == Qt::LeftButton)
	{
		if (isFullScreen())
		{
			ui.ctrlBarWidget->show();
			m_animationWidget->show();
			if (m_isShowMax)
				this->showMaximized();
			else
				this->showNormal();
			ui.widget->show();
		}
		else
		{
			ui.ctrlBarWidget->hide();
			m_animationWidget->hide();
			this->showFullScreen();
			ui.widget->hide();
		}
	}
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
	m_animationWidget->setPos(this->width() - POS_X, POS_Y);
	m_animationWidget->setFixedHeight(event->size().height() - 100);
	update();
}

void MainWindow::showEvent(QShowEvent *event)
{
	Q_UNUSED(event);

	if (m_isInit)
		return;

	setWindowState(Qt::WindowFullScreen);
	showNormal();

	m_isInit = true;
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		isLeftPressDown = false;
		if (dir != NONE)
		{
			this->releaseMouse(); //释放鼠标抓取
			this->setCursor(QCursor(Qt::ArrowCursor));
			dir = NONE; 
		}
	}
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
	QPoint globalPoint = event->globalPos();   //鼠标全局坐标

	QRect rect = this->rect();  //rect == QRect(0,0 1280x720)
	QPoint topLeft = mapToGlobal(rect.topLeft());
	QPoint bottomRight = mapToGlobal(rect.bottomRight());

	if (this->windowState() != Qt::WindowMaximized)
	{
		if (!isLeftPressDown)  //没有按下左键时
		{
			this->region(globalPoint); //窗口大小的改变——判断鼠标位置，改变光标形状
		}
		else
		{
			if (dir != NONE)
			{
				QRect newRect(topLeft, bottomRight); //定义一个矩形  拖动后最大1000*1618

				switch (dir)
				{
				case LEFT:

					if (bottomRight.x() - globalPoint.x() <= this->minimumWidth())
					{
						newRect.setLeft(topLeft.x());  //小于界面的最小宽度时，设置为左上角横坐标为窗口x
						//只改变左边界
					}
					else
					{
						newRect.setLeft(globalPoint.x());
					}
					break;
				case RIGHT:
					newRect.setWidth(globalPoint.x() - topLeft.x());  //只能改变右边界
					break;
				case UP:
					if (bottomRight.y() - globalPoint.y() <= this->minimumHeight())
					{
						newRect.setY(topLeft.y());
					}
					else
					{
						newRect.setY(globalPoint.y());
					}
					break;
				case DOWN:
					newRect.setHeight(globalPoint.y() - topLeft.y());
					break;
				case LEFTTOP:
					if (bottomRight.x() - globalPoint.x() <= this->minimumWidth())
					{
						newRect.setX(topLeft.x());
					}
					else
					{
						newRect.setX(globalPoint.x());
					}

					if (bottomRight.y() - globalPoint.y() <= this->minimumHeight())
					{
						newRect.setY(topLeft.y());
					}
					else
					{
						newRect.setY(globalPoint.y());
					}
					break;
				case RIGHTTOP:
					if (globalPoint.x() - topLeft.x() >= this->minimumWidth())
					{
						newRect.setWidth(globalPoint.x() - topLeft.x());
					}
					else
					{
						newRect.setWidth(bottomRight.x() - topLeft.x());
					}
					if (bottomRight.y() - globalPoint.y() >= this->minimumHeight())
					{
						newRect.setY(globalPoint.y());
					}
					else
					{
						newRect.setY(topLeft.y());
					}
					break;
				case LEFTBOTTOM:
					if (bottomRight.x() - globalPoint.x() >= this->minimumWidth())
					{
						newRect.setX(globalPoint.x());
					}
					else
					{
						newRect.setX(topLeft.x());
					}
					if (globalPoint.y() - topLeft.y() >= this->minimumHeight())
					{
						newRect.setHeight(globalPoint.y() - topLeft.y());
					}
					else
					{
						newRect.setHeight(bottomRight.y() - topLeft.y());
					}
					break;
				case RIGHTBOTTOM:
					newRect.setWidth(globalPoint.x() - topLeft.x());
					newRect.setHeight(globalPoint.y() - topLeft.y());
					break;
				default:
					break;
				}
				this->setGeometry(newRect);
			}
			else
			{
				move(event->globalPos() - m_movePoint); //移动窗口
				event->accept();
			}
		}
	}
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
	switch (event->button())
	{
	case Qt::LeftButton:
		isLeftPressDown = true;

		if (dir != NONE)
		{
			this->mouseGrabber(); //返回当前抓取鼠标输入的窗口
		}
		else
		{
			m_movePoint = event->globalPos() - this->frameGeometry().topLeft();
			//globalPos()鼠标位置，topLeft()窗口左上角的位置
		}
		break;
	case Qt::RightButton:
		//this->setWindowState(Qt::WindowMinimized);
		m_menu->exec(event->globalPos());
		break;
	default:
		MainWindow::mousePressEvent(event);
	}
}

void MainWindow::slotShowVideo(const QString &fileName)
{
	//视频数据
	VideoDataFunc videoFunc = std::bind(&WOpenGLWidget::slotReceiveVideoData, ui.openGLWidget,
		std::placeholders::_1);

	//视频信息
	VideoInfoFunc videoInfoFunc = std::bind(&MainWindow::slotReceiveVideoInfo, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

	//当前播放的时间
	TimeFunc timeFunc = std::bind(&WCtrlBarWidget::slotSetTime, ui.ctrlBarWidget,
		std::placeholders::_1);

	m_demuxThread->clear();
	m_demuxThread->open(fileName.toStdString().c_str(), videoFunc, videoInfoFunc, timeFunc);
	m_demuxThread->setPause(false);

	if (!m_demuxThread->isRunning())
		m_demuxThread->start();
}
/**
 * 上一个.
 * 
 */
void MainWindow::slotBackward()
{
	QString fileName = m_animationWidget->getNextPlayFileName(false);
	if(!fileName.isEmpty())
		slotShowVideo(fileName);
}

/**
 * 下一个.
 * 
 */
void MainWindow::slotForward()
{
	QString fileName = m_animationWidget->getNextPlayFileName(true);
	if (!fileName.isEmpty())
		slotShowVideo(fileName);
}

void MainWindow::slotStop()
{
	if (m_demuxThread)
	{
		m_demuxThread->setPause(false);
		m_demuxThread->close();
	}

	ui.openGLWidget->clear();
	ui.ctrlBarWidget->clear();

	m_isStop = true;
	ui.ctrlBarWidget->slotSetTime(0);
}

void MainWindow::slotReceiveVideoInfo(int width, int height, int64_t totalTime)
{
	ui.openGLWidget->slotOpenVideo(width, height);
	ui.ctrlBarWidget->slotStartPlay(totalTime);
}

void MainWindow::slotMin()
{
	this->showMinimized();
}

void MainWindow::slotMax()
{
	if (!m_isShowMax)
		this->showMaximized();
	else
		this->showNormal();

	m_isShowMax = !m_isShowMax;
}

void MainWindow::slotClose()
{
	qApp->exit();
}
