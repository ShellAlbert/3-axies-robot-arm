#ifndef ZPROCESSINGTHREAD_H
#define ZPROCESSINGTHREAD_H

#include <QThread>
#include <QImage>
class ZMatFIFO;
class ZProcessingThread : public QThread
{
    Q_OBJECT
public:
    ZProcessingThread(ZMatFIFO *fifo);
signals:
    void ZSigNewImg(const QImage &img);
protected:
    void run();
private:
    ZMatFIFO *m_fifo;
};

#endif // ZPROCESSINGTHREAD_H
