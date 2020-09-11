#include "zmainui.h"
#include <QPainter>
#include <QTime>
#include <QDebug>
#include "zgblpara.h"
#include "zdialoghome.h"
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/resource.h>
#include <time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>
ZMainUI::ZMainUI(ZDiffFIFO *fifoDIFF,QWidget *parent)
    : QWidget(parent)
{
    this->m_fifoDIFF=fifoDIFF;

    for(qint32 i=0;i<2;i++)
    {
        this->m_statusWord[i]=0;
        this->m_velocity[i]=0;
        this->m_position[i]=0;
    }

    //reset frame counter.
    this->m_iFrmCounter=0;

    this->m_bLocked=false;
    this->m_bSelectROI=false;


    this->m_diffAvailableNums=0;
    this->m_matAvailableNums=0;
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
    QObject::connect(this->m_ctrlBar,SIGNAL(ZSigModeChanged()),this,SLOT(ZSlotModeChanged()));

    QObject::connect(this->m_dirBar,SIGNAL(ZSigLeft()),this,SLOT(ZSlotMove2Left()));
    QObject::connect(this->m_dirBar,SIGNAL(ZSigRight()),this,SLOT(ZSlotMove2Right()));
    QObject::connect(this->m_dirBar,SIGNAL(ZSigUp()),this,SLOT(ZSlotMove2Up()));
    QObject::connect(this->m_dirBar,SIGNAL(ZSigDown()),this,SLOT(ZSlotMove2Down()));

    QObject::connect(&this->m_timerLost,SIGNAL(timeout()),this,SLOT(ZSlotLostTimeout()));
    return true;
}

QSize ZMainUI::sizeHint() const
{
    return QSize(800,600);
}
void ZMainUI::resizeEvent(QResizeEvent *event)
{
    //this->m_ptCenter=QPoint(this->width()/2,this->height()/2);
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
    painter.setRenderHints(QPainter::Antialiasing,true);
    if(this->m_img.isNull())
    {
        //if image is invalid,draw a black background.
        painter.fillRect(QRectF(0,0,this->width(),this->height()),Qt::black);
        painter.setPen(QPen(Qt::yellow,2));
        QFont font=painter.font();
        font.setPixelSize(60);
        painter.setFont(font);
        QString strTips("Video lost,check connection!");
        QRect rectTips((this->width()-painter.fontMetrics().width(strTips))/2,///< x
                       (this->height()-painter.fontMetrics().height())/2,///< y
                       painter.fontMetrics().width(strTips)*2,///<width
                       painter.fontMetrics().height());///<height
        painter.drawText(rectTips,strTips);
        return;
    }

    //draw on image first.
    QPainter p(&this->m_img);
    p.setRenderHints(QPainter::Antialiasing,true);

    //draw center rectangle indicator on image.
    this->ZDrawRectangleIndicator(p,this->m_img);

    //draw bottom circle indicator on image.
    this->ZDrawCircleIndicator(p,this->m_img);

    //draw split lines on image.
    //this->ZDrawSplitGrid(p,this->m_img);

    switch(gGblPara.m_appMode)
    {
    case Free_Mode:
    {
        QFont font=p.font();
        font.setPixelSize(40);
        p.setFont(font);
        p.setPen(QPen(Qt::yellow,4));

        //draw FreeMode.
        QPoint ptFree(0,0+p.fontMetrics().height());
        p.drawText(ptFree,QString("FREE"));
    }
        break;
    case SelectROI_Mode:
    {
        QFont font=p.font();
        font.setPixelSize(40);
        p.setFont(font);
        p.setPen(QPen(Qt::yellow,4));

        //draw SelectROI.
        QPoint ptROI(0,0+p.fontMetrics().height());
        p.drawText(ptROI,QString("SELECT ROI"));

        //draw a rectangle mask on the image.
        this->ZDrawROIMask(p,this->m_img);
    }
        break;
    case Track_Mode:
    {
        //(0,0,200,200):draw the ROI.
        //(0,200,200,200):Locked/Lost.
        //(0,400,200,200):cost millsec.

        //draw the selected ROI on the left-top corner for referencing.
        p.drawImage(QRect(0,0,200,200),this->m_initImg);
        //p.drawImage(QRect(0,0,600,600),this->m_initImg);

        QFont font=p.font();
        font.setPixelSize(40);
        p.setFont(font);
        p.setPen(QPen(Qt::yellow,4));
        if(this->m_bLocked)
        {
            //draw the locked rectangle.
            p.drawRect(this->m_rectLocked);

            QPoint ptLocked(0,200+p.fontMetrics().height());
            p.drawText(ptLocked,QString("Locked"));
            QPoint ptFps(0,ptLocked.y()+p.fontMetrics().height());
            p.drawText(ptFps,QString::number(gGblPara.m_iCostMSec)+"ms/"+QString::number(gGblPara.m_iFps)+"fps");
            //draw the diff x&y.
            QPoint ptDiff(0,ptFps.y()+p.fontMetrics().height());
            p.drawText(ptDiff,QString::number(this->m_diffX)+","+QString::number(this->m_diffY));
        }else{
            QPoint ptLost(0,200);
            p.drawText(ptLost,QString("Lost:")+QString::number(this->m_iLostTimeout));
        }
    }
        break;
    }

    //finally,draw the image.
    painter.drawImage(QRectF(0,0,this->width(),this->height()),this->m_img);


    //draw UI flush fps on right-top corner.
    painter.setPen(QPen(Qt::yellow,2));
    QFont fontFrm=painter.font();
    fontFrm.setPixelSize(50);
    painter.setFont(fontFrm);

    QString strFps=QString::number(this->getFps())+QString("FPS");
    QPoint ptFps;
    ptFps.setX(this->width()-painter.fontMetrics().width(strFps)-10);
    ptFps.setY(painter.fontMetrics().height()+10);
    painter.drawText(ptFps,strFps);

    //draw diff FIFO available nums.
    QString strDiffAvailableNums=QString("DF:")+QString::number(this->m_diffAvailableNums);
    QPoint ptDiffAvailable;
    ptDiffAvailable.setX(this->width()-painter.fontMetrics().width(strDiffAvailableNums)-10);
    ptDiffAvailable.setY(painter.fontMetrics().height()*2+10);
    painter.drawText(ptDiffAvailable,strDiffAvailableNums);
    //draw cvMat FIFO available nums.
    QString strMatAvailableNums=QString("MF:")+QString::number(this->m_matAvailableNums);
    QPoint ptMatAvailable;
    ptMatAvailable.setX(this->width()-painter.fontMetrics().width(strMatAvailableNums)-10);
    ptMatAvailable.setY(painter.fontMetrics().height()*3+10);
    painter.drawText(ptMatAvailable,strMatAvailableNums);

    //draw the Axies0,Axies1 on the bottom.
    QString strS0Pos=QString::number(this->m_position[0]);
    QString strS0Vel=QString::number(this->m_velocity[0]);
    QString strS1Pos=QString::number(this->m_position[1]);
    QString strS1Vel=QString::number(this->m_velocity[1]);
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

    //draw dynamic logs.
    if(1)
    {
        QFont font=painter.font();
        font.setPixelSize(26);
        painter.setFont(font);

        // 10 lines log.
        //draw from bottom to top.
        QPointF ptLog(10,this->height()-painter.fontMetrics().height());
        for(qint32 i=this->m_vecLog.size()-1;i>=0;i--)
        {
            if(this->m_vecLog.at(i).bErrFlag)
            {
                painter.setPen(QPen(Qt::red,2));
            }else{
                painter.setPen(QPen(Qt::white,2));
            }
            ptLog.setY(ptLog.y()-painter.fontMetrics().height());
            painter.drawText(ptLog,this->m_vecLog.at(i).log);
        }
    }
}
void ZMainUI::ZDrawRectangleIndicator(QPainter &p,QImage &img)
{
    //draw a rectangle indicator.
    //    ___   |    ___
    //   |      |      |
    //          |
    //--------------------------
    //          |
    //   |      |      |
    //    ___   |   ___
    p.save();
    //move the (0,0) to the center of image.
    p.translate(img.width()/2,img.height()/2);

    //set different color.
    if(gGblPara.m_appMode==Track_Mode)
    {
        if(this->m_diffX>=-5 && this->m_diffX<=5 && this->m_diffY>=-5 && this->m_diffY<=5)
        {
            p.setPen(QPen(Qt::green,2));
        }else if(this->m_diffX>=-20 && this->m_diffX<=20 && this->m_diffY>=-20 && this->m_diffY<=20)
        {
            p.setPen(QPen(Qt::yellow,2));
        }else{
            p.setPen(QPen(Qt::red,2));
        }

        //draw diff x&y.
        QFont font=p.font();
        font.setPixelSize(30);
        p.setFont(font);
        if(this->m_diffX>0)
        {
            //draw diff x on the right part.
            QString strDiffX=QString::number(this->m_diffX);
            QPoint pt(100+10,-p.fontMetrics().height());
            p.drawText(pt,strDiffX);
        }else if(this->m_diffX<0)
        {
            //draw diff x on the left part.
            QString strDiffX=QString::number(this->m_diffX);
            QPoint pt(-100-10-p.fontMetrics().width(strDiffX),-p.fontMetrics().height());
            p.drawText(pt,strDiffX);
        }
        if(this->m_diffY>0)
        {
            //draw diff y on the top part.
            QString strDiffY=QString::number(this->m_diffY);
            QPoint pt(0,-50-10-p.fontMetrics().height());
            p.drawText(pt,strDiffY);
        }else if(this->m_diffY<0)
        {
            //draw diff x on the bottom part.
            QString strDiffY=QString::number(this->m_diffY);
            QPoint pt(0,50+10);
            p.drawText(pt,strDiffY);
        }
    }else{
        //Free mode & Select ROI mode.
        p.setPen(QPen(Qt::green,2));
    }

    //draw the cross + two lines.
    p.drawLine(QPointF(-img.width()/2,0),QPointF(img.width()/2,0));
    p.drawLine(QPointF(0,-img.height()/2),QPointF(0,img.height()/2));

    //draw the rectangle.
    QPen pen=p.pen();
    pen.setWidth(4);
    p.setPen(pen);
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
void ZMainUI::ZDrawCircleIndicator(QPainter &p,QImage &img)
{
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
}
void ZMainUI::ZDrawSplitGrid(QPainter &p,QImage &img)
{
    //draw split grid to 3x3 sub-area.
    p.save();
    p.translate(0,0);
    int xSubStep=img.width()/3;
    int ySubStep=img.height()/3;
    int iStretchSize=6;
    const QLineF lines[8]={
        //the left-top area.
        {QPoint(0,ySubStep*2-iStretchSize),QPoint(xSubStep*2-iStretchSize,ySubStep*2-iStretchSize)},///<
        {QPoint(xSubStep*2-iStretchSize,ySubStep*2-iStretchSize),QPoint(xSubStep*2-iStretchSize,0)},///<
        //the right-top area.
        {QPoint(xSubStep*3+iStretchSize,ySubStep*2+iStretchSize),QPoint(xSubStep*1+iStretchSize,ySubStep*2+iStretchSize)},///<
        {QPoint(xSubStep*1+iStretchSize,ySubStep*2+iStretchSize),QPoint(xSubStep*1+iStretchSize,0)},///,
        //the left-bottom area.
        {QPoint(0,ySubStep*1+iStretchSize),QPoint(xSubStep*2+iStretchSize,ySubStep*1+iStretchSize)},///<
        {QPoint(xSubStep*2+iStretchSize,ySubStep*1+iStretchSize),QPoint(xSubStep*2+iStretchSize,ySubStep*3+iStretchSize)},///,
        //the right-bottom area.
        {QPoint(xSubStep*3-iStretchSize,ySubStep*1-iStretchSize),QPoint(xSubStep*1-iStretchSize,ySubStep*1-iStretchSize)},///<
        {QPoint(xSubStep*1-iStretchSize,ySubStep*1-iStretchSize),QPoint(xSubStep*1-iStretchSize,ySubStep*3-iStretchSize)},///,
    };
    p.setPen(QPen(Qt::white,4));
    p.drawLines(lines,2);
    p.setPen(QPen(Qt::red,4));
    p.drawLines(lines+2,2);
    p.setPen(QPen(Qt::green,4));
    p.drawLines(lines+4,2);
    p.setPen(QPen(Qt::blue,4));
    p.drawLines(lines+6,2);
    p.restore();
}
void ZMainUI::ZDrawROIMask(QPainter &p,QImage &img)
{
    //draw current position on left-bottom corner.
#if 1
    p.setPen(QPen(Qt::red,2));
    QString strROI;
    strROI+="("+QString::number(this->m_ptStart.x())+","+QString::number(this->m_ptStart.y())+")";
    strROI+="\n";
    strROI+="("+QString::number(this->m_ptEnd.x())+","+QString::number(this->m_ptEnd.y())+")";
    strROI+="\n";
    strROI+=QString::number(abs(this->m_ptStart.x()-this->m_ptEnd.x()))+"*"+QString::number(abs(this->m_ptStart.y()-this->m_ptEnd.y()));
    QFont font=p.font();
    font.setPixelSize(66);
    p.setFont(font);
    QRect rectStrROI;
    rectStrROI.setX(0);
    rectStrROI.setY(this->height()-p.fontMetrics().height()*3);
    rectStrROI.setWidth(this->width());
    rectStrROI.setHeight(this->height()-rectStrROI.y());
    p.drawText(rectStrROI,strROI);
#endif
    //draw the mask.
    p.save();
    p.fillRect(QRect(this->m_ptStart,this->m_ptEnd),QColor(255,0,0,100));
    p.restore();
}
void ZMainUI::ZSlotPDO(int servoID,int statusWord,int velocity,int position)
{
    switch(servoID)
    {
    case 0:
        this->m_statusWord[0]=statusWord;
        this->m_velocity[0]=velocity;
        this->m_position[0]=position;
        break;
    case 1:
        this->m_statusWord[1]=statusWord;
        this->m_velocity[1]=velocity;
        this->m_position[1]=position;
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
    switch(gGblPara.m_appMode)
    {
    case Free_Mode:
        break;
    case SelectROI_Mode:
        this->m_bSelectROI=true;
        this->m_ptStart=event->pos();
        this->m_ptEnd=this->m_ptStart;
        break;
    case Track_Mode:
        break;
    default:
        break;
    }
    QWidget::mousePressEvent(event);
}
void ZMainUI::mouseMoveEvent(QMouseEvent *event)
{
    switch(gGblPara.m_appMode)
    {
    case Free_Mode:
        break;
    case SelectROI_Mode:
    {
        this->m_ptEnd=event->pos();

        //update the ROI coordinate.
        if((this->m_ptEnd.x()>this->m_ptStart.x())&&(this->m_ptEnd.y()>this->m_ptStart.y()))
        {
            //drag from left-top to right-bottom.
            gGblPara.m_rectROI.x=this->m_ptStart.x();
            gGblPara.m_rectROI.y=this->m_ptStart.y();
            gGblPara.m_rectROI.width=abs(this->m_ptStart.x()-this->m_ptEnd.x());
            gGblPara.m_rectROI.height=abs(this->m_ptStart.y()-this->m_ptEnd.y());
        }else if((this->m_ptEnd.x()<this->m_ptStart.x())&&(this->m_ptEnd.y()<this->m_ptStart.y()))
        {
            //drag from right-bottom to left-top.
            gGblPara.m_rectROI.x=this->m_ptEnd.x();
            gGblPara.m_rectROI.y=this->m_ptEnd.y();
            gGblPara.m_rectROI.width=abs(this->m_ptStart.x()-this->m_ptEnd.x());
            gGblPara.m_rectROI.height=abs(this->m_ptStart.y()-this->m_ptEnd.y());
        }else if((this->m_ptEnd.x()>this->m_ptStart.x())&&(this->m_ptEnd.y()<this->m_ptStart.y()))
        {
            //drag from left-bottom to right-top.
            gGblPara.m_rectROI.x=this->m_ptStart.x();
            gGblPara.m_rectROI.y=this->m_ptEnd.y();
            gGblPara.m_rectROI.width=abs(this->m_ptStart.x()-this->m_ptEnd.x());
            gGblPara.m_rectROI.height=abs(this->m_ptStart.y()-this->m_ptEnd.y());
        }else if((this->m_ptEnd.x()<this->m_ptStart.x())&&(this->m_ptEnd.y()>this->m_ptStart.y()))
        {
            //drag from right-top to left-bottom.
            gGblPara.m_rectROI.x=this->m_ptEnd.x();
            gGblPara.m_rectROI.y=this->m_ptStart.y();
            gGblPara.m_rectROI.width=abs(this->m_ptStart.x()-this->m_ptEnd.x());
            gGblPara.m_rectROI.height=abs(this->m_ptStart.y()-this->m_ptEnd.y());
        }else{
            gGblPara.m_rectROI.x=0;
            gGblPara.m_rectROI.y=0;
            gGblPara.m_rectROI.width=200;
            gGblPara.m_rectROI.height=200;
        }
    }
        break;
    case Track_Mode:
        break;
    }
    QWidget::mouseMoveEvent(event);
}
void ZMainUI::mouseReleaseEvent(QMouseEvent *event)
{
    switch(gGblPara.m_appMode)
    {
    case Free_Mode:
        break;
    case SelectROI_Mode:
        this->m_bSelectROI=false;
        break;
    case Track_Mode:
        break;
    default:
        break;
    }
    QWidget::mouseReleaseEvent(event);
}
void ZMainUI::ZSlotModeChanged()
{
    switch(gGblPara.m_appMode)
    {
    case Free_Mode:
        break;
    case SelectROI_Mode:
        break;
    case Track_Mode:

        break;
    }
}
void ZMainUI::ZSlotLocked(bool bLocked,QRect rect)
{
    this->m_bLocked=bLocked;
    this->m_rectLocked=rect;
    if(this->m_bLocked)
    {
        if(this->m_timerLost.isActive())
        {
            this->m_timerLost.stop();
        }
    }else{
        if(!this->m_timerLost.isActive())
        {
            this->m_iLostTimeout=0;
            this->m_timerLost.start(1000);
        }
    }
}
void ZMainUI::ZSlotInitBox(const QImage &img)
{
    this->m_initImg=img;
}
void ZMainUI::ZSlotDiffXY(int diffX,int diffY)
{
    this->m_diffX=diffX;
    this->m_diffY=diffY;
}
void ZMainUI::ZSlotDiffAvailable(int nums)
{
    this->m_diffAvailableNums=nums;
}
void ZMainUI::ZSlotMatAvailable(int nums)
{
    this->m_matAvailableNums=nums;
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

void ZMainUI::ZSlotMove2Left()
{
    ZDiffResult ret;
    ret.move_mode=PPM_POSITION_RELATIVE;
    switch(gGblPara.m_iStepMode)
    {
    case 0:
        ret.diff_x=-10000;
        ret.diff_y=0;
        break;
    case 1:
        ret.diff_x=-3000;
        ret.diff_y=0;
        break;
    case 2:
        ret.diff_x=-1000;
        ret.diff_y=0;
        break;
    default:
        break;
    }
    this->m_fifoDIFF->ZTryPutDiff(ret,100);
}
void ZMainUI::ZSlotMove2Right()
{
    ZDiffResult ret;
    ret.move_mode=PPM_POSITION_RELATIVE;
    switch(gGblPara.m_iStepMode)
    {
    case 0:
        ret.diff_x=+10000;
        ret.diff_y=0;
        break;
    case 1:
        ret.diff_x=+3000;
        ret.diff_y=0;
        break;
    case 2:
        ret.diff_x=+1000;
        ret.diff_y=0;
        break;
    default:
        break;
    }
    this->m_fifoDIFF->ZTryPutDiff(ret,100);
}
void ZMainUI::ZSlotMove2Up()
{
    ZDiffResult ret;
    ret.move_mode=PPM_POSITION_RELATIVE;
    switch(gGblPara.m_iStepMode)
    {
    case 0:
        ret.diff_x=0;
        ret.diff_y=+10000;
        break;
    case 1:
        ret.diff_x=0;
        ret.diff_y=+3000;
        break;
    case 2:
        ret.diff_x=0;
        ret.diff_y=+1000;
        break;
    default:
        break;
    }
    this->m_fifoDIFF->ZTryPutDiff(ret,100);
}
void ZMainUI::ZSlotMove2Down()
{
    ZDiffResult ret;
    ret.move_mode=PPM_POSITION_RELATIVE;
    switch(gGblPara.m_iStepMode)
    {
    case 0:
        ret.diff_x=0;
        ret.diff_y=-10000;
        break;
    case 1:
        ret.diff_x=0;
        ret.diff_y=-3000;
        break;
    case 2:
        ret.diff_x=0;
        ret.diff_y=-1000;
        break;
    default:
        break;
    }
    this->m_fifoDIFF->ZTryPutDiff(ret,100);
}
void ZMainUI::ZSlotHome()
{
    //    ZDialogHome diaHome(this);
    //    diaHome.setGeometry(0,0,600,300);
    //    diaHome.show();

    ZDiffResult ret;
    ret.move_mode=PPM_POSITION_ABSOLUTE;
    switch(gGblPara.m_iStepMode)
    {
    case 0:
        ret.diff_x=0;
        ret.diff_y=0;
        break;
    case 1:
        ret.diff_x=0;
        ret.diff_y=0;
        break;
    case 2:
        ret.diff_x=0;
        ret.diff_y=0;
        break;
    default:
        break;
    }
    this->m_fifoDIFF->ZTryPutDiff(ret,100);
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
void ZMainUI::ZSlotLostTimeout()
{
    this->m_iLostTimeout++;
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
