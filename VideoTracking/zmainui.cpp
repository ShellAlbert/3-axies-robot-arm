#include "zmainui.h"

ZMainUI::ZMainUI(QWidget *parent)
    : QWidget(parent)
{
}

ZMainUI::~ZMainUI()
{
    this->m_ecThread->quit();
    this->m_ecThread->wait();

    //Left right direction.
    delete this->m_tbCtrlLR;
    delete this->m_llActVelLR;
    delete this->m_leActVelLR;

    delete this->m_llActPosLR;
    delete this->m_leActPosLR;

    delete this->m_llTarPosLR;
    delete this->m_leTarPosLR;

    delete this->m_gLayoutLft;
    //Up Down direction.
    delete this->m_tbCtrlUD;
    delete this->m_llActVelUD;
    delete this->m_leActVelUD;

    delete this->m_llActPosUD;
    delete this->m_leActPosUD;

    delete this->m_llTarPosUD;
    delete this->m_leTarPosUD;

    delete this->m_gLayoutRht;
    //the main.
    delete this->m_hLayoutMain;
}
bool ZMainUI::ZDoInit()
{
    //left part: left right direction servo motor.
    this->m_tbCtrlLR=new QToolButton;
    this->m_tbCtrlLR->setText(tr("START"));

    this->m_llActVelLR=new QLabel;
    this->m_llActVelLR->setText(tr("实际速度"));
    this->m_leActVelLR=new QLineEdit;
    this->m_leActVelLR->setFocusPolicy(Qt::NoFocus);
    this->m_leActVelLR->setText("0");

    this->m_llActPosLR=new QLabel;
    this->m_llActPosLR->setText(tr("当前位置"));
    this->m_leActPosLR=new QLineEdit;
    this->m_leActPosLR->setFocusPolicy(Qt::NoFocus);
    this->m_leActPosLR->setText("0");

    this->m_llTarPosLR=new QLabel;
    this->m_llTarPosLR->setText(tr("目标位置"));
    this->m_leTarPosLR=new QLineEdit;
    this->m_leTarPosLR->setFocusPolicy(Qt::NoFocus);
    this->m_leTarPosLR->setText("0");

    this->m_gLayoutLft=new QGridLayout;
    this->m_gLayoutLft->addWidget(this->m_tbCtrlLR,0,1,1,1);

    this->m_gLayoutLft->addWidget(this->m_llActVelLR,1,0,1,1);
    this->m_gLayoutLft->addWidget(this->m_leActVelLR,1,1,1,1);

    this->m_gLayoutLft->addWidget(this->m_llActPosLR,2,0,1,1);
    this->m_gLayoutLft->addWidget(this->m_leActPosLR,2,1,1,1);

    this->m_gLayoutLft->addWidget(this->m_llTarPosLR,3,0,1,1);
    this->m_gLayoutLft->addWidget(this->m_leTarPosLR,3,1,1,1);

    //right part: up down direction servo motor.
    this->m_tbCtrlUD=new QToolButton;
    this->m_tbCtrlUD->setText(tr("START"));

    this->m_llActVelUD=new QLabel;
    this->m_llActVelUD->setText(tr("实际速度"));
    this->m_leActVelUD=new QLineEdit;
    this->m_leActVelUD->setFocusPolicy(Qt::NoFocus);
    this->m_leActVelUD->setText("0");

    this->m_llActPosUD=new QLabel;
    this->m_llActPosUD->setText(tr("当前位置"));
    this->m_leActPosUD=new QLineEdit;
    this->m_leActPosUD->setFocusPolicy(Qt::NoFocus);
    this->m_leActPosUD->setText("0");

    this->m_llTarPosUD=new QLabel;
    this->m_llTarPosUD->setText(tr("目标位置"));
    this->m_leTarPosUD=new QLineEdit;
    this->m_leTarPosUD->setFocusPolicy(Qt::NoFocus);
    this->m_leTarPosUD->setText("0");

    this->m_gLayoutRht=new QGridLayout;
    this->m_gLayoutRht->addWidget(this->m_tbCtrlUD,0,1,1,1);

    this->m_gLayoutRht->addWidget(this->m_llActVelUD,1,0,1,1);
    this->m_gLayoutRht->addWidget(this->m_leActVelUD,1,1,1,1);

    this->m_gLayoutRht->addWidget(this->m_llActPosUD,2,0,1,1);
    this->m_gLayoutRht->addWidget(this->m_leActPosUD,2,1,1,1);

    this->m_gLayoutRht->addWidget(this->m_llTarPosUD,3,0,1,1);
    this->m_gLayoutRht->addWidget(this->m_leTarPosUD,3,1,1,1);

    //the main layout.
    this->m_hLayoutMain=new QHBoxLayout;
    this->m_hLayoutMain->addLayout(this->m_gLayoutLft);
    this->m_hLayoutMain->addStretch(1);
    this->m_hLayoutMain->addLayout(this->m_gLayoutRht);
    this->setLayout(this->m_hLayoutMain);

    //make connections.
    QObject::connect(this->m_tbCtrlLR,SIGNAL(clicked(bool)),this,SLOT(ZSlotCtrlLR()));
    QObject::connect(this->m_tbCtrlUD,SIGNAL(clicked(bool)),this,SLOT(ZSlotCtrlUD()));

    this->m_ecThread=new ZEtherCATThread;
    this->m_ecThread->start();
    return true;
}
void ZMainUI::ZSlotCtrlLR()
{
    if((gGblPara.m_iSlavesEnBitMask&(0x1<<0))==0)
    {
        //set enabled flag.
        gGblPara.m_iSlavesEnBitMask|=(0x1<<0);
        this->m_tbCtrlLR->setText(tr("STOP"));
    }else{
        //set disabled flag.
        gGblPara.m_iSlavesEnBitMask&=~(0x1<<0);
        this->m_tbCtrlLR->setText(tr("START"));
    }
}
void ZMainUI::ZSlotCtrlUD()
{
    if((gGblPara.m_iSlavesEnBitMask&(0x1<<1))==0)
    {
        //set enabled flag.
        gGblPara.m_iSlavesEnBitMask|=(0x1<<1);
        this->m_tbCtrlUD->setText(tr("STOP"));
    }else{
        //set disabled flag.
        gGblPara.m_iSlavesEnBitMask&=~(0x1<<1);
        this->m_tbCtrlUD->setText(tr("START"));
    }
}
