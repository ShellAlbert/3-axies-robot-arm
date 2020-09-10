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
    bool ZAddFrame(const cv::Mat &frame);
    bool ZGetFrame(cv::Mat &frame);
    void ZClearFIFO();
    qint32 ZGetSize();
private:
    QMutex m_mutex;
    QQueue<cv::Mat> m_queue;
    QSemaphore *m_freeSema;
    QSemaphore *m_usedSema;
    qint32 m_iSize;
};

#endif // ZMATFIFO_H
