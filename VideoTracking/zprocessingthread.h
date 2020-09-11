#ifndef ZPROCESSINGTHREAD_H
#define ZPROCESSINGTHREAD_H

#include <QThread>
#include <QImage>
#include <opencv4/opencv2/core.hpp>
#include <opencv4/opencv2/highgui.hpp>
#include <opencv4/opencv2/imgproc.hpp>
#include <opencv4/opencv2/objdetect.hpp>
#include <opencv4/opencv2/tracking.hpp>
#include "zdifffifo.h"
using namespace cv;
using namespace std;
class ZMatFIFO;
class ZDiffFIFO;
class ZProcessingThread : public QThread
{
    Q_OBJECT
public:
    ZProcessingThread(ZMatFIFO *fifo,ZDiffFIFO *fifoDIFF);
signals:
    void ZSigInitBox(const QImage &img);
    void ZSigLocked(bool bLocked,const QRect &rect);
    void ZSigDiffXY(int diffX,int diffY);
    void ZSigMatAvailable(int nums);
protected:
    void run();
private:
    void ZTrackObject(cv::Mat &mat);
    qint32 getFps();
private:
    ZMatFIFO *m_fifo;
    ZDiffFIFO *m_fifoDIFF;
};

#endif // ZPROCESSINGTHREAD_H
