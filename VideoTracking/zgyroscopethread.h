#ifndef ZGYROSCOPETHREAD_H
#define ZGYROSCOPETHREAD_H

#include <QThread>
#include <QtSerialPort>
class ZGyroscopeThread:public QThread
{
    Q_OBJECT
public:
    ZGyroscopeThread();
public slots:
    void ZSlotReadReady();
protected:
    void run();
private:
    QSerialPort *m_com;
    char *m_buffer;
    qint32 m_count;
};

#endif // ZGYROSCOPETHREAD_H
