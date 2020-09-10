#ifndef ZSERVOTHREAD_H
#define ZSERVOTHREAD_H

#include <QThread>
class ZServoThread : public QThread
{
    Q_OBJECT
public:
    ZServoThread();

signals:
    void ZSigLog(bool bError,const QString &log);
    void ZSigPDO(int servoID,int statusWord,int velocity,int position);
    void ZSigTargetReached();
protected:
    void run();
private:
    int ZMapPixel2Servo(int servoID,int diff);
    void ZUpdatePDO(void);
};

#endif // ZSERVOTHREAD_H
