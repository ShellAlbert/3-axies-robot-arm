#include "zcapturethread.h"
#include "zgblpara.h"

#include <opencv4/opencv2/core.hpp>
#include <opencv4/opencv2/highgui.hpp>
#include <opencv4/opencv2/imgproc.hpp>

#include <QDebug>
#include <QTime>
ZCaptureThread::ZCaptureThread(QString ip,ZMatFIFO *fifo)
{
    this->m_ip=ip;
    this->m_fifo=fifo;
}
void ZCaptureThread::run()
{
    cv::VideoCapture cap;
    QImage img;
    qint32 iSkipFrm=0;
    while(!gGblPara.m_bExitFlag)
    {
        if(!cap.isOpened())
        {
            qDebug()<<"connection lost!";
            QString rtspAddr=QString("rtsp://%1:554/user=admin&password=&channel=1&stream=0.sdp?real_stream").arg(this->m_ip);
            if(cap.open(rtspAddr.toStdString()))
            {
                double dRate=0;//cap.get(CV_CAP_PROP_FPS);
                qDebug()<<"rate:"<<dRate;
            }else{
                qDebug()<<"open rtsp failed.";
                this->sleep(5);
                continue;
            }
        }
        cv::Mat  mat;
        cap>>mat;
        if(!mat.data)
        {
            qDebug()<<"failed to get rtsp!";
            cap.release();
            this->sleep(5);
            continue;
        }
        //convert mat to QImage for local display.
        img=cvMat2QImage(mat);
        emit this->ZSigNewImg(img);

        //put cvMat to FIFO under track mode.
        if(gGblPara.m_appMode==Track_Mode)
        {
            if(iSkipFrm++>=5)
            {
                this->m_fifo->ZAddFrame(mat);
                iSkipFrm=0;
            }
        }
        this->usleep(100);
    }
    cap.release();
}

