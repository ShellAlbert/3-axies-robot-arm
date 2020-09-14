#include "zgyroscopethread.h"
#include <zgblpara.h>
ZGyroscopeThread::ZGyroscopeThread()
{

}
void ZGyroscopeThread::run()
{
    this->m_com=new QSerialPort;
    this->m_com->setBaudRate(QSerialPort::Baud9600);
    while(!gGblPara.m_bExitFlag)
    {

    }
    this->m_com->close();
    delete this->m_com;
    return;
}
