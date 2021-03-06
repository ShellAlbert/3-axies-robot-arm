#include "zmatfifo.h"
#include <QDebug>
ZMatFIFO::ZMatFIFO(qint32 iSize)
{
    this->m_freeSema=new QSemaphore(iSize);
    this->m_usedSema=new QSemaphore(0);
}
ZMatFIFO::~ZMatFIFO()
{

}
bool ZMatFIFO::ZTryPutMat(const cv::Mat &mat,int milliseconds)
{
    if(this->m_freeSema->tryAcquire(1,milliseconds))
    {
        this->m_queue.enqueue(mat);
        this->m_usedSema->release();
        return true;
    }
    return false;
}
bool ZMatFIFO::ZTryGetMat(cv::Mat &mat,int milliseconds)
{
    if(this->m_usedSema->tryAcquire(1,milliseconds))
    {
        mat=this->m_queue.dequeue().clone();
        this->m_freeSema->release();
        return true;
    }
    return false;
}
int ZMatFIFO::ZGetUsedNums()
{
    return this->m_usedSema->available();
}
int ZMatFIFO::ZGetFreeNums()
{
    return this->m_freeSema->available();
}

