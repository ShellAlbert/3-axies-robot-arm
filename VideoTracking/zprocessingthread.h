#ifndef ZPROCESSINGTHREAD_H
#define ZPROCESSINGTHREAD_H

#include <QThread>
#include <QImage>
#include <opencv4/opencv2/core.hpp>
#include <opencv4/opencv2/highgui.hpp>
#include <opencv4/opencv2/imgproc.hpp>
#include <opencv4/opencv2/objdetect.hpp>
#include <opencv4/opencv2/tracking.hpp>
using namespace cv;
using namespace std;
class ZMatFIFO;
class ZProcessingThread : public QThread
{
    Q_OBJECT
public:
    ZProcessingThread(ZMatFIFO *fifo);
signals:
    void ZSigNewImg(const QImage &img);
    void ZSigNewInitBox(const QImage &img);
protected:
    void run();
private:
    void ZMapPixels2Encoder(cv::Mat &mat);
    void ZDrawCrossIndicator(cv::Mat &mat);
    void ZDrawOnQImage(QImage &img);
private:
    ZMatFIFO *m_fifo;
};

#endif // ZPROCESSINGTHREAD_H
