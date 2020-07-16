#include "zprocessingthread.h"
#include "zgblpara.h"
#include "zmatfifo.h"
#include <QDebug>

#include <opencv4/opencv2/core.hpp>
#include <opencv4/opencv2/highgui.hpp>
#include <opencv4/opencv2/imgproc.hpp>
#include <opencv4/opencv2/objdetect.hpp>
#include <opencv4/opencv2/tracking.hpp>
using namespace cv;
using namespace std;
ZProcessingThread::ZProcessingThread(ZMatFIFO *fifo)
{
    this->m_fifo=fifo;
}
void ZProcessingThread::run()
{
    bool bInit=false;
    cv::Ptr<Tracker> tracker=TrackerKCF::create();
    QImage img;
    //cv::HOGDescriptor hog;
    //hog.setSVMDetector(cv::HOGDescriptor::getDefaultPeopleDetector());

    while(!gGblPara.m_bExitFlag)
    {
        cv::Mat mat=this->m_fifo->ZGetFrame();
//        img=cvMat2QImage(mat);
//        emit this->ZSigNewImg(img);

        //we do ImgProc on gray.
        //cv::cvtColor(mat,mat,cv::COLOR_RGB2GRAY);
        cv::resize(mat,mat,cv::Size(mat.cols/2,mat.rows/2));

        //draw the ROI rectangle(200x200).
        Rect2d roi;
        roi.width=100;
        roi.height=100;
        roi.x=mat.cols/2-roi.width/2;
        roi.y=mat.rows/2-roi.height/2;
        cv::rectangle(mat,roi,cv::Scalar(0,255,0),3,1);

        //define the center(x,y).
        int iOrgCenterX=mat.cols/2-roi.width/2;
        int iOrgCenterY=mat.rows/2-roi.height/2;

        if(gGblPara.m_bTrackingEnabled)
        {
            if(!bInit)
            {
                tracker->init(mat,roi);
                bInit=true;
            }
            //update the tracking result.
            if(tracker->update(mat,roi))
            {

                //draw the tracked object.
                cv::rectangle(mat,roi,cv::Scalar(255,255,255),2,1);

                //calculate the track diff x&y.
                gGblPara.m_trackDiffX=(roi.x+roi.width/2)-iOrgCenterX;
                gGblPara.m_trackDiffY=(roi.y+roi.height/2)-iOrgCenterY;
            }else{
                qDebug()<<"tracking failed.";
            }
        }else{
            bInit=false;
        }

        //detect peoples.
        //std::vector<cv::Rect> regions;
        //hog.detectMultiScale(mat,regions,0,cv::Size(8,8),cv::Size(32,32),1.05,1);
        //qDebug()<<"regions:"<<regions.size();
        //for(size_t i=0;i<regions.size();i++)
        //{
        //    cv::rectangle(mat,regions[i],cv::Scalar(0,255,0),2);
        //}

        img=cvMat2QImage(mat);
        emit this->ZSigNewImg(img);
        this->usleep(100);
    }
}
