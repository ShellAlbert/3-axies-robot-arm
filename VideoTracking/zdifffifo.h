#ifndef ZDIFFFIFO_H
#define ZDIFFFIFO_H

#include <QQueue>
#include <QSemaphore>
#include "zgblpara.h"
class ZDiffFIFO
{
public:
    ZDiffFIFO(qint32 iSize);
    bool ZTryPutDiff(const ZDiffResult &diff,int milliseconds=1000);
    bool ZTryGetDiff(ZDiffResult &diff,int milliseconds=1000);
    int ZGetUsedNums();
    int ZGetFreeNums();
    ~ZDiffFIFO();
private:
    QQueue<ZDiffResult> m_queue;
    QSemaphore *m_freeSema;
    QSemaphore *m_usedSema;
};

#endif // ZDIFFFIFO_H
