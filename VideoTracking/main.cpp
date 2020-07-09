#include "zmainui.h"
#include <QApplication>
#include "zcapturethread.h"
#include "zmatfifo.h"
#include "zprocessingthread.h"
#include "zethercatthread.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    ZMatFIFO fifoCap1(25,false);
    ZCaptureThread cap1("192.168.137.12",&fifoCap1);
    ZProcessingThread proc1(&fifoCap1);
    ZEtherCATThread ecThread;
    ZMainUI win;

    QObject::connect(&proc1,SIGNAL(ZSigNewImg(QImage)),&win,SLOT(ZSlotUpdateImg(QImage)));
    QObject::connect(&ecThread,SIGNAL(ZSigPDO(qint32,qint32,qint32,qint32,qint32)),&win,SLOT(ZSlotPDO(qint32,qint32,qint32,qint32,qint32)));
    if(!win.ZDoInit())
    {
        return -1;
    }
    win.show();
    cap1.start();
    proc1.start();
    ecThread.start();

    qint32 ret=app.exec();
    cap1.wait();
    proc1.wait();
    return ret;
}
