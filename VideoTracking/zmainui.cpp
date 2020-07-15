#include "zmainui.h"
#include <QPainter>
#include <QDebug>
#include "zgblpara.h"
ZMainUI::ZMainUI(QWidget *parent)
    : QWidget(parent)
{
    //slave 0 Position Actual Value.
    this->m_iS0PosActVal=0;
    //slave 0 Target Position.
    this->m_iS0TarPos=0;
    //slave 0 Actual Velocity.
    this->m_iS0ActVel=0;

    //slave 1 Position Actual Value.
    this->m_iS1PosActVal=0;
    //slave 1 Target Position.
    this->m_iS1TarPos=0;
    //slave 1 Actual Velocity.
    this->m_iS1ActVel=0;

    //reset frame counter.
    this->m_iFrmCounter=0;
}

ZMainUI::~ZMainUI()
{
    //the top ctrl bar.
    delete this->m_ctrlBar;
    delete this->m_hLayCtrlBar;
    delete this->m_dirBar;
    delete this->m_hLayDirBar;
    delete this->m_vLayMain;
}
bool ZMainUI::ZDoInit()
{
    this->m_ctrlBar=new ZCtrlBar;
    this->m_hLayCtrlBar=new QHBoxLayout;
    this->m_hLayCtrlBar->addStretch(1);
    this->m_hLayCtrlBar->addWidget(this->m_ctrlBar);
    this->m_hLayCtrlBar->addStretch(1);

    this->m_dirBar=new ZDirectionBar;
    this->m_hLayDirBar=new QHBoxLayout;
    this->m_hLayDirBar->addStretch(1);
    this->m_hLayDirBar->addWidget(this->m_dirBar);

    this->m_vLayMain=new QVBoxLayout;
    this->m_vLayMain->setContentsMargins(0,0,0,0);
    this->m_vLayMain->addLayout(this->m_hLayCtrlBar);
    this->m_vLayMain->addStretch(1);
    this->m_vLayMain->addLayout(this->m_hLayDirBar);
    this->m_vLayMain->addStretch(1);
    this->setLayout(this->m_vLayMain);

    //make connections.
    QObject::connect(this->m_dirBar,SIGNAL(ZSigLeft()),this,SLOT(ZSlotMoveToLeft()));
    QObject::connect(this->m_dirBar,SIGNAL(ZSigRight()),this,SLOT(ZSlotMoveToRight()));
    QObject::connect(this->m_dirBar,SIGNAL(ZSigUp()),this,SLOT(ZSlotMoveToUp()));
    QObject::connect(this->m_dirBar,SIGNAL(ZSigDown()),this,SLOT(ZSlotMoveToDown()));
    return true;
}
QSize ZMainUI::sizeHint() const
{
    return QSize(800,600);
}
void ZMainUI::ZSlotUpdateImg(const QImage &img)
{
    this->m_img=img;
    this->update();
}
void ZMainUI::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e);

    QPainter painter(this);
    if(this->m_img.isNull())
    {
        //if image is invalid,draw a black background.
        painter.fillRect(QRectF(0,0,this->width(),this->height()),Qt::black);
    }else{
        //draw the image.
        painter.drawImage(QRectF(0,0,this->width(),this->height()),this->m_img);
    }

    //draw logs.
    painter.setPen(QPen(Qt::red,2));
    QFont font=painter.font();
    font.setPixelSize(20);
    painter.setFont(font);
    QPointF ptLog(10,0);
    for(qint32 i=0;i<this->m_vecLog.size();i++)
    {
        ptLog.setY(ptLog.y()+painter.fontMetrics().height());
        painter.drawText(ptLog,this->m_vecLog.at(i).log);
    }

    //draw frame counter & fps.
    QString strFrmCount=QString::number(this->m_iFrmCounter++);
    //we keep 10 pixels space.
    QFont fontFrm=painter.font();
    fontFrm.setPixelSize(66);
    painter.setFont(fontFrm);
    QPoint pt;
    pt.setX(this->width()-painter.fontMetrics().width(strFrmCount)-10);
    pt.setY(painter.fontMetrics().height()+10);
    painter.drawText(pt,strFrmCount);


    //draw the Axies0,Axies1 on the bottom.
    QString strAxis0=QString::number(this->m_iS0PosActVal);
    QString strAxis1=QString::number(this->m_iS1PosActVal);
    QFont fontAxies=painter.font();
    fontAxies.setPixelSize(36);
    painter.setFont(fontAxies);
    painter.setPen(QPen(Qt::red,2));
    QRect rectAxis0(this->width()-painter.fontMetrics().width(strAxis0),///< x
                    this->height()-painter.fontMetrics().height()*2,///< y
                    painter.fontMetrics().width(strAxis0),///<width
                    painter.fontMetrics().height());///<height
    QRect rectAxis1(this->width()-painter.fontMetrics().width(strAxis1),///< x
                    this->height()-painter.fontMetrics().height()*1,///< y
                    painter.fontMetrics().width(strAxis1),///<width
                    painter.fontMetrics().height());///<height
    painter.drawText(rectAxis0,strAxis0);
    painter.drawText(rectAxis1,strAxis1);
}
void ZMainUI::ZSlotPDO(qint32 iSlave,qint32 iActPos,qint32 iTarPos,qint32 iActVel)
{
    switch(iSlave)
    {
    case 0:
        this->m_iS0PosActVal=iActPos;
        this->m_iS0TarPos=iTarPos;
        this->m_iS0ActVel=iActVel;
        break;
    case 1:
        this->m_iS1PosActVal=iActPos;
        this->m_iS1TarPos=iTarPos;
        this->m_iS1ActVel=iActVel;
        break;
    default:
        break;
    }
}
void ZMainUI::closeEvent(QCloseEvent *event)
{
    gGblPara.m_bExitFlag=true;
    QWidget::closeEvent(event);
}

void ZMainUI::mousePressEvent(QMouseEvent *event)
{
    this->m_ptNew=event->pos();
    gGblPara.m_pixelDiffX=this->m_ptNew.x()-this->m_ptCenter.x();
    gGblPara.m_pixelDiffY=this->m_ptNew.y()-this->m_ptCenter.y();
    gGblPara.m_pixelDiffX*=200;
    gGblPara.m_pixelDiffY*=200;
    //if (gGblPara.m_pixelDiffX>0), move torward to right.
    //if (gGblPara.m_pixelDiffX<0), move torward to left.

    //if (gGblPara.m_pixelDiffY>0), move torward to down.
    //if (gGblPara.m_pixelDiffY<0), move torward to up.

    //qDebug("diff x=%d,y=%d\n",gGblPara.m_pixelDiffX,gGblPara.m_pixelDiffY);
    this->ZSlotLog(false,QString("(%1,%2) -> (%3,%4),X:%5,Y:%6").arg(this->m_ptCenter.x()).arg(this->m_ptCenter.y()).arg(this->m_ptNew.x()).arg(this->m_ptNew.y()).arg(gGblPara.m_pixelDiffX).arg(gGblPara.m_pixelDiffY));
    QWidget::mousePressEvent(event);
}
void ZMainUI::mouseReleaseEvent(QMouseEvent *event)
{
    QWidget::mouseReleaseEvent(event);
}
void ZMainUI::ZSlotLog(bool bErrFlag,QString log)
{
    if(this->m_vecLog.size()>10)
    {
        this->m_vecLog.takeFirst();
    }
    ZVectorLog vecLog;
    vecLog.bErrFlag=bErrFlag;
    vecLog.log=log;
    this->m_vecLog.append(vecLog);
}

void ZMainUI::ZSlotMoveToLeft()
{
    gGblPara.m_pixelDiffX=+2000;
}
void ZMainUI::ZSlotMoveToRight()
{
    gGblPara.m_pixelDiffX=-2000;
}
void ZMainUI::ZSlotMoveToUp()
{
    gGblPara.m_pixelDiffY=+2000;
}
void ZMainUI::ZSlotMoveToDown()
{
    gGblPara.m_pixelDiffY=-2000;
}
