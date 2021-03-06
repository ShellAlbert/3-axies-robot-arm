#include "zdialoghome.h"
#include <QDebug>
ZDialogHome::ZDialogHome(QWidget *parent):QDialog(parent)
{
    this->m_grp1=new QGroupBox(tr("Axis(es) Zero Calibration"));

    this->m_tbS0Homing=new QToolButton;
    this->m_tbS0Homing->setText(tr("Axis0 Zero"));

    this->m_tbS1Homing=new QToolButton;
    this->m_tbS1Homing->setText(tr("Axis1 Zero"));

    this->m_hLayoutGrp1=new QHBoxLayout;
    this->m_hLayoutGrp1->addWidget(this->m_tbS0Homing);
    this->m_hLayoutGrp1->addWidget(this->m_tbS1Homing);
    this->m_grp1->setLayout(this->m_hLayoutGrp1);

    this->m_vLayMain=new QVBoxLayout;
    this->m_vLayMain->addWidget(this->m_grp1);
    this->setLayout(this->m_vLayMain);

    //make connections.
    QObject::connect(this->m_tbS0Homing,SIGNAL(clicked(bool)),this,SLOT(ZSlotS0Homing()));
    QObject::connect(this->m_tbS1Homing,SIGNAL(clicked(bool)),this,SLOT(ZSlotS1Homing()));
}
ZDialogHome::~ZDialogHome()
{
    delete this->m_tbS0Homing;
    delete this->m_tbS1Homing;
    delete this->m_hLayoutGrp1;
    delete this->m_grp1;

    delete this->m_vLayMain;
}
QSize ZDialogHome::sizeHint() const
{
    return QSize(600,300);
}
void ZDialogHome::ZSlotS0Homing()
{

}
void ZDialogHome::ZSlotS1Homing()
{

}
