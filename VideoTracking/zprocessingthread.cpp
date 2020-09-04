#include "zprocessingthread.h"
#include "zgblpara.h"
#include "zmatfifo.h"
#include <QDebug>
#include <QPainter>
#include <QTime>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
ZProcessingThread::ZProcessingThread(ZMatFIFO *fifo)
{
    this->m_fifo=fifo;
}
void ZProcessingThread::run()
{
    bool bInit=false;

    //use CSRT when you need higher object tracking accuracy and can tolerate slower FPS throughput.
    //cv::Ptr<Tracker> tracker=cv::TrackerCSRT::create();

    //use KCF when you need faster FPS throughtput but can handle slightly lower object tracking accuracy.
    cv::Ptr<Tracker> tracker=TrackerKCF::create();

    //use MOSSE when you need pure speed.
    //cv::Ptr<Tracker> tracker=TrackerMOSSE::create();

    //cv::TrackerBoosting::create();
    //cv::Ptr<Tracker> tracker=cv::TrackerGOTURN::create();
    //cv::TrackerMIL::create();
    //cv::TrackerMedianFlow::create();

    QImage img;
    //cv::HOGDescriptor hog;
    //hog.setSVMDetector(cv::HOGDescriptor::getDefaultPeopleDetector());

    int iOldTs,iNewTs;
    iOldTs=iNewTs=QTime::currentTime().msecsSinceStartOfDay();
    cv::Rect2d rectROI;
    cv::Mat  matROI;
    int iDiffX_Old,iDiffY_Old;
    while(!gGblPara.m_bExitFlag)
    {
        cv::Mat mat=this->m_fifo->ZGetFrame();
        //we do ImgProc on gray.
        //cv::cvtColor(mat,mat,cv::COLOR_RGB2GRAY);

        //stretch to 1/2 to speed up(includes ROI box coordinate(x,y,width,height).
        //resize to 1/2 size to reduce time.
        cv::resize(mat,mat,cv::Size(mat.cols/2,mat.rows/2));

        //define ROI rectangle(200x200) on center point(0,0).
        //cv::rectangle(mat,roi,cv::Scalar(0,255,0),1,1);
        if(!gGblPara.m_bTrackInit)
        {
            //stretch to 1/2 to speed up(includes ROI box coordinate(x,y,width,height).
            rectROI.x=gGblPara.m_rectROI.x/2;
            rectROI.y=gGblPara.m_rectROI.y/2;
            rectROI.width=gGblPara.m_rectROI.width/2;
            rectROI.height=gGblPara.m_rectROI.height/2;
            //initial.
            tracker->clear();
            tracker=TrackerKCF::create();
            if(tracker->init(mat,rectROI))
            {
                qDebug()<<"track Init okay";
            }else{
                qDebug()<<"track Init error";
            }

            //save selected ROI & show selected ROI on UI.
            matROI=mat(rectROI);
            img=cvMat2QImage(matROI);
            emit this->ZSigInitBox(img);

            gGblPara.m_bTrackInit=true;
            qDebug()<<"reInit ROI";
        }else{
            //update the tracking result.
            if(tracker->update(mat,rectROI))
            {
                //calculate the msec.
                iNewTs=QTime::currentTime().msecsSinceStartOfDay();
                gGblPara.m_iCostMSec=iNewTs-iOldTs;
                iOldTs=iNewTs;

                //draw the tracked object.
                //restore original size(*2).
                QRect rectLocked;
                rectLocked.setX(rectROI.x*2);
                rectLocked.setY(rectROI.y*2);
                rectLocked.setWidth(rectROI.width*2);
                rectLocked.setHeight(rectROI.height*2);
                emit this->ZSigLocked(true,rectLocked);
                //qDebug()<<"locked:"<<rectLocked;
                //fps.
                gGblPara.m_iFps=this->getFps();

                //calculate the track diff x&y.
                //restore original size(*2).
                int diffX=((rectROI.x+rectROI.width/2)-mat.cols/2)*2;
                int diffY=((rectROI.y+rectROI.height/2)-mat.rows/2)*2;
                if(diffX!=iDiffX_Old || diffY!=iDiffY_Old)
                {
                    this->ZHandlePixelDiff(diffX,diffY);
                    iDiffX_Old=diffX;
                    iDiffY_Old=diffY;
                }
            }else{
                //tracking failed.
                emit this->ZSigLocked(false,QRect());
            }
        }
        this->usleep(10);
    }
}
void ZProcessingThread::ZTrackObject(cv::Mat &mat)
{

}
void ZProcessingThread::ZHandlePixelDiff(int diffX,int diffY)
{
    static int iInitFlag=0;
    if(!iInitFlag)
    {
        iInitFlag=1;
        srand(time(0));
    }
    //generate random absolute position for testing.
    int a=-20000,b=20000;
    char buffer[256];
    int pos=rand()%(b-a+1)+a;
    int pos2=rand()%(b-a+1)+a;
    int len;
    //Profile Position Mode - Absolute Position.
    sprintf(buffer,"abs_pos=%d,%d\n",pos,pos2);
    //Profile Position Mode - Relative Position.
    //sprintf(buffer,"rel_pos=%d,%d\n",pos,pos2);
    len=strlen(buffer);
    write(gGblPara.m_fdServoFIFOIn,(void*)&len,sizeof(len));
    write(gGblPara.m_fdServoFIFOIn,(void*)buffer,len);
    qDebug("%d:%s\n",len,buffer);
}
qint32 ZProcessingThread::getFps()
{
    static qint32 iFps=0;
    static qint32 iLastMSec=QTime::currentTime().msecsSinceStartOfDay();
    static qint32 iFrameCount=0;

    ++iFrameCount;
    qint32 iNowMSec=QTime::currentTime().msecsSinceStartOfDay();
    if(iNowMSec-iLastMSec>1000)
    {
        iFps=iFrameCount;
        iFrameCount=0;
        iLastMSec=iNowMSec;
    }
    return iFps;
}
