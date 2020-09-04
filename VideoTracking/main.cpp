#include "zmainui.h"
#include <QApplication>
#include "zcapturethread.h"
#include "zmatfifo.h"
#include "zprocessingthread.h"
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
    //TechServo fifo in fd.
    if((gGblPara.m_fdServoFIFOIN=open(SERVO_FIFO_IN,O_WRONLY))<0)
    {
        qDebug()<<"failed to open fifo:"<<SERVO_FIFO_IN;
        return -1;
    }

    ZMatFIFO fifoCap1(25,false);
    ZCaptureThread cap1("192.168.137.12",&fifoCap1);
    ZProcessingThread proc1(&fifoCap1);
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

    qint32 ret=app.exec();
    cap1.wait();
    proc1.wait();
    close(gGblPara.m_fdServoFIFOIN);

    return ret;
}
