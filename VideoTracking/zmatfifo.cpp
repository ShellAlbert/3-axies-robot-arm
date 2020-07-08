#include "zmatfifo.h"
#include <QDebug>
ZMatFIFO::ZMatFIFO(qint32 iSize,bool bDropFrame)
{
    this->m_freeSema=new QSemaphore(iSize);
    this->m_usedSema=new QSemaphore(0);
    this->m_clearFIFO1=new QSemaphore(1);
    this->m_clearFIFO2=new QSemaphore(1);
    this->m_iSize=iSize;
    this->m_bDropFrame=bDropFrame;
}
void ZMatFIFO::ZAddFrame(const cv::Mat &frame)
{
    this->m_clearFIFO1->acquire();

    if(this->m_bDropFrame)//drop frame enabled.
    {
        if(this->m_freeSema->tryAcquire())
        {
            this->m_mutex.lock();
            this->m_queue.enqueue(frame);
            this->m_mutex.unlock();

            this->m_usedSema->release();
        }
    }else{ //drop frame disabled,wait.
        this->m_freeSema->acquire();
        this->m_mutex.lock();
        this->m_queue.enqueue(frame);
        this->m_mutex.unlock();

        this->m_usedSema->release();
    }

    this->m_clearFIFO1->release();
}
cv::Mat ZMatFIFO::ZGetFrame()
{
    this->m_clearFIFO2->acquire();

    this->m_usedSema->acquire();
    cv::Mat matTemp;
    this->m_mutex.lock();
    matTemp=this->m_queue.dequeue();
    this->m_mutex.unlock();
    this->m_freeSema->release();

    this->m_clearFIFO2->release();

    return matTemp.clone();
}
void ZMatFIFO::ZClearFIFO()
{
    if(this->m_queue.size()!=0)
    {
        //stop adding frames to FIFO.
        this->m_clearFIFO1->acquire();
        //stop taking frames from FIFO.
        this->m_clearFIFO2->acquire();

        //release all remaining slots in queue.
        this->m_freeSema->release(this->m_queue.size());

        //acquire all queue slots.
        this->m_freeSema->acquire(this->m_iSize);

        //reset usedSlots to zero.
        this->m_usedSema->acquire(this->m_queue.size());

        //clear fifo.
        this->m_queue.clear();

        //release all slots.
        this->m_freeSema->release(this->m_iSize);

        //allow getFrames() to resume.
        this->m_clearFIFO1->release();
        //allow addFrame() to resume.
        this->m_clearFIFO2->release();

    }
}
qint32 ZMatFIFO::ZGetSize()
{
    return this->m_queue.size();
}
