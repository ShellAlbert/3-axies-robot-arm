#ifndef ZMATFIFO_H
#define ZMATFIFO_H

#include <QQueue>
#include <QMutex>
#include <QSemaphore>
#include <opencv4/opencv2/highgui.hpp>
class ZMatFIFO
{
public:
    ZMatFIFO(qint32 iSize,bool bDropFrame);
    void ZAddFrame(const cv::Mat &frame);
    cv::Mat ZGetFrame();
    void ZClearFIFO();
    qint32 ZGetSize();
private:
    QMutex m_mutex;
    QQueue<cv::Mat> m_queue;
    QSemaphore *m_freeSema;
    QSemaphore *m_usedSema;
    QSemaphore *m_clearFIFO1;
    QSemaphore *m_clearFIFO2;
    qint32 m_iSize;
    bool m_bDropFrame;
};

#endif // ZMATFIFO_H
