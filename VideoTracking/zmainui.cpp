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
    //left,right,up,down control.
    delete this->m_tbMoveLft;
    delete this->m_tbMoveRht;
    delete this->m_tbMoveUp;
    delete this->m_tbMoveDn;
    delete this->m_gLayoutMove;

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

    //the main layout.
    delete this->m_hLayoutTop;

    //the main layout.
    delete this->m_teLog;
    delete this->m_vLayoutMain;
}
bool ZMainUI::ZDoInit()
{
    //left,right,up,down control.
    this->m_tbMoveLft=new QToolButton;
    this->m_tbMoveLft->setText(tr("Left"));
    this->m_tbMoveLft->setFocusPolicy(Qt::NoFocus);
    //this->m_tbMoveLft->setAutoRepeat(true);
    //this->m_tbMoveLft->setAutoRepeatDelay(1000);//delay 100ms.
    //this->m_tbMoveLft->setAutoRepeatInterval(100);//internal 100ms.

    this->m_tbMoveRht=new QToolButton;
    this->m_tbMoveRht->setText(tr("Right"));
    this->m_tbMoveRht->setFocusPolicy(Qt::NoFocus);

    this->m_tbMoveUp=new QToolButton;
    this->m_tbMoveUp->setText(tr("Up"));
    this->m_tbMoveUp->setFocusPolicy(Qt::NoFocus);

    this->m_tbMoveDn=new QToolButton;
    this->m_tbMoveDn->setText(tr("Down"));
    this->m_tbMoveDn->setFocusPolicy(Qt::NoFocus);

    this->m_gLayoutMove=new QGridLayout;
    this->m_gLayoutMove->addWidget(this->m_tbMoveUp,0,1,1,1);
    this->m_gLayoutMove->addWidget(this->m_tbMoveLft,1,0,1,1);
    this->m_gLayoutMove->addWidget(this->m_tbMoveRht,1,2,1,1);
    this->m_gLayoutMove->addWidget(this->m_tbMoveDn,2,1,1,1);

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

    this->m_gLayLft=new QGridLayout;
    this->m_gLayLft->addLayout(this->m_gLayoutMove,0,0,1,2);
    this->m_gLayLft->addWidget(this->m_llS0PosActVal,1,0,1,1);
    this->m_gLayLft->addWidget(this->m_leS0PosActVal,1,1,1,1);

    this->m_gLayLft->addWidget(this->m_llS0TarPos,2,0,1,1);
    this->m_gLayLft->addWidget(this->m_leS0TarPos,2,1,1,1);

    this->m_gLayLft->addWidget(this->m_llS0ActVel,3,0,1,1);
    this->m_gLayLft->addWidget(this->m_leS0ActVel,3,1,1,1);

    this->m_gLayLft->addWidget(this->m_llS1PosActVal,4,0,1,1);
    this->m_gLayLft->addWidget(this->m_leS1PosActVal,4,1,1,1);

    this->m_gLayLft->addWidget(this->m_llS1TarPos,5,0,1,1);
    this->m_gLayLft->addWidget(this->m_leS1TarPos,5,1,1,1);

    this->m_gLayLft->addWidget(this->m_llS1ActVel,6,0,1,1);
    this->m_gLayLft->addWidget(this->m_leS1ActVel,6,1,1,1);

    //the layout top.
    this->m_hLayoutTop=new QHBoxLayout;
    this->m_hLayoutTop->addLayout(this->m_gLayLft);
    this->m_hLayoutTop->addStretch(1);

    //the main layout.
    this->m_teLog=new QTextEdit;
    this->m_vLayoutMain=new QVBoxLayout;
    this->m_vLayoutMain->addLayout(this->m_hLayoutTop);
    this->m_vLayoutMain->addStretch(1);
    this->m_vLayoutMain->addWidget(this->m_teLog);
    this->setLayout(this->m_vLayoutMain);

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

    painter.drawImage(QRectF(0,0,this->width(),this->height()),this->m_img);
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
void ZMainUI::keyPressEvent(QKeyEvent *event)
{
    switch(event->key())
    {
    case Qt::Key_Up:
        qDebug()<<"keyup";
        break;
    case Qt::Key_Down:
        qDebug()<<"keydown";
        break;
    case Qt::Key_Left:
        qDebug()<<"keyleft";
        break;
    case Qt::Key_Right:
        qDebug()<<"keyright";
        break;
    default:
        break;
    }
    QWidget::keyPressEvent(event);
}
void ZMainUI::keyReleaseEvent(QKeyEvent *event)
{
    switch(event->key())
    {
    case Qt::Key_Up:
        qDebug()<<"keyup_release";
        break;
    case Qt::Key_Down:
        qDebug()<<"keydown_release";
        break;
    case Qt::Key_Left:
        qDebug()<<"keyleft_release";
        break;
    case Qt::Key_Right:
        qDebug()<<"keyright_release";
        break;
    default:
        break;
    }
    QWidget::keyReleaseEvent(event);
}

void ZMainUI::ZSlotLog(bool bErrFlag,QString log)
{
    if(bErrFlag)
    {
        this->m_teLog->append(QString("<ERR> :")+log);
    }else{
        this->m_teLog->append(QString("<INFO>:")+log);
    }
}
void ZMainUI::ZSlotMoveToLeft()
{
    gGblPara.m_iSlave1TarPos+=200;
}
void ZMainUI::ZSlotMoveToRight()
{
    gGblPara.m_iSlave1TarPos-=200;
}
void ZMainUI::ZSlotMoveToUp()
{
    gGblPara.m_iSlave0TarPos+=200;

}
void ZMainUI::ZSlotMoveToDown()
{

    gGblPara.m_iSlave0TarPos-=200;
}
