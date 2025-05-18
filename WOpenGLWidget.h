#ifndef WOPENGLWIDGET_H
#define WOPENGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <QImage>
#include <QOpenGLShaderProgram>
#include <QOpenGLShader>
#include <QDebug>
#include <QImage>
#include <QMouseEvent>
#include <QSet>
#include <QPainter>
#include <QMutex>
#include <QOpenGLBuffer>

class OpenGLDisplayImpl
{
public:
	OpenGLDisplayImpl()
	{
		textureY = NULL;
		textureU = NULL;
		textureV = NULL;
		videoW = 0;
		videoH = 0;
	}

	unsigned char *buffer[3] = {0};

	QOpenGLTexture*         textureY;
	QOpenGLTexture*         textureU;
	QOpenGLTexture*         textureV;

	GLsizei                 videoW, videoH;
};

class WOpenGLWidget : public QOpenGLWidget,public QOpenGLFunctions
{
    Q_OBJECT
public:
    WOpenGLWidget(QWidget* parent = Q_NULLPTR);
	~WOpenGLWidget();
	
signals:
	void sigUpdate();

public:
	void slotOpenVideo(int width,int height);
	void slotReceiveVideoData(uint8_t* yuvBuffer);
	void clear();
	void deleteBuffer();

protected:
    virtual void initializeGL();
    virtual void paintGL();
    virtual void resizeGL(int w, int h);

private:
	void draw();

private:
    QOpenGLShaderProgram *m_program = nullptr;          //着色器程序

	QOpenGLBuffer VBO, EBO;

	OpenGLDisplayImpl *m_impl = nullptr;
	bool m_isShowVideo = false;
	QMutex m_mux;
};

#endif // WOPENGLWIDGET_H
