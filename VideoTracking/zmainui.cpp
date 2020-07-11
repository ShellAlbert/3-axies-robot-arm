#include "zmainui.h"
#include <QPainter>
#include <QDebug>
#include "zgblpara.h"
ZMainUI::ZMainUI(QWidget *parent)
    : QWidget(parent)
{
}

ZMainUI::~ZMainUI()
{
    //slave 0 Position Actual Value.
    delete this->m_llS0PosActVal;
    delete this->m_leS0PosActVal;
    //slave 0 Target Position.
    delete this->m_llS0TarPos;
    delete this->m_leS0TarPos;
    //slave 0 Actual Velocity.
    delete this->m_llS0ActVel;
    delete this->m_leS0ActVel;

    //slave 1 Position Actual Value.
    delete this->m_llS1PosActVal;
    delete this->m_leS1PosActVal;
    //slave 1 Target Position.
    delete this->m_llS1TarPos;
    delete this->m_leS1TarPos;
    //slave 1 Actual Velocity.
    delete this->m_llS1ActVel;
    delete this->m_leS1ActVel;

    delete this->m_gLayLft;

    //left,right,up,down control.
    delete this->m_tbMoveLft;
    delete this->m_tbMoveRht;
    delete this->m_tbMoveUp;
    delete this->m_tbMoveDn;
    delete this->m_gLayoutMove;
    delete this->m_vLayRht;

    //the main layout.
    delete this->m_hLayoutTop;
}
bool ZMainUI::ZDoInit()
{

    //slave 0 Position Actual Value.
    this->m_llS0PosActVal=new QLabel(tr("CurPos"));
    this->m_leS0PosActVal=new QLineEdit;
    this->m_leS0PosActVal->setFocusPolicy(Qt::NoFocus);
    this->m_leS0PosActVal->setText("0");

    //slave 0 Target Position.
    this->m_llS0TarPos=new QLabel(tr("TarPos"));
    this->m_leS0TarPos=new QLineEdit;
    this->m_leS0TarPos->setFocusPolicy(Qt::NoFocus);
    this->m_leS0TarPos->setText("0");

    //slave 0 Actual Velocity.
    this->m_llS0ActVel=new QLabel(tr("ActVel"));
    this->m_leS0ActVel=new QLineEdit;
    this->m_leS0ActVel->setFocusPolicy(Qt::NoFocus);
    this->m_leS0ActVel->setText("0");

    //slave 1 Position Actual Value.
    this->m_llS1PosActVal=new QLabel(tr("CurPos"));
    this->m_leS1PosActVal=new QLineEdit;
    this->m_leS1PosActVal->setFocusPolicy(Qt::NoFocus);
    this->m_leS1PosActVal->setText("0");

    //slave 1 Target Position.
    this->m_llS1TarPos=new QLabel(tr("TarPos"));
    this->m_leS1TarPos=new QLineEdit;
    this->m_leS1TarPos->setFocusPolicy(Qt::NoFocus);
    this->m_leS1TarPos->setText("0");

    //slave 1 Actual Velocity.
    this->m_llS1ActVel=new QLabel(tr("ActVel"));
    this->m_leS1ActVel=new QLineEdit;
    this->m_leS1ActVel->setFocusPolicy(Qt::NoFocus);
    this->m_leS1ActVel->setText("0");

//    this->m_gLayLft=new QGridLayout;
//    this->m_gLayLft->addWidget(this->m_llS0PosActVal,0,0,1,1);
//    this->m_gLayLft->addWidget(this->m_leS0PosActVal,0,1,1,1);

//    this->m_gLayLft->addWidget(this->m_llS0TarPos,1,0,1,1);
//    this->m_gLayLft->addWidget(this->m_leS0TarPos,1,1,1,1);

//    this->m_gLayLft->addWidget(this->m_llS0ActVel,2,0,1,1);
//    this->m_gLayLft->addWidget(this->m_leS0ActVel,2,1,1,1);

//    this->m_gLayLft->addWidget(this->m_llS1PosActVal,3,0,1,1);
//    this->m_gLayLft->addWidget(this->m_leS1PosActVal,3,1,1,1);

//    this->m_gLayLft->addWidget(this->m_llS1TarPos,4,0,1,1);
//    this->m_gLayLft->addWidget(this->m_leS1TarPos,4,1,1,1);

//    this->m_gLayLft->addWidget(this->m_llS1ActVel,5,0,1,1);
//    this->m_gLayLft->addWidget(this->m_leS1ActVel,5,1,1,1);


    //left,right,up,down control.
    this->m_tbMoveLft=new QToolButton;
    this->m_tbMoveLft->setObjectName("leftButton");
    this->m_tbMoveLft->setFocusPolicy(Qt::NoFocus);
    this->m_tbMoveLft->setIcon(QIcon(":/images/arrow-l.png"));
    this->m_tbMoveLft->setIconSize(QSize(72,72));

    //this->m_tbMoveLft->setAutoRepeat(true);
    //this->m_tbMoveLft->setAutoRepeatDelay(1000);//delay 100ms.
    //this->m_tbMoveLft->setAutoRepeatInterval(100);//internal 100ms.

    this->m_tbMoveRht=new QToolButton;
    this->m_tbMoveRht->setText(tr("Right"));
    this->m_tbMoveRht->setFocusPolicy(Qt::NoFocus);
    this->m_tbMoveRht->setIcon(QIcon(":/images/arrow-r.png"));
    this->m_tbMoveRht->setIconSize(QSize(72,72));

    this->m_tbMoveUp=new QToolButton;
    this->m_tbMoveUp->setText(tr("Up"));
    this->m_tbMoveUp->setFocusPolicy(Qt::NoFocus);
    this->m_tbMoveUp->setIcon(QIcon(":/images/arrow-u.png"));
    this->m_tbMoveUp->setIconSize(QSize(72,72));

    this->m_tbMoveDn=new QToolButton;
    this->m_tbMoveDn->setText(tr("Down"));
    this->m_tbMoveDn->setFocusPolicy(Qt::NoFocus);
    this->m_tbMoveDn->setIcon(QIcon(":/images/arrow-d.png"));
    this->m_tbMoveDn->setIconSize(QSize(72,72));

    this->m_gLayoutMove=new QGridLayout;
    this->m_gLayoutMove->addWidget(this->m_tbMoveUp,0,1,1,1);
    this->m_gLayoutMove->addWidget(this->m_tbMoveLft,1,0,1,1);
    this->m_gLayoutMove->addWidget(this->m_tbMoveRht,1,2,1,1);
    this->m_gLayoutMove->addWidget(this->m_tbMoveDn,2,1,1,1);

    this->m_vLayRht=new QVBoxLayout;
    this->m_vLayRht->addStretch(1);
    this->m_vLayRht->addLayout(this->m_gLayoutMove);
    this->m_vLayRht->addStretch(1);

    //the layout top.
    this->m_hLayoutTop=new QHBoxLayout;
    this->m_hLayoutTop->addStretch(1);
    this->m_hLayoutTop->addLayout(this->m_vLayRht);
    this->setLayout(this->m_hLayoutTop);

    //make connections.
    QObject::connect(this->m_tbMoveLft,SIGNAL(clicked(bool)),this,SLOT(ZSlotMoveToLeft()));
    QObject::connect(this->m_tbMoveRht,SIGNAL(clicked(bool)),this,SLOT(ZSlotMoveToRight()));
    QObject::connect(this->m_tbMoveUp,SIGNAL(clicked(bool)),this,SLOT(ZSlotMoveToUp()));
    QObject::connect(this->m_tbMoveDn,SIGNAL(clicked(bool)),this,SLOT(ZSlotMoveToDown()));
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
        painter.fillRect(QRectF(0,0,this->width(),this->height()),Qt::black);
        return;
    }

    //draw center rectange on image.
    QPainter painterImg(&this->m_img);
    int centerBoxWidth=200;
    int centerBoxHeight=200;
    QRectF rectCenter(this->m_img.width()/2-centerBoxWidth/2,///<
                       this->m_img.height()/2-centerBoxHeight,///<
                       centerBoxWidth,centerBoxHeight);
    painterImg.setPen(QPen(Qt::red,2));
    painterImg.drawRect(rectCenter);
    //qDebug()<<"Image Center:"<<rectCenter;
    this->m_ptCenter=rectCenter.center().toPoint();

    //draw the image.
    painter.drawImage(QRectF(0,0,this->width(),this->height()),this->m_img);

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
}
void ZMainUI::ZSlotPDO(qint32 iSlave,qint32 iActPos,qint32 iTarPos,qint32 iActVel)
{
    switch(iSlave)
    {
    case 0:
        this->m_leS0PosActVal->setText(QString::number(iActPos));
        this->m_leS0TarPos->setText(QString::number(iTarPos));
        this->m_leS0ActVel->setText(QString::number(iActVel));
        break;
    case 1:
        this->m_leS1PosActVal->setText(QString::number(iActPos));
        this->m_leS1TarPos->setText(QString::number(iTarPos));
        this->m_leS1ActVel->setText(QString::number(iActVel));
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
    gGblPara.m_pixelDiffX=-2000;
}
void ZMainUI::ZSlotMoveToRight()
{
    gGblPara.m_pixelDiffX=+2000;
}
void ZMainUI::ZSlotMoveToUp()
{
    gGblPara.m_pixelDiffY=-2000;
}
void ZMainUI::ZSlotMoveToDown()
{
    gGblPara.m_pixelDiffY=+2000;
}
