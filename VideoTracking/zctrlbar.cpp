#include "zctrlbar.h"
#include "zgblpara.h"
#include <QDebug>
ZCtrlBar::ZCtrlBar()
{
    this->setObjectName("ZCtrlBar");

    this->m_tbHome=new QToolButton;
    this->m_tbHome->setObjectName("ZCtrlBarButton");
    this->m_tbHome->setFocusPolicy(Qt::NoFocus);
    this->m_tbHome->setIcon(QIcon(":/images/home.png"));
    this->m_tbHome->setIconSize(QSize(72,72));

    this->m_tbMode=new QToolButton;
    this->m_tbMode->setObjectName("ZCtrlBarButton");
    this->m_tbMode->setFocusPolicy(Qt::NoFocus);
    this->m_tbMode->setIcon(QIcon(":/images/free.png"));
    this->m_tbMode->setIconSize(QSize(72,72));

    this->m_tbData=new QToolButton;
    this->m_tbData->setObjectName("ZCtrlBarButton");
    this->m_tbData->setFocusPolicy(Qt::NoFocus);
    this->m_tbData->setIcon(QIcon(":/images/data.png"));
    this->m_tbData->setIconSize(QSize(72,72));

    this->m_tbScan=new QToolButton;
    this->m_tbScan->setObjectName("ZCtrlBarButton");
    this->m_tbScan->setFocusPolicy(Qt::NoFocus);
    this->m_tbScan->setIcon(QIcon(":/images/scan.png"));
    this->m_tbScan->setIconSize(QSize(72,72));

    this->m_tbCalibrate=new QToolButton;
    this->m_tbCalibrate->setObjectName("ZCtrlBarButton");
    this->m_tbCalibrate->setFocusPolicy(Qt::NoFocus);
    this->m_tbCalibrate->setIcon(QIcon(":/images/cal_start.png"));
    this->m_tbCalibrate->setIconSize(QSize(72,72));

    this->m_hLay=new QHBoxLayout;
    this->m_hLay->setContentsMargins(60,20,60,20);
    this->m_hLay->setSpacing(20);
    this->m_hLay->addStretch(1);
    this->m_hLay->addWidget(this->m_tbHome);
    this->m_hLay->addWidget(this->m_tbMode);
    this->m_hLay->addWidget(this->m_tbData);
    this->m_hLay->addWidget(this->m_tbScan);
    this->m_hLay->addWidget(this->m_tbCalibrate);
    this->m_hLay->addStretch(1);

    this->setLayout(this->m_hLay);

    //make connections.
    QObject::connect(this->m_tbMode,SIGNAL(clicked(bool)),this,SLOT(ZSlotMode()));
    QObject::connect(this->m_tbHome,SIGNAL(clicked(bool)),this,SIGNAL(ZSigHome()));
    QObject::connect(this->m_tbScan,SIGNAL(clicked(bool)),this,SIGNAL(ZSigScan()));
    QObject::connect(this->m_tbCalibrate,SIGNAL(clicked(bool)),this,SLOT(ZSlotCalibrate()));
}
ZCtrlBar::~ZCtrlBar()
{
    delete this->m_tbHome;
    delete this->m_tbMode;
    delete this->m_tbData;
    delete this->m_tbScan;
    delete this->m_tbCalibrate;
    delete this->m_hLay;
}
void ZCtrlBar::ZSlotMode()
{
    switch(gGblPara.m_appMode)
    {
    case Free_Mode:
        gGblPara.m_appMode=SelectROI_Mode;
        this->m_tbMode->setIcon(QIcon(":/images/selectroi.png"));
        break;
    case SelectROI_Mode:
        gGblPara.m_appMode=Track_Mode;
        this->m_tbMode->setIcon(QIcon(":/images/track.png"));
        break;
    case Track_Mode:
        gGblPara.m_appMode=Free_Mode;
        this->m_tbMode->setIcon(QIcon(":/images/free.png"));
        break;
    default:
        break;
    }
    emit this->ZSigModeChanged();
}
void ZCtrlBar::ZSlotCalibrate()
{
    switch(gGblPara.m_CalibrateFSM)
    {
    case FSM_Calibrate_Start:
        gGblPara.m_CalibrateFSM=FSM_Calibrate_Left;
        this->m_tbCalibrate->setIcon(QIcon(":/images/cal_1.png"));
        break;
    case FSM_Calibrate_Left:
        gGblPara.m_CalibrateFSM=FSM_Calibrate_LeftConfirm;
        break;
    case FSM_Calibrate_LeftConfirm:
        gGblPara.m_CalibrateFSM=FSM_Calibrate_Right;
        this->m_tbCalibrate->setIcon(QIcon(":/images/cal_2.png"));
        break;
    case FSM_Calibrate_Right:
        gGblPara.m_CalibrateFSM=FSM_Calibrate_RightConfirm;
        break;
    case FSM_Calibrate_RightConfirm:
        gGblPara.m_CalibrateFSM=FSM_Calibrate_Top;
        this->m_tbCalibrate->setIcon(QIcon(":/images/cal_3.png"));
        break;
    case FSM_Calibrate_Top:
        gGblPara.m_CalibrateFSM=FSM_Calibrate_TopConfirm;
        break;
    case FSM_Calibrate_TopConfirm:
        gGblPara.m_CalibrateFSM=FSM_Calibrate_Bottom;
        this->m_tbCalibrate->setIcon(QIcon(":/images/cal_4.png"));
        break;
    case FSM_Calibrate_Bottom:
        gGblPara.m_CalibrateFSM=FSM_Calibrate_BottomConfirm;
        break;
    case FSM_Calibrate_BottomConfirm:
        gGblPara.m_CalibrateFSM=FSM_Calibrate_Done;
        this->m_tbCalibrate->setIcon(QIcon(":/images/cal_done.png"));
        break;
    case FSM_Calibrate_Done:
        gGblPara.m_CalibrateFSM=FSM_Calibrate_Start;
        this->m_tbCalibrate->setIcon(QIcon(":/images/cal_start.png"));
        break;
    default:
        break;
    }
}
