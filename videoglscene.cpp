#include "videoglscene.h"
#include "opencv2/opencv.hpp"
#include "fileinputdialog.h"

#include <QtGui>
#include <QtOpenGL>

VideoGlScene::VideoGlScene(QObject *parent) :
    QGraphicsScene(parent)
{
//    imageBuff=cv::imread("/home/sam/ULB/Fotos/Interf+bel.bmp",0);
//    imageBuff=cv::imread("/home/sam/GridCentralZone.png"); // color images seem to work to

    cam=cv::VideoCapture(0);

    //now make the control dialogues
    FileInputDialog* fileDialog = new FileInputDialog;

    connect(fileDialog,SIGNAL(buttonPressed(QString)),this,SLOT(loadImage(QString)));

    QGraphicsProxyWidget *proxy = new QGraphicsProxyWidget(0, Qt::Dialog);
    proxy->setWidget(fileDialog);
    addItem(proxy);

    QPointF pos(10, 10);
    foreach (QGraphicsItem *item, items()) {
        item->setFlag(QGraphicsItem::ItemIsMovable);
        item->setCacheMode(QGraphicsItem::DeviceCoordinateCache);

        const QRectF rect = item->boundingRect();
        item->setPos(pos.x() - rect.x(), pos.y() - rect.y());
        pos += QPointF(0, 10 + rect.height());
    }

}

void VideoGlScene::drawBackground(QPainter *painter, const QRectF &)
{
    if (cam.isOpened()) {
        cam >> imageBuff;
    }

    if (painter->paintEngine()->type() != QPaintEngine::OpenGL
            && painter->paintEngine()->type() != QPaintEngine::OpenGL2)
    {
        qWarning("OpenGLScene: drawBackground needs a QGLWidget to be set as viewport on the graphics view");
        return;
    }

    painter->beginNativePainting();

    //place image drawing code here
    int depth = imageBuff.depth();
    int cn = imageBuff.channels();
    GLenum format = GL_LUMINANCE;
    if (cn==3) {
        format = GL_BGR;
    } else if (cn==4) {
        format = GL_BGRA;
    }
    GLenum gldepth = GL_UNSIGNED_BYTE;
    if (depth==CV_16U) gldepth=GL_UNSIGNED_SHORT;

    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    GLuint mytex;
    glGenTextures(1,&mytex);
    glBindTexture(GL_TEXTURE_2D, mytex);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    if (imageBuff.isContinuous()) glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glTexImage2D(GL_TEXTURE_2D,0,cn,imageBuff.cols,imageBuff.rows,0,format,gldepth,imageBuff.data);

    //calculate projected size in order to keep aspect ratio intact
    int maxX=1024;
    int maxY=768;
    if (imageBuff.rows!=0) {
        double aspRatio=imageBuff.rows/(double)imageBuff.cols;
        double windowAspRatio=this->height()/(double)this->width();
        if (aspRatio>windowAspRatio) {
            // amount of rows is limiting factor
            maxY=this->height();
            maxX=maxY/aspRatio;
        } else {
            maxX=this->width();
            maxY=maxX*aspRatio;
        }
    }

    glBegin(GL_QUADS);

    glTexCoord2f(0.0,0.0); glVertex2f(0,0);
    glTexCoord2f(1.0,0.0); glVertex2f(maxX,0.0);
    glTexCoord2f(1.0,1.0); glVertex2f(maxX,maxY);
    glTexCoord2f(0.0,1.0); glVertex2f(0.0,maxY);
    /*
    glTexCoord2f(0.0,0.0); glVertex2f(0,0);
    glTexCoord2f(1.0,0.0); glVertex2f(imageBuff.cols,0.0);
    glTexCoord2f(1.0,1.0); glVertex2f(imageBuff.cols,imageBuff.rows);
    glTexCoord2f(0.0,1.0); glVertex2f(0.0,imageBuff.rows);
    */
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glDeleteTextures(1,&mytex);


    painter->endNativePainting();

    QTimer::singleShot(20, this, SLOT(update()));

}

void VideoGlScene::loadImage(QString imName)
{
    imageBuff=cv::imread(imName.toStdString(),0);
    update();
}



