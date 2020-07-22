#include "zmainui.h"
#include <QPainter>
#include <QTime>
#include <QDebug>
#include "zgblpara.h"
#include "zdialoghome.h"
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
    QObject::connect(this->m_ctrlBar,SIGNAL(ZSigHome()),this,SLOT(ZSlotHome()));
    QObject::connect(this->m_ctrlBar,SIGNAL(ZSigScan()),this,SLOT(ZSlotScan()));
    QObject::connect(this->m_ctrlBar,SIGNAL(ZSigCalibrate()),this,SLOT(ZSlotCalibrate()));

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
void ZMainUI::resizeEvent(QResizeEvent *event)
{
    this->m_ptCenter=QPoint(this->width()/2,this->height()/2);
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
        //draw center rectangle indicator.
        this->ZDrawRectangleIndicator(this->m_img);

        //draw bottom circle indicator.
        this->ZDrawCircleIndicator(this->m_img);

        //draw the image.
        painter.drawImage(QRectF(0,0,this->width(),this->height()),this->m_img);
    }

    //draw logs.
    if(0)
    {
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
    }

    //draw frame counter & fps.
    painter.setPen(QPen(Qt::red,2));
    QString strFrmCount=QString::number(/*this->m_iFrmCounter++*/this->getFps());
    //we keep 10 pixels space.
    QFont fontFrm=painter.font();
    fontFrm.setPixelSize(66);
    painter.setFont(fontFrm);
    QPoint pt;
    pt.setX(this->width()-painter.fontMetrics().width(strFrmCount)-10);
    pt.setY(painter.fontMetrics().height()+10);
    painter.drawText(pt,strFrmCount);


    //draw the Axies0,Axies1 on the bottom.
    QString strS0Pos=QString::number(this->m_iS0PosActVal);
    QString strS0Vel=QString::number(this->m_iS0ActVel);
    QString strS1Pos=QString::number(this->m_iS1PosActVal);
    QString strS1Vel=QString::number(this->m_iS1ActVel);
    QFont fontAxies=painter.font();
    fontAxies.setPixelSize(36);
    painter.setFont(fontAxies);
    //Slave 0 Position. (red color).
    //Slave 0 Velocity. (yellow).
    //Slave 1 Position. (red color).
    //Slave 1 Velocity. (yellow).
    //define rectangel.
    QRect rectS0Pos(this->width()-painter.fontMetrics().width(strS0Pos),///< x
                    this->height()-painter.fontMetrics().height()*4,///< y
                    painter.fontMetrics().width(strS0Pos),///<width
                    painter.fontMetrics().height());///<height
    QRect rectS0Vel(this->width()-painter.fontMetrics().width(strS0Vel),///< x
                    this->height()-painter.fontMetrics().height()*3,///< y
                    painter.fontMetrics().width(strS0Vel),///<width
                    painter.fontMetrics().height());///<height
    QRect rectS1Pos(this->width()-painter.fontMetrics().width(strS1Pos),///< x
                    this->height()-painter.fontMetrics().height()*2,///< y
                    painter.fontMetrics().width(strS1Pos),///<width
                    painter.fontMetrics().height());///<height
    QRect rectS1Vel(this->width()-painter.fontMetrics().width(strS1Vel),///< x
                    this->height()-painter.fontMetrics().height()*1,///< y
                    painter.fontMetrics().width(strS1Vel),///<width
                    painter.fontMetrics().height());///<height
    //draw Position with red color.
    painter.setPen(QPen(Qt::red,2));
    painter.drawText(rectS0Pos,strS0Pos);
    painter.drawText(rectS1Pos,strS1Pos);
    //draw velocity with yellow color.
    painter.setPen(QPen(Qt::yellow,2));
    painter.drawText(rectS0Vel,strS0Vel);
    painter.drawText(rectS1Vel,strS1Vel);

    //draw the track difference X&Y.
    if(gGblPara.m_bTrackingEnabled)
    {
        QString strTips;
        if(gGblPara.m_bTargetLocked)
        {
            strTips=QString("Locked:")+QString::number(gGblPara.m_iCostMSec);
        }else{
            strTips=QString("Tracking...");
        }
        QString strDiffXY=QString::number(gGblPara.m_trackDiffX)+","+QString::number(gGblPara.m_trackDiffY);
        QRect rectTips(0,///< x
                       this->height()-painter.fontMetrics().height()*2,///< y
                       painter.fontMetrics().width(strTips),///<width
                       painter.fontMetrics().height());///<height
        QRect rectDiffXY(0,///< x
                         this->height()-painter.fontMetrics().height()*1,///< y
                         painter.fontMetrics().width(strDiffXY),///<width
                         painter.fontMetrics().height());///<height
        painter.setPen(QPen(Qt::yellow,2));
        painter.drawText(rectTips,strTips);
        painter.drawText(rectDiffXY,strDiffXY);
    }
}
void ZMainUI::ZDrawRectangleIndicator(QImage &img)
{
    QPainter p;
    //p.setRenderHints(QPainter::Antialiasing,true);
    p.begin(&img);
    //draw a rectangle indicator.
    //    ___       ___
    //   |             |
    //
    //
    //   |             |
    //    ___       ___
    p.save();
    //move the (0,0) to the center of image.
    p.translate(img.width()/2,img.height()/2);
    p.setPen(QPen(Qt::green,4));
    p.drawLine(QPointF(70,-50),QPointF(100,-50));
    p.drawLine(QPointF(100,-50),QPointF(100,-20));
    ///
    p.drawLine(QPointF(70,50),QPointF(100,50));
    p.drawLine(QPointF(100,50),QPointF(100,20));
    ///
    p.drawLine(QPointF(-70,-50),QPointF(-100,-50));
    p.drawLine(QPointF(-100,-50),QPointF(-100,-20));
    ///
    p.drawLine(QPointF(-70,50),QPointF(-100,50));
    p.drawLine(QPointF(-100,50),QPointF(-100,20));
    //draw a half-tranparent mask and red color center point.
    p.setPen(Qt::NoPen);
    p.setBrush(QBrush(QColor(0,255,0,20)));
    //draw a ellipse with x radius=100,y radius=50.
    p.drawEllipse(QPoint(0,0),100,50);
    p.setBrush(QBrush(QColor(255,0,0,255)));
    p.drawEllipse(QPoint(0,0),6,6);
    p.restore();

}
void ZMainUI::ZDrawCircleIndicator(QImage &img)
{
    QPainter p;
    //p.setRenderHints(QPainter::Antialiasing,true);
    p.begin(&img);

    //draw the big scale.
    p.save();
    p.translate(img.width()/2,img.height());
    p.setPen(QPen(Qt::white,4));
    //we draw 9 lines in (180-18-18) degree.
    //so,(180-18-18) degreen /9=16 degree.
    for(int i=0;i<13;i++)
    {
        p.drawLine(QPointF(180,0),QPointF(200,0));
        p.rotate(-15);
    }
    p.restore();

    //draw the small scale.
    p.save();
    p.translate(img.width()/2,img.height());
    p.setPen(QPen(Qt::white,2));
    p.rotate(-7.5);
    p.drawLine(QPointF(190,0),QPointF(200,0));
    for(int i=0;i<11;i++)
    {
        p.rotate(-15);
        p.drawLine(QPointF(190,0),QPointF(200,0));
    }
    p.restore();

    //draw a arrow.
    p.save();
    p.translate(img.width()/2,img.height());
    p.setPen(QPen(Qt::white,2));
    p.setBrush(Qt::white);
    p.drawEllipse(QPoint(0,0),20,20);
    QPointF pt[]={{-6,0},{0,-150},{6,0}};
    p.rotate(30);
    p.drawConvexPolygon(pt,3);
    p.restore();
    p.end();
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
#if 0
    this->m_ptNew=event->pos();

    //map image pixel coordinate to physical motor move increasement.
    //y=kx+b for axis0.
    float kX=100.0;
    float kBx=0.0;
    //y=kx+b for axis1.
    float kY=100.0;
    float kBy=0.0;

    //image pixel diff.
    qint32 nPixDiffX=this->m_ptNew.x()-this->m_ptCenter.x();
    qint32 nPixDiffY=this->m_ptNew.y()-this->m_ptCenter.y();

    //map pixel diff to motor move diff.
    //set new motor move diff.
    gGblPara.m_moveDiffX=kX*nPixDiffX+kBx;
    gGblPara.m_moveDiffY=kY*nPixDiffY+kBy;
    qDebug("newMove:%d,%d\n",gGblPara.m_moveDiffX,gGblPara.m_moveDiffY);
#endif
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
    switch(gGblPara.m_iStepMode)
    {
    case 0:
        gGblPara.m_moveDiffX=+8000;
        break;
    case 1:
        gGblPara.m_moveDiffX=+1000;
        break;
    case 2:
        gGblPara.m_moveDiffX=+100;
        break;
    default:
        break;
    }
}
void ZMainUI::ZSlotMoveToRight()
{
    switch(gGblPara.m_iStepMode)
    {
    case 0:
        gGblPara.m_moveDiffX=-8000;
        break;
    case 1:
        gGblPara.m_moveDiffX=-1000;
        break;
    case 2:
        gGblPara.m_moveDiffX=-100;
        break;
    default:
        break;
    }
}
void ZMainUI::ZSlotMoveToUp()
{
    switch(gGblPara.m_iStepMode)
    {
    case 0:
        gGblPara.m_moveDiffY=+8000;
        break;
    case 1:
        gGblPara.m_moveDiffY=+1000;
        break;
    case 2:
        gGblPara.m_moveDiffY=+100;
        break;
    default:
        break;
    }
}
void ZMainUI::ZSlotMoveToDown()
{
    switch(gGblPara.m_iStepMode)
    {
    case 0:
        gGblPara.m_moveDiffY=-8000;
        break;
    case 1:
        gGblPara.m_moveDiffY=-1000;
        break;
    case 2:
        gGblPara.m_moveDiffY=-100;
        break;
    default:
        break;
    }
}
void ZMainUI::ZSlotHome()
{
    ZDialogHome diaHome(this);
    diaHome.setGeometry(0,0,600,300);
    diaHome.show();
}
void ZMainUI::ZSlotScan()
{
    //map image pixel coordinate to physical motor move increasement.
    //y=kx+b for axis0.
    float kX=1000.0;
    float kBx=0.0;
    //y=kx+b for axis1.
    float kY=100.0;
    float kBy=0.0;

    //map pixel diff to motor move diff.
    //set new motor move diff.
    gGblPara.m_moveDiffX=(kX*gGblPara.m_trackDiffX+kBx);
    gGblPara.m_moveDiffY=(kY*gGblPara.m_trackDiffY+kBy);
}
void ZMainUI::ZSlotCalibrate()
{

}
qint32 ZMainUI::getFps()
{
    static qint32 iFps=0;
    static qint32 iLastMSec=QTime::currentTime().msecsSinceStartOfDay();
    static qint32 iFrameCount=0;

    ++iFrameCount;
    qint32 iNowMSec=QTime::currentTime().msecsSinceStartOfDay();
    if(iNowMSec-iLastMSec>1000)
    {
        iFps=iFrameCount;
        iFrameCount=0;
        iLastMSec=iNowMSec;
    }
    return iFps;
}
