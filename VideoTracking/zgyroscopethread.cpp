#include "zgyroscopethread.h"
#include <zgblpara.h>
#include <QDebug>
#include "jy901.h"
ZGyroscopeThread::ZGyroscopeThread()
{

}
void ZGyroscopeThread::run()
{
    this->m_buffer=new char[1024];
    this->m_count=0;
    this->m_com=new QSerialPort;
    this->m_com->setPortName("/dev/ttyUSB0");
    this->m_com->setBaudRate(QSerialPort::Baud9600);
    this->m_com->setDataBits(QSerialPort::Data8);
    this->m_com->setParity(QSerialPort::NoParity);
    this->m_com->setStopBits(QSerialPort::OneStop);
    this->m_com->setFlowControl(QSerialPort::NoFlowControl);
    if(!this->m_com->open(QIODevice::ReadWrite))
    {
        qDebug()<<"failed to open ttyUSB0"<<this->m_com->errorString();
        return;
    }
    QObject::connect(this->m_com,SIGNAL(readyRead()),this,SLOT(ZSlotReadReady()));
    //enter event-loop,until exit() was called.
    this->exec();

    this->m_com->close();
    delete this->m_com;
    delete [] this->m_buffer;
    return;
}
void ZGyroscopeThread::ZSlotReadReady()
{
    int iCanRead=1024-this->m_count;
    while(this->m_com->bytesAvailable()>0)
    {
        int iRet=this->m_com->read(this->m_buffer+this->m_count,iCanRead);
        if(iRet>0)
        {
            this->m_count+=iRet;

            //scan buffer,parse buffer.
            for(qint32 i=0;i<this->m_count-1;i++)
            {
                if(this->m_buffer[i]==0x55 && this->m_buffer[i+1]==0x50)
                {

                }
            }
        }else{
            qDebug()<<"read error:"<<this->m_com->errorString();
        }
    }

    while(this->m_com->bytesAvailable()>=11)
    {
        char buffer[11];
        int ret=this->m_com->read(buffer,11);
        qDebug("read %d from com\n",ret);
        if(buffer[0]!=0x55)
        {
            qDebug()<<"error sync head!";
            continue;
        }
        switch(buffer[1])
        {
        case 0x50:
            stcTime.ucYear 		= buffer[2];
            stcTime.ucMonth 	= buffer[3];
            stcTime.ucDay 		= buffer[4];
            stcTime.ucHour 		= buffer[5];
            stcTime.ucMinute 	= buffer[6];
            stcTime.ucSecond 	= buffer[7];
            stcTime.usMiliSecond=((unsigned short)buffer[9]<<8)|buffer[8];
            break;
        case 0x51:
            stcAcc.a[0] = ((unsigned short)buffer[3]<<8)|buffer[2];
            stcAcc.a[1] = ((unsigned short)buffer[5]<<8)|buffer[4];
            stcAcc.a[2] = ((unsigned short)buffer[7]<<8)|buffer[6];
            break;
        case 0x52:
            stcGyro.w[0] = ((unsigned short)buffer[3]<<8)|buffer[2];
            stcGyro.w[1] = ((unsigned short)buffer[5]<<8)|buffer[4];
            stcGyro.w[2] = ((unsigned short)buffer[7]<<8)|buffer[6];
            break;
        case 0x53:
            stcAngle.Angle[0] = ((unsigned short)buffer[3]<<8)|buffer[2];
            stcAngle.Angle[1] = ((unsigned short)buffer[5]<<8)|buffer[4];
            stcAngle.Angle[2] = ((unsigned short)buffer[7]<<8)|buffer[6];
            stcAngle.T = ((unsigned short)buffer[9]<<8)|buffer[8];
            break;
        case 0x54:
            stcMag.h[0] = ((unsigned short)buffer[3]<<8)|buffer[2];
            stcMag.h[1] = ((unsigned short)buffer[5]<<8)|buffer[4];
            stcMag.h[2] = ((unsigned short)buffer[7]<<8)|buffer[6];
            stcAngle.T = ((unsigned short)buffer[9]<<8)|buffer[8];
            break;
        case 0x55:
            stcDStatus.sDStatus[0] = ((unsigned short)buffer[3]<<8)|buffer[2];
            stcDStatus.sDStatus[1] = ((unsigned short)buffer[5]<<8)|buffer[4];
            stcDStatus.sDStatus[2] = ((unsigned short)buffer[7]<<8)|buffer[6];
            stcDStatus.sDStatus[3] = ((unsigned short)buffer[9]<<8)|buffer[8];
            break;
        case 0x56:
            buffer[2] = 0x12;
            buffer[3] = 0x34;
            buffer[4] = 0x56;
            buffer[5] = 0x78;
            CharToLong((char*)&stcPress.lPressure,(char*)&buffer[2]);
            CharToLong((char*)&stcPress.lAltitude,(char*)&buffer[6]);
            break;
        case 0x57:
            CharToLong((char*)&stcLonLat.lLon,(char*)&buffer[2]);
            CharToLong((char*)&stcLonLat.lLat,(char*)&buffer[6]);
            break;
        case 0x58:
            stcGPSV.sGPSHeight = ((unsigned short)buffer[3]<<8)|buffer[2];
            stcGPSV.sGPSYaw = ((unsigned short)buffer[5]<<8)|buffer[4];
            CharToLong((char*)&stcGPSV.lGPSVelocity,(char*)&buffer[6]);
            break;
        }

        qDebug("Angle:%.3f,%.3f,%.3f\n",(float)stcAngle.Angle[0]/32768*180,(float)stcAngle.Angle[1]/32768*180,(float)stcAngle.Angle[2]/32768*180);
    }
}
