#include "zmainui.h"
#include <QApplication>
#include "zcapturethread.h"
#include "zmatfifo.h"
#include "zprocessingthread.h"
#include "zrdservooutthread.h"
#include <QFile>
#include <QDebug>
#include <zgblpara.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    //load skin.
    QFile fileSkin(":/skin/skin_default.qss");
    if(fileSkin.open(QIODevice::ReadOnly))
    {
        QString skinQss=fileSkin.readAll();
        app.setStyleSheet(skinQss);
        fileSkin.close();
    }
    //TechServo FIFO.
    //open read mode side first.
    qDebug()<<"opening server.fifo.out ...";
    if((gGblPara.m_fdServoFIFOOut=open(SERVO_FIFO_OUT,O_RDONLY))<0)
    {
        qDebug()<<"failed to open read fifo:"<<SERVO_FIFO_OUT;
        return -1;
    }
    qDebug()<<"open server.fifo.out okay";

    //open write mode side second.
    qDebug()<<"opening server.fifo.in ...";
    if((gGblPara.m_fdServoFIFOIn=open(SERVO_FIFO_IN,O_WRONLY))<0)
    {
        qDebug()<<"failed to open write fifo:"<<SERVO_FIFO_IN;
        return -1;
    }
    qDebug()<<"open server.fifo.in okay";

    ZMatFIFO fifoCap1(25,false);
    ZCaptureThread cap1("192.168.137.12",&fifoCap1);
    ZProcessingThread proc1(&fifoCap1);
    ZRdServoOutThread servoOut;
    ZMainUI win;

    QObject::connect(&cap1,SIGNAL(ZSigNewImg(QImage)),&win,SLOT(ZSlotUpdateImg(QImage)));
    QObject::connect(&proc1,SIGNAL(ZSigInitBox(QImage)),&win,SLOT(ZSlotInitBox(QImage)));
    QObject::connect(&proc1,SIGNAL(ZSigLocked(bool,QRect)),&win,SLOT(ZSlotLocked(bool,QRect)));

    if(!win.ZDoInit())
    {
        return -1;
    }
    win.showMaximized();
    //win.showFullScreen();
    cap1.start();
    proc1.start();
    servoOut.start();

    qint32 ret=app.exec();
    cap1.wait();
    proc1.wait();
    servoOut.wait();

    //close FIFOs.
    close(gGblPara.m_fdServoFIFOIn);
    close(gGblPara.m_fdServoFIFOOut);

    return ret;
}
