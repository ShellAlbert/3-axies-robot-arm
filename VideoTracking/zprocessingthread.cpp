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

        //resize to reduce time in tracking mode.
        //cv::resize(mat,mat,cv::Size(mat.cols/2,mat.rows/2));

        //draw the ROI rectangle(200x200).
        Rect2d roi;
        roi.width=100;
        roi.height=100;
        roi.x=mat.cols/2-roi.width/2;
        roi.y=mat.rows/2-roi.height/2;
        //cv::rectangle(mat,roi,cv::Scalar(0,255,0),1,1);

        //we draw a radius=100 circle.//RGB  //BGR
        cv::circle(mat,cv::Point(mat.cols/2,mat.rows/2),100,cv::Scalar(0xd3,0x06,0xff),4,1);
        //draw a line from left to right.
        cv::Point ptLeft1,ptLeft2,ptRight1,ptRight2;
        ptLeft1.x=mat.cols/2-100;
        ptLeft1.y=mat.rows/2;
        ptLeft2.x=mat.cols/2-20;
        ptLeft2.y=mat.rows/2;
        cv::line(mat,ptLeft1,ptLeft2,cv::Scalar(0xd3,0x06,0xff),2,1);

        ptRight1.x=mat.cols/2+20;
        ptRight1.y=mat.rows/2;
        ptRight2.x=mat.cols/2+100;
        ptRight2.y=mat.rows/2;
        cv::line(mat,ptRight1,ptRight2,cv::Scalar(0xd3,0x06,0xff),2,1);

        //draw a line from top to bottom.
        cv::Point ptTop1,ptTop2,ptBottom1,ptBottom2;
        ptTop1.x=mat.cols/2;
        ptTop1.y=mat.rows/2-100;
        ptTop2.x=mat.cols/2;
        ptTop2.y=mat.rows/2-20;
        cv::line(mat,ptTop1,ptTop2,cv::Scalar(0xd3,0x06,0xff),2,1);

        ptBottom1.x=mat.cols/2;
        ptBottom1.y=mat.rows/2+20;
        ptBottom2.x=mat.cols/2;
        ptBottom2.y=mat.rows/2+100;
        cv::line(mat,ptBottom1,ptBottom2,cv::Scalar(0xd3,0x06,0xff),2,1);

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


        //mapping pixel move to motor move,create linear relations.
        //define box size.
        int iBoxWidth=100,iBoxHeight=100;
        switch(gGblPara.m_CalibrateFSM)
        {
        case FSM_Calibrate_Start:
            break;
        case FSM_Calibrate_Left:
            //draw the left rectange for x-axis calibration.
            cv::Rect rectLeft(0,mat.rows/2-iBoxHeight/2,iBoxWidth,iBoxHeight);
            cv::rectangle(mat,rectLeft,cv::Scalar(0,255,0),2);
            break;
        case FSM_Calibrate_Right:
            //draw the right rectangle for x-axis calibration.
            cv::Rect rectRight(mat.cols-iBoxWidth,mat.rows/2-iBoxHeight/2,iBoxWidth,iBoxHeight);
            cv::rectangle(mat,rectRight,cv::Scalar(0,255,0),2);
            break;
        case FSM_Calibrate_Top:
            break;
        case FSM_Calibrate_Bottom:
            break;
        case FSM_Calibrate_Done:
            break;
        }

        //convert mat to QImage for local display.
        img=cvMat2QImage(mat);
        emit this->ZSigNewImg(img);
        this->usleep(100);
    }
}
