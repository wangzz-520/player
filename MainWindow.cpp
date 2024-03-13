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
	m_open = m_menu->addAction("��");
	m_openMore = m_menu->addAction("�򿪶���ļ�");
	m_openNet = m_menu->addAction("��������");

	m_dlg = new WNetDialog(this);

	//������ʽ
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
	// ��ȡ��������Ļ�ϵ�λ������topLeftΪ���Ͻǵ㣬rightButtonΪ���½ǵ�
	QRect rect = this->rect();

	QPoint topLeft = this->mapToGlobal(rect.topLeft()); //�����Ͻǵ�(0,0)ת��Ϊȫ������
	QPoint rightButton = this->mapToGlobal(rect.bottomRight());

	int x = currentGlobalPoint.x(); //��ǰ��������
	int y = currentGlobalPoint.y();

	if (((topLeft.x() + PADDING >= x) && (topLeft.x() <= x))
		&& ((topLeft.y() + PADDING >= y) && (topLeft.y() <= y)))
	{
		// ���Ͻ�
		dir = LEFTTOP;
		this->setCursor(QCursor(Qt::SizeFDiagCursor));  // ���ù����״
	}
	else if (((x >= rightButton.x() - PADDING) && (x <= rightButton.x()))
		&& ((y >= rightButton.y() - PADDING) && (y <= rightButton.y())))
	{
		// ���½�
		dir = RIGHTBOTTOM;
		this->setCursor(QCursor(Qt::SizeFDiagCursor));
	}
	else if (((x <= topLeft.x() + PADDING) && (x >= topLeft.x()))
		&& ((y >= rightButton.y() - PADDING) && (y <= rightButton.y())))
	{
		//���½�
		dir = LEFTBOTTOM;
		this->setCursor(QCursor(Qt::SizeBDiagCursor));
	}
	else if (((x <= rightButton.x()) && (x >= rightButton.x() - PADDING))
		&& ((y >= topLeft.y()) && (y <= topLeft.y() + PADDING)))
	{
		// ���Ͻ�
		dir = RIGHTTOP;
		this->setCursor(QCursor(Qt::SizeBDiagCursor));
	}
	else if ((x <= topLeft.x() + PADDING) && (x >= topLeft.x()))
	{
		// ���
		dir = LEFT;
		this->setCursor(QCursor(Qt::SizeHorCursor));
	}
	else if ((x <= rightButton.x()) && (x >= rightButton.x() - PADDING))
	{
		// �ұ�
		dir = RIGHT;
		this->setCursor(QCursor(Qt::SizeHorCursor));
	}
	else if ((y >= topLeft.y()) && (y <= topLeft.y() + PADDING))
	{
		// �ϱ�
		dir = UP;
		this->setCursor(QCursor(Qt::SizeVerCursor));
	}
	else if ((y <= rightButton.y()) && (y >= rightButton.y() - PADDING))
	{
		// �±�
		dir = DOWN;
		this->setCursor(QCursor(Qt::SizeVerCursor));
	}
	else
	{
		// Ĭ��
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
			this->releaseMouse(); //�ͷ����ץȡ
			this->setCursor(QCursor(Qt::ArrowCursor));
			dir = NONE; 
		}
	}
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
	QPoint globalPoint = event->globalPos();   //���ȫ������

	QRect rect = this->rect();  //rect == QRect(0,0 1280x720)
	QPoint topLeft = mapToGlobal(rect.topLeft());
	QPoint bottomRight = mapToGlobal(rect.bottomRight());

	if (this->windowState() != Qt::WindowMaximized)
	{
		if (!isLeftPressDown)  //û�а������ʱ
		{
			this->region(globalPoint); //���ڴ�С�ĸı䡪���ж����λ�ã��ı�����״
		}
		else
		{
			if (dir != NONE)
			{
				QRect newRect(topLeft, bottomRight); //����һ������  �϶������1000*1618

				switch (dir)
				{
				case LEFT:

					if (bottomRight.x() - globalPoint.x() <= this->minimumWidth())
					{
						newRect.setLeft(topLeft.x());  //С�ڽ������С���ʱ������Ϊ���ϽǺ�����Ϊ����x
						//ֻ�ı���߽�
					}
					else
					{
						newRect.setLeft(globalPoint.x());
					}
					break;
				case RIGHT:
					newRect.setWidth(globalPoint.x() - topLeft.x());  //ֻ�ܸı��ұ߽�
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
				move(event->globalPos() - m_movePoint); //�ƶ�����
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
			this->mouseGrabber(); //���ص�ǰץȡ�������Ĵ���
		}
		else
		{
			m_movePoint = event->globalPos() - this->frameGeometry().topLeft();
			//globalPos()���λ�ã�topLeft()�������Ͻǵ�λ��
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
	//��Ƶ����
	VideoDataFunc videoFunc = std::bind(&WOpenGLWidget::slotReceiveVideoData, ui.openGLWidget,
		std::placeholders::_1);

	//��Ƶ��Ϣ
	VideoInfoFunc videoInfoFunc = std::bind(&MainWindow::slotReceiveVideoInfo, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

	//��ǰ���ŵ�ʱ��
	TimeFunc timeFunc = std::bind(&WCtrlBarWidget::slotSetTime, ui.ctrlBarWidget,
		std::placeholders::_1);

	m_demuxThread->clear();
	m_demuxThread->open(fileName.toStdString().c_str(), videoFunc, videoInfoFunc, timeFunc);
	m_demuxThread->setPause(false);

	if (!m_demuxThread->isRunning())
		m_demuxThread->start();
}
/**
 * ��һ��.
 * 
 */
void MainWindow::slotBackward()
{
	QString fileName = m_animationWidget->getNextPlayFileName(false);
	if(!fileName.isEmpty())
		slotShowVideo(fileName);
}

/**
 * ��һ��.
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
