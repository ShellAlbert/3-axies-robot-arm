#ifndef ZCAPTURETHREAD_H
#define ZCAPTURETHREAD_H

#include <QThread>
#include <QImage>
#include "zmatfifo.h"
class ZCaptureThread : public QThread
{
    Q_OBJECT
public:
    ZCaptureThread(QString ip,ZMatFIFO *fifo);

signals:
    void ZSigFpsUpdated(qint32 iFps);
protected:
    void run();

private:
    qint32 getFps();
private:
    QString m_ip;
    ZMatFIFO *m_fifo;
};

#endif // ZCAPTURETHREAD_H
