#include "zdirectionbar.h"

ZDirectionBar::ZDirectionBar()
{
    //left,right,up,down control.
    this->m_tbMoveLft=new QToolButton;
    this->m_tbMoveLft->setObjectName("ZDirectionBarButton");
    this->m_tbMoveLft->setFocusPolicy(Qt::NoFocus);
    this->m_tbMoveLft->setIcon(QIcon(":/images/arrow-o-l.png"));
    this->m_tbMoveLft->setIconSize(QSize(72,72));
    this->m_tbMoveLft->setAutoRepeat(true);
    this->m_tbMoveLft->setAutoRepeatDelay(1);//delay 100ms.
    this->m_tbMoveLft->setAutoRepeatInterval(1);//internal 100ms.

    this->m_tbMoveRht=new QToolButton;
    this->m_tbMoveRht->setObjectName("ZDirectionBarButton");
    this->m_tbMoveRht->setFocusPolicy(Qt::NoFocus);
    this->m_tbMoveRht->setIcon(QIcon(":/images/arrow-o-r.png"));
    this->m_tbMoveRht->setIconSize(QSize(72,72));
    this->m_tbMoveRht->setAutoRepeat(true);
    this->m_tbMoveRht->setAutoRepeatDelay(1);//delay 100ms.
    this->m_tbMoveRht->setAutoRepeatInterval(1);//internal 100ms.

    this->m_tbMoveUp=new QToolButton;
    this->m_tbMoveUp->setObjectName("ZDirectionBarButton");
    this->m_tbMoveUp->setFocusPolicy(Qt::NoFocus);
    this->m_tbMoveUp->setIcon(QIcon(":/images/arrow-o-u.png"));
    this->m_tbMoveUp->setIconSize(QSize(72,72));
    this->m_tbMoveUp->setAutoRepeat(true);
    this->m_tbMoveUp->setAutoRepeatDelay(1);//delay 100ms.
    this->m_tbMoveUp->setAutoRepeatInterval(1);//internal 100ms.

    this->m_tbMoveDn=new QToolButton;
    this->m_tbMoveDn->setObjectName("ZDirectionBarButton");
    this->m_tbMoveDn->setFocusPolicy(Qt::NoFocus);
    this->m_tbMoveDn->setIcon(QIcon(":/images/arrow-o-d.png"));
    this->m_tbMoveDn->setIconSize(QSize(72,72));
    this->m_tbMoveDn->setAutoRepeat(true);
    this->m_tbMoveDn->setAutoRepeatDelay(1);//delay 100ms.
    this->m_tbMoveDn->setAutoRepeatInterval(1);//internal 100ms.

    this->m_gLayoutMove=new QGridLayout;
    this->m_gLayoutMove->addWidget(this->m_tbMoveUp,0,1,1,1);
    this->m_gLayoutMove->addWidget(this->m_tbMoveLft,1,0,1,1);
    this->m_gLayoutMove->addWidget(this->m_tbMoveRht,1,2,1,1);
    this->m_gLayoutMove->addWidget(this->m_tbMoveDn,2,1,1,1);
    this->setLayout(this->m_gLayoutMove);

    QObject::connect(this->m_tbMoveLft,SIGNAL(clicked(bool)),this,SIGNAL(ZSigLeft()));
    QObject::connect(this->m_tbMoveRht,SIGNAL(clicked(bool)),this,SIGNAL(ZSigRight()));
    QObject::connect(this->m_tbMoveUp,SIGNAL(clicked(bool)),this,SIGNAL(ZSigUp()));
    QObject::connect(this->m_tbMoveDn,SIGNAL(clicked(bool)),this,SIGNAL(ZSigDown()));
}
ZDirectionBar::~ZDirectionBar()
{
    delete this->m_tbMoveLft;
    delete this->m_tbMoveRht;
    delete this->m_tbMoveUp;
    delete this->m_tbMoveDn;
    delete this->m_gLayoutMove;
}
