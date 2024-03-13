#include "WOpenGLWidget.h"
#include <QDebug>
#include <QTimer>
#include <QElapsedTimer>

static float vertices[] = {
//     ---- 位置 ----    - 纹理坐标 -
     1.0f,  1.0f, 0.0f, 1.0f, 1.0f,   // 右上
     1.0f, -1.0f, 0.0f, 1.0f, 0.0f,   // 右下
    -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,   // 左下
    -1.0f,  1.0f, 0.0f, 0.0f, 1.0f    // 左上
};

static unsigned int indices[] = {
    0, 1, 3,
    1, 2, 3
};


WOpenGLWidget::WOpenGLWidget(QWidget* parent)
    : QOpenGLWidget(parent)
	, m_impl(new OpenGLDisplayImpl)
	, EBO(QOpenGLBuffer::IndexBuffer)
{
}

WOpenGLWidget::~WOpenGLWidget()
{
	clear();

	if (m_impl->textureY)
	{
		m_impl->textureY->destroy();
	}

	if (m_impl->textureU)
	{
		m_impl->textureU->destroy();
	}

	if (m_impl->textureV)
	{
		m_impl->textureV->destroy();
	}

	delete m_impl;
	m_impl = nullptr;
}

void WOpenGLWidget::slotOpenVideo(int width, int height)
{
	qDebug() << "slotOpenVideo";
	m_mux.lock();
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	m_isShowVideo = false;
	m_impl->videoW = width;
	m_impl->videoH = height;

	deleteBuffer();

	resize(this->width(), this->height());

	m_isShowVideo = true;
	m_mux.unlock();
}

void WOpenGLWidget::slotReceiveVideoData(uint8_t* yuvBuffer)
{
	if (!m_impl)
		return;

	m_mux.lock();
	if(!m_impl->buffer[0])
		m_impl->buffer[0] = new unsigned char[m_impl->videoW * m_impl->videoH];//y

	if (!m_impl->buffer[1])
		m_impl->buffer[1] = new unsigned char[m_impl->videoW * m_impl->videoH / 4];//u

	if (!m_impl->buffer[2])
		m_impl->buffer[2] = new unsigned char[m_impl->videoW * m_impl->videoH / 4];//v

	memcpy(m_impl->buffer[0], yuvBuffer, m_impl->videoW * m_impl->videoH);
	memcpy(m_impl->buffer[1], yuvBuffer + m_impl->videoW * m_impl->videoH,
		m_impl->videoW * m_impl->videoH / 4);
	memcpy(m_impl->buffer[2], yuvBuffer + 5 * m_impl->videoW * m_impl->videoH / 4,
		m_impl->videoW * m_impl->videoH / 4);

	update();
    m_mux.unlock();
}


void WOpenGLWidget::clear()
{
	m_mux.lock();

	deleteBuffer();
	m_isShowVideo = false;
	m_mux.unlock();
}

void WOpenGLWidget::deleteBuffer()
{
	if (m_impl)
	{
		if (m_impl->buffer[0]) {
			delete m_impl->buffer[0];
			m_impl->buffer[0] = nullptr;
		}

		if (m_impl->buffer[1]) {
			delete m_impl->buffer[1];
			m_impl->buffer[1] = nullptr;
		}

		if (m_impl->buffer[2]) {
			delete m_impl->buffer[2];
			m_impl->buffer[2] = nullptr;
		}
	}
}

void WOpenGLWidget::initializeGL()
{
	m_mux.lock();
    initializeOpenGLFunctions();

    m_program = new QOpenGLShaderProgram();
    m_program->addShaderFromSourceFile(QOpenGLShader::Vertex,":/shaders/shaders/shapes.vert");
    m_program->addShaderFromSourceFile(QOpenGLShader::Fragment,":/shaders/shaders/shapes.frag");
	bool success = m_program->link();
    if (!success)
        qDebug() << "ERR:" << m_program->log();


	VBO.create();
    EBO.create();

	VBO.bind();
    EBO.bind();

    VBO.allocate(vertices, 20 * sizeof(float));
    EBO.allocate(indices, 6 * sizeof(unsigned int));

	m_impl->textureY = new QOpenGLTexture(QOpenGLTexture::Target2D);
	m_impl->textureY->create();
	m_impl->textureY->setMinificationFilter(QOpenGLTexture::Nearest);
	m_impl->textureY->setMinificationFilter(QOpenGLTexture::Linear);
	m_impl->textureY->setWrapMode(QOpenGLTexture::ClampToEdge);

	m_impl->textureU = new QOpenGLTexture(QOpenGLTexture::Target2D);
	m_impl->textureU->create();
	m_impl->textureU->setMinificationFilter(QOpenGLTexture::Nearest);
	m_impl->textureU->setMinificationFilter(QOpenGLTexture::Linear);
	m_impl->textureU->setWrapMode(QOpenGLTexture::ClampToEdge);

	m_impl->textureV = new QOpenGLTexture(QOpenGLTexture::Target2D);
	m_impl->textureV->create();
	m_impl->textureV->setMinificationFilter(QOpenGLTexture::Nearest);
	m_impl->textureV->setMinificationFilter(QOpenGLTexture::Linear);
	m_impl->textureV->setWrapMode(QOpenGLTexture::Repeat);

	m_mux.unlock();

	// 启动定时器
    QTimer *ti = new QTimer(this);
    connect(ti, &QTimer::timeout, this, [=] {
        update();
    });
    ti->start(100);
}

void WOpenGLWidget::paintGL()
{
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_mux.lock();
	if (m_isShowVideo)
	{		
        VBO.bind();
        EBO.bind();
		m_program->bind();

		m_impl->textureY->bind(0);
		m_impl->textureU->bind(1);
		m_impl->textureV->bind(2);

		m_program->setUniformValue("tex_y", 0);
		m_program->setUniformValue("tex_u", 1);
		m_program->setUniformValue("tex_v", 2);

        int vertexLocation = m_program->attributeLocation("position");
        m_program->enableAttributeArray(vertexLocation);
        m_program->setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3, 5 * sizeof(float));

        int texcoordLocation = m_program->attributeLocation("texCoord");
        m_program->enableAttributeArray(texcoordLocation);
        m_program->setAttributeBuffer(texcoordLocation, GL_FLOAT, 3 * sizeof(float), 2, 5 * sizeof(float));

        //激活纹理单元0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_impl->textureY->textureId());
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_impl->videoW,
            m_impl->videoH, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, m_impl->buffer[0]);
        //设置纹理环绕方式
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        //激活纹理单元1
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_impl->textureU->textureId());
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_impl->videoW / 2, m_impl->videoH / 2
            , 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, m_impl->buffer[1]);
        //设置纹理环绕方式
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        //激活纹理单元2
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, m_impl->textureV->textureId());
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_impl->videoW / 2, m_impl->videoH / 2
            , 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, m_impl->buffer[2]);
        //设置纹理环绕方式
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glDrawElements(GL_TRIANGLES,6, GL_UNSIGNED_INT,0);

        if(m_impl->textureY)
            m_impl->textureY->release();
        if(m_impl->textureU)
            m_impl->textureU->release();
        if(m_impl->textureV)
            m_impl->textureV->release();

        m_program->release();
	}
	m_mux.unlock();
}

void WOpenGLWidget::resizeGL(int w, int h)
{
	// 设置视口
	//glViewport(0, 0, w, h);
}
