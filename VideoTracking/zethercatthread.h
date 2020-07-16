#ifndef ZETHERCATTHREAD_H
#define ZETHERCATTHREAD_H

#include <QThread>
#include <zgblpara.h>
#include <zpidcalc.h>
#include <QVector>
class ZEtherCATThread : public QThread
{
    Q_OBJECT
public:
    ZEtherCATThread();
protected:
    void run();
signals:
    void ZSigPDO(qint32 iSlave,qint32 iActPos,qint32 iTarPos,qint32 iActVel);
signals:
    void ZSigLog(bool bErrFlag,QString log);
private:
    void ZDoCyclicTask();
private:
    QTimer *m_timer;
    ZPIDCalc m_pidS0;//slave 0 pid.
    ZPIDCalc m_pidS1;//slave 1 pid.

private:
    //Path planning vector.
    QVector<QPoint> m_vecPathPlan;
};

#endif // ZETHERCATTHREAD_H
