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

    //for coordinate mapping.
    cv::Mat matLftROI,matLftMatched;
    cv::Mat matRhtROI,matRhtMatched;
    cv::Mat matTopROI,matTopMatched;
    cv::Mat matBtmROI,matBtmMatched;
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
                //set flag.
                gGblPara.m_bObjectLocked=true;

                //draw the tracked object.
                cv::rectangle(mat,roi,cv::Scalar(255,255,255),2,1);

                //calculate the track diff x&y.
                int iOrgCenterX=mat.cols/2-roi.width/2;
                int iOrgCenterY=mat.rows/2-roi.height/2;
                gGblPara.m_trackDiffX=(roi.x+roi.width/2)-iOrgCenterX;
                gGblPara.m_trackDiffY=(roi.y+roi.height/2)-iOrgCenterY;
            }else{
                //tracking failed.
                //set flag.
                gGblPara.m_bObjectLocked=false;
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
        cv::Rect rectCenter(mat.cols/2-iBoxWidth/2,mat.rows/2-iBoxHeight/2,iBoxWidth,iBoxHeight);
        //define the draw area form ROI & matched.
        cv::Mat drawLftROI(mat,cv::Rect(0,0,iBoxWidth,iBoxHeight));
        cv::Mat drawLftMatched(mat,cv::Rect(iBoxWidth,0,iBoxWidth,iBoxHeight));
        cv::Mat drawRhtROI(mat,cv::Rect(0,iBoxHeight*1,iBoxWidth,iBoxHeight));
        cv::Mat drawRhtMatched(mat,cv::Rect(iBoxWidth,iBoxHeight*1,iBoxWidth,iBoxHeight));
        cv::Mat drawTopROI(mat,cv::Rect(0,iBoxHeight*2,iBoxWidth,iBoxHeight));
        cv::Mat drawTopMatched(mat,cv::Rect(iBoxWidth,iBoxHeight*2,iBoxWidth,iBoxHeight));
        cv::Mat drawBtmROI(mat,cv::Rect(0,iBoxHeight*3,iBoxWidth,iBoxHeight));
        cv::Mat drawBtmMatched(mat,cv::Rect(iBoxWidth,iBoxHeight*3,iBoxWidth,iBoxHeight));
        switch(gGblPara.m_CalibrateFSM)
        {
        case FSM_Calibrate_Start:
            break;
        case FSM_Calibrate_Left:
        {
            //dynamic indicate the left ROI.
            cv::Rect rectLeft(0,mat.rows/2-iBoxHeight/2,iBoxWidth,iBoxHeight);
            cv::rectangle(mat,rectLeft,cv::Scalar(255,0,0),2);
            matLftROI=cv::Mat(mat,rectLeft);

            //draw the left ROI.
            cv::copyTo(matLftROI,drawLftROI,matLftROI);
        }
            break;
        case FSM_Calibrate_LeftConfirm:
        {
            //select the current matched mat based on center.
            matLftMatched=cv::Mat(mat,rectCenter);

            //draw the left ROI.
            cv::copyTo(matLftROI,drawLftROI,matLftROI);
            //draw the left matched.
            cv::copyTo(matLftMatched,drawLftMatched,matLftMatched);
        }
            break;
        case FSM_Calibrate_Right:
        {
            //first draw the previous mateched result.
            //draw the left ROI.
            cv::copyTo(matLftROI,drawLftROI,matLftROI);
            //draw the left matched.
            cv::copyTo(matLftMatched,drawLftMatched,matLftMatched);
            ////////////////////////////////////////////////////////////////

            //dynamic indicate the right ROI.
            cv::Rect rectRight(mat.cols-iBoxWidth,mat.rows/2-iBoxHeight/2,iBoxWidth,iBoxHeight);
            cv::rectangle(mat,rectRight,cv::Scalar(255,0,0),2);
            matRhtROI=cv::Mat(mat,rectRight);
            //draw the right ROI.
            cv::copyTo(matRhtROI,drawRhtROI,matRhtROI);
        }
            break;
        case FSM_Calibrate_RightConfirm:
        {
            //first draw the previous mateched result.
            //draw the left ROI.
            cv::copyTo(matLftROI,drawLftROI,matLftROI);
            //draw the left matched.
            cv::copyTo(matLftMatched,drawLftMatched,matLftMatched);
            ////////////////////////////////////////////////////////////////

            //select the current matched mat based on center.
            matRhtMatched=cv::Mat(mat,rectCenter);

            //draw the right ROI.
            cv::copyTo(matRhtROI,drawRhtROI,matRhtROI);
            //draw the right matched.
            cv::copyTo(matRhtMatched,drawRhtMatched,matRhtMatched);
        }
            break;
        case FSM_Calibrate_Top:
        {
            //first draw the previous mateched result.
            //draw the left ROI.
            cv::copyTo(matLftROI,drawLftROI,matLftROI);
            //draw the left matched.
            cv::copyTo(matLftMatched,drawLftMatched,matLftMatched);
            //draw the right ROI.
            cv::copyTo(matRhtROI,drawRhtROI,matRhtROI);
            //draw the right matched.
            cv::copyTo(matRhtMatched,drawRhtMatched,matRhtMatched);
            ///////////////////////////////////////////////////////////

            //dynamic indicate the top ROI.
            cv::Rect rectTop(mat.cols/2,0,iBoxWidth,iBoxHeight);
            cv::rectangle(mat,rectTop,cv::Scalar(255,0,0),2);
            matTopROI=cv::Mat(mat,rectTop);

            //draw the top ROI.
            cv::copyTo(matTopROI,drawTopROI,matTopROI);
        }
            break;
        case FSM_Calibrate_TopConfirm:
        {
            //first draw the previous mateched result.
            //draw the left ROI.
            cv::copyTo(matLftROI,drawLftROI,matLftROI);
            //draw the left matched.
            cv::copyTo(matLftMatched,drawLftMatched,matLftMatched);
            //draw the right ROI.
            cv::copyTo(matRhtROI,drawRhtROI,matRhtROI);
            //draw the right matched.
            cv::copyTo(matRhtMatched,drawRhtMatched,matRhtMatched);
            ///////////////////////////////////////////////////////////

            //select the current matched mat based on center.
            matTopMatched=cv::Mat(mat,rectCenter);

            //draw the top ROI.
            cv::copyTo(matTopROI,drawTopROI,matTopROI);
            //draw the top matched.
            cv::copyTo(matTopMatched,drawTopMatched,matTopMatched);
        }
            break;
        case FSM_Calibrate_Bottom:
        {
            //first draw the previous mateched result.
            //draw the left ROI.
            cv::copyTo(matLftROI,drawLftROI,matLftROI);
            //draw the left matched.
            cv::copyTo(matLftMatched,drawLftMatched,matLftMatched);
            //draw the right ROI.
            cv::copyTo(matRhtROI,drawRhtROI,matRhtROI);
            //draw the right matched.
            cv::copyTo(matRhtMatched,drawRhtMatched,matRhtMatched);
            //draw the top ROI.
            cv::copyTo(matTopROI,drawTopROI,matTopROI);
            //draw the top matched.
            cv::copyTo(matTopMatched,drawTopMatched,matTopMatched);
            ///////////////////////////////////////////////////////////

            //dynamic indicate the bottom ROI.
            cv::Rect rectBtm(mat.cols/2,mat.rows-iBoxHeight,iBoxWidth,iBoxHeight);
            cv::rectangle(mat,rectBtm,cv::Scalar(255,0,0),2);
            matBtmROI=cv::Mat(mat,rectBtm);

            //draw the bottom ROI.
            cv::copyTo(matBtmROI,drawBtmROI,matBtmROI);
        }
            break;
        case FSM_Calibrate_BottomConfirm:
        {
            //first draw the previous mateched result.
            //draw the left ROI.
            cv::copyTo(matLftROI,drawLftROI,matLftROI);
            //draw the left matched.
            cv::copyTo(matLftMatched,drawLftMatched,matLftMatched);
            //draw the right ROI.
            cv::copyTo(matRhtROI,drawRhtROI,matRhtROI);
            //draw the right matched.
            cv::copyTo(matRhtMatched,drawRhtMatched,matRhtMatched);
            //draw the top ROI.
            cv::copyTo(matTopROI,drawTopROI,matTopROI);
            //draw the top matched.
            cv::copyTo(matTopMatched,drawTopMatched,matTopMatched);
            ///////////////////////////////////////////////////////////

            //select the current matched mat based on center.
            matBtmMatched=cv::Mat(mat,rectCenter);

            //draw the bottom ROI.
            cv::copyTo(matBtmROI,drawBtmROI,matBtmROI);
            //draw the bottom matched.
            cv::copyTo(matBtmMatched,drawBtmMatched,matBtmMatched);
        }
            break;
        case FSM_Calibrate_Done:
            break;
        default:
            break;
        }

        if(1)//draw cross indicator +.
        {
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
        }

        //convert mat to QImage for local display.
        img=cvMat2QImage(mat);
        emit this->ZSigNewImg(img);
        this->usleep(100);
    }
}
