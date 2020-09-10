#include "zmatfifo.h"
#include <QDebug>
ZMatFIFO::ZMatFIFO(qint32 iSize)
{
    this->m_freeSema=new QSemaphore(1/*iSize*/);
    this->m_usedSema=new QSemaphore(0);
    this->m_iSize=1/*iSize*/;
}
bool ZMatFIFO::ZAddFrame(const cv::Mat &frame)
{
    if(this->m_freeSema->tryAcquire(1,100))
    {
        this->m_queue.enqueue(frame);
        this->m_usedSema->release();
        return true;
    }
    return false;
}
bool ZMatFIFO::ZGetFrame(cv::Mat &frame)
{
    if(this->m_usedSema->tryAcquire(1,100))
    {
        frame=this->m_queue.dequeue().clone();
        this->m_freeSema->release();
        return true;
    }
    return false;
}
qint32 ZMatFIFO::ZGetSize()
{
    return this->m_queue.size();
}
