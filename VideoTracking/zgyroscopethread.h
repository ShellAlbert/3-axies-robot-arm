#ifndef ZGYROSCOPETHREAD_H
#define ZGYROSCOPETHREAD_H

#include <QThread>
#include <QtSerialPort>
class ZGyroscopeThread:public QObject
{
    Q_OBJECT
public:
    ZGyroscopeThread();
protected:
    void run();
private:
    QSerialPort *m_com;
};

#endif // ZGYROSCOPETHREAD_H
