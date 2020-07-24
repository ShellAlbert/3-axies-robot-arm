#include "zmainui.h"
#include <QApplication>
#include "zcapturethread.h"
#include "zmatfifo.h"
#include "zprocessingthread.h"
#include "zethercatthread.h"
#include <QFile>
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

    ZMatFIFO fifoCap1(25,false);
    ZCaptureThread cap1("192.168.137.12",&fifoCap1);
    ZProcessingThread proc1(&fifoCap1);
    ZEtherCATThread ecThread;
    ZMainUI win;

    QObject::connect(&cap1,SIGNAL(ZSigNewImg(QImage)),&win,SLOT(ZSlotUpdateImg(QImage)));
    QObject::connect(&proc1,SIGNAL(ZSigInitBox(QImage)),&win,SLOT(ZSlotInitBox(QImage)));
    QObject::connect(&proc1,SIGNAL(ZSigLocked(bool,QRect)),&win,SLOT(ZSlotLocked(bool,QRect)));
    QObject::connect(&ecThread,SIGNAL(ZSigPDO(qint32,qint32,qint32,qint32)),&win,SLOT(ZSlotPDO(qint32,qint32,qint32,qint32)));
    QObject::connect(&ecThread,SIGNAL(ZSigLog(bool,QString)),&win,SLOT(ZSlotLog(bool,QString)));
    if(!win.ZDoInit())
    {
        return -1;
    }
    win.showMaximized();
    //win.showFullScreen();
    cap1.start();
    proc1.start();
    ecThread.start();

    qint32 ret=app.exec();
    cap1.wait();
    proc1.wait();
    return ret;
}
