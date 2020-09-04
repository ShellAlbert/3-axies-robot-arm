#ifndef ZRDSERVOOUTTHREAD_H
#define ZRDSERVOOUTTHREAD_H

#include <QThread>

class ZRdServoOutThread:public QThread
{
    Q_OBJECT
public:
    ZRdServoOutThread();
protected:
    void run();

};

#endif // ZRDSERVOOUTTHREAD_H
