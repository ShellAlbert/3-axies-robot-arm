#include "zmainui.h"
#include <QApplication>
#include "zcapturethread.h"
#include "zmatfifo.h"
#include "zprocessingthread.h"
#include <zservothread.h>
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

    //create thread & UI.
    ZMatFIFO fifoCap1(25);
    ZCaptureThread cap1("192.168.137.12",&fifoCap1);
    ZProcessingThread proc1(&fifoCap1);
    ZServoThread servoThread;
    ZMainUI win;

    //make connections.
    QObject::connect(&cap1,SIGNAL(ZSigNewImg(QImage)),&win,SLOT(ZSlotUpdateImg(QImage)));
    QObject::connect(&proc1,SIGNAL(ZSigInitBox(QImage)),&win,SLOT(ZSlotInitBox(QImage)));
    QObject::connect(&proc1,SIGNAL(ZSigLocked(bool,QRect)),&win,SLOT(ZSlotLocked(bool,QRect)));
    QObject::connect(&proc1,SIGNAL(ZSigDiffXY(int,int)),&win,SLOT(ZSlotDiffXY(int,int)));
    QObject::connect(&servoThread,SIGNAL(ZSigLog(bool,QString)),&win,SLOT(ZSlotLog(bool,QString)));
    QObject::connect(&servoThread,SIGNAL(ZSigPDO(int,int,int,int)),&win,SLOT(ZSlotPDO(int,int,int,int)));
    if(!win.ZDoInit())
    {
        qDebug()<<"main UI initial failed.";
        return -1;
    }
    win.showMaximized();
    //win.showFullScreen();
    cap1.start();
    proc1.start();
    servoThread.start();

    qint32 ret=app.exec();
    cap1.wait();
    proc1.wait();
    servoThread.wait();
    return ret;
}
