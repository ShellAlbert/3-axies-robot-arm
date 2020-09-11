#ifndef ZMATFIFO_H
#define ZMATFIFO_H

#include <QQueue>
#include <QMutex>
#include <QSemaphore>
#include <opencv4/opencv2/highgui.hpp>
class ZMatFIFO
{
public:
    ZMatFIFO(qint32 iSize);
    bool ZTryPutMat(const cv::Mat &mat,int milliseconds=1000);
    bool ZTryGetMat(cv::Mat &mat,int milliseconds=1000);
    int ZGetUsedNums();
    int ZGetFreeNums();
    ~ZMatFIFO();
private:
    QQueue<cv::Mat> m_queue;
    QSemaphore *m_freeSema;
    QSemaphore *m_usedSema;
};

#endif // ZMATFIFO_H
