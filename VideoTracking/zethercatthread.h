#ifndef ZETHERCATTHREAD_H
#define ZETHERCATTHREAD_H

#include <QThread>
#include <zgblpara.h>
class ZEtherCATThread : public QThread
{
    Q_OBJECT
public:
    ZEtherCATThread();
protected:
    void run();
signals:
    void ZSigPDO(qint32 iSlave,qint32 iActPos,qint32 iTarPos,qint32 iActVel,qint32 iStatusWord);
signals:
    void ZSigLog(bool bErrFlag,QString log);
private:
    void ZDoCyclicTask();
};

#endif // ZETHERCATTHREAD_H
