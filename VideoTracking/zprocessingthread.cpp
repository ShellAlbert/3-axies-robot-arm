#include "zprocessingthread.h"
#include "zgblpara.h"
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include "zmatfifo.h"
#include <QDebug>
using namespace cv;
ZProcessingThread::ZProcessingThread(ZMatFIFO *fifo)
{
    this->m_fifo=fifo;
}
void ZProcessingThread::run()
{
    QImage img;
    while(!gGblPara.m_bExitFlag)
    {
        cv::Mat mat=this->m_fifo->ZGetFrame();
        //cv::cvtColor(mat,mat,cv::COLOR_RGB2GRAY);
        img=cvMat2QImage(mat);
        emit this->ZSigNewImg(img);

        this->usleep(100);
    }
}
