#include "zdifffifo.h"

ZDiffFIFO::ZDiffFIFO(qint32 iSize)
{
    this->m_freeSema=new QSemaphore(iSize);
    this->m_usedSema=new QSemaphore(0);
}
bool ZDiffFIFO::ZTryPutDiff(const ZDiffResult &diff,int milliseconds)
{
    if(this->m_freeSema->tryAcquire(1,milliseconds))
    {
        this->m_queue.enqueue(diff);
        this->m_usedSema->release();
        return true;
    }
    return false;
}
bool ZDiffFIFO::ZTryGetDiff(ZDiffResult &diff,int milliseconds)
{
    if(this->m_usedSema->tryAcquire(1,milliseconds))
    {
        diff=this->m_queue.dequeue();
        this->m_freeSema->release();
        return true;
    }
    return false;
}
ZDiffFIFO::~ZDiffFIFO()
{

}
int ZDiffFIFO::ZGetUsedNums()
{
    return this->m_usedSema->available();
}
int ZDiffFIFO::ZGetFreeNums()
{
    return this->m_freeSema->available();
}
