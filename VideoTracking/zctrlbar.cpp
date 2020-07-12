#include "zctrlbar.h"

ZCtrlBar::ZCtrlBar()
{
    this->m_tbHome=new QToolButton;
    this->m_tbHome->setFocusPolicy(Qt::NoFocus);
    this->m_tbHome->setIcon(QIcon(":/images/home.png"));
    this->m_tbHome->setIconSize(QSize(72,72));

    this->m_tbTrack=new QToolButton;
    this->m_tbTrack->setFocusPolicy(Qt::NoFocus);
    this->m_tbTrack->setIcon(QIcon(":/images/track.png"));
    this->m_tbTrack->setIconSize(QSize(72,72));

    this->m_tbData=new QToolButton;
    this->m_tbData->setFocusPolicy(Qt::NoFocus);
    this->m_tbData->setIcon(QIcon(":/images/data.png"));
    this->m_tbData->setIconSize(QSize(72,72));

    this->m_tbScan=new QToolButton;
    this->m_tbScan->setFocusPolicy(Qt::NoFocus);
    this->m_tbScan->setIcon(QIcon(":/images/scan.png"));
    this->m_tbScan->setIconSize(QSize(72,72));

    this->m_hLay=new QHBoxLayout;
    this->m_hLay->setContentsMargins(60,20,60,20);
    this->m_hLay->setSpacing(20);
    this->m_hLay->addStretch(1);
    this->m_hLay->addWidget(this->m_tbHome);
    this->m_hLay->addWidget(this->m_tbTrack);
    this->m_hLay->addWidget(this->m_tbData);
    this->m_hLay->addWidget(this->m_tbScan);
    this->m_hLay->addStretch(1);

    this->setLayout(this->m_hLay);
}
ZCtrlBar::~ZCtrlBar()
{
    delete this->m_tbHome;
    delete this->m_tbTrack;
    delete this->m_tbData;
    delete this->m_tbScan;
    delete this->m_hLay;
}
