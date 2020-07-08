#include "zmainui.h"

ZMainUI::ZMainUI(QWidget *parent)
    : QWidget(parent)
{
}

ZMainUI::~ZMainUI()
{
    delete this->m_tbCtrlLR;
    delete this->m_llActVelLR;
    delete this->m_llActPosLR;
    delete this->m_llTarPosLR;
    //Up Down direction.
    delete this->m_tbCtrlUD;
    delete this->m_llActVelUD;
    delete this->m_llActPosUD;
    delete this->m_llTarPosUD;

    delete this->m_vLayoutLft;
    //right part: video.
    delete this->m_vLayoutRht;
    delete this->m_hLayoutMain;
}
bool ZMainUI::ZDoInit()
{
    //left part: servo motor.
    this->m_tbCtrlLR=new QToolButton;
    this->m_tbCtrlLR->setText(tr("START"));

    this->m_llActVelLR=new QLabel;
    this->m_llActVelLR->setText(tr("ActualVelocity:0"));

    this->m_llActPosLR=new QLabel;
    this->m_llActPosLR->setText(tr("ActualPosition:0"));

    this->m_llTarPosLR=new QLabel;
    this->m_llTarPosLR->setText(tr("TargetPosition:0"));

    this->m_tbCtrlUD=new QToolButton;
    this->m_tbCtrlUD->setText(tr("START"));

    this->m_llActVelUD=new QLabel;
    this->m_llActVelUD->setText(tr("ActualVelocity:0"));

    this->m_llActPosUD=new QLabel;
    this->m_llActPosUD->setText(tr("ActualPosition:0"));

    this->m_llTarPosUD=new QLabel;
    this->m_llTarPosUD->setText(tr("TargetPosition:0"));

    this->m_vLayoutLft=new QVBoxLayout;
    this->m_vLayoutLft->addWidget(this->m_tbCtrlLR);
    this->m_vLayoutLft->addWidget(this->m_llActVelLR);
    this->m_vLayoutLft->addWidget(this->m_llActPosLR);
    this->m_vLayoutLft->addWidget(this->m_llTarPosLR);

    this->m_vLayoutLft->addWidget(this->m_tbCtrlUD);
    this->m_vLayoutLft->addWidget(this->m_llActVelUD);
    this->m_vLayoutLft->addWidget(this->m_llActPosUD);
    this->m_vLayoutLft->addWidget(this->m_llTarPosUD);

    this->m_vLayoutLft->addStretch(1);
    //right part: video.
    this->m_vLayoutRht=new QVBoxLayout;

    this->m_hLayoutMain=new QHBoxLayout;
    this->m_hLayoutMain->addLayout(this->m_vLayoutLft);
    this->m_hLayoutMain->addLayout(this->m_vLayoutRht);
    this->setLayout(this->m_hLayoutMain);

    return true;
}
