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
    void ZSigPDO(qint32 iSlave,qint32 iActPos,qint32 iActVel,qint32 iStatusWord);
private:

};

#endif // ZETHERCATTHREAD_H
