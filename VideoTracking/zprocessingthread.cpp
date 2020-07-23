#include "zprocessingthread.h"
#include "zgblpara.h"
#include "zmatfifo.h"
#include <QDebug>
#include <QPainter>
#include <QTime>
ZProcessingThread::ZProcessingThread(ZMatFIFO *fifo)
{
    this->m_fifo=fifo;
}
void ZProcessingThread::run()
{
    bool bInit=false;

    //use CSRT when you need higher object tracking accuracy and can tolerate slower FPS throughput.
    cv::Ptr<Tracker> tracker=cv::TrackerCSRT::create();

    //use KCF when you need faster FPS throughtput but can handle slightly lower object tracking accuracy.
    //cv::Ptr<Tracker> tracker=TrackerKCF::create();

    //use MOSSE when you need pure speed.
    //cv::Ptr<Tracker> tracker=TrackerMOSSE::create();

    //cv::TrackerBoosting::create();
    //cv::Ptr<Tracker> tracker=cv::TrackerGOTURN::create();
    //cv::TrackerMIL::create();
    //cv::TrackerMedianFlow::create();

    QImage img;
    //cv::HOGDescriptor hog;
    //hog.setSVMDetector(cv::HOGDescriptor::getDefaultPeopleDetector());

    cv::Mat matSelectedROI;

    int iOldTs,iNewTs;
    iOldTs=iNewTs=QTime::currentTime().msecsSinceStartOfDay();
    while(!gGblPara.m_bExitFlag)
    {
        cv::Mat mat=this->m_fifo->ZGetFrame();
        //we do ImgProc on gray.
        cv::cvtColor(mat,mat,cv::COLOR_RGB2GRAY);

        //resize to reduce time in tracking mode.
        //cv::resize(mat,mat,cv::Size(800,600));

        //define ROI rectangle(200x200) on center point(0,0).
        Rect2d roi;
        roi.width=200;
        roi.height=200;
        roi.x=mat.cols/2-roi.width/2;
        roi.y=mat.rows/2-roi.height/2;
        //cv::rectangle(mat,roi,cv::Scalar(0,255,0),1,1);

        if(gGblPara.m_bTrackingEnabled)
        {
            if(!bInit)
            {
                tracker->init(mat,roi);
                bInit=true;

                //save the selected ROI.
                matSelectedROI=cv::Mat(mat,roi);
            }
            //update the tracking result.
            if(tracker->update(mat,roi))
            {
                //set flag.
                gGblPara.m_bTargetLocked=true;

                //calculate the msec.
                iNewTs=QTime::currentTime().msecsSinceStartOfDay();
                gGblPara.m_iCostMSec=iNewTs-iOldTs;
                iOldTs=iNewTs;

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
                gGblPara.m_bTargetLocked=false;
            }
            //draw the selected ROI image on the left-top corner.
            cv::Mat matDisplayed=cv::Mat(mat,cv::Rect(0,0,200,200));
            cv::copyTo(matSelectedROI,matDisplayed,matSelectedROI);
        }else{
            bInit=false;
        }

        //mapping pixels to encoder.
        //this->ZMapPixels2Encoder(mat);

        //draw cross indicator +.
        //this->ZDrawCrossIndicator(mat);

        //convert mat to QImage for local display.
        //img=cvMat2QImage(mat);

        //draw something on QImage.
        //this->ZDrawOnQImage(img);

        //emit this->ZSigNewImg(img);
        this->usleep(100);
    }
}
void ZProcessingThread::ZTrackObject(cv::Mat &mat)
{

}
void ZProcessingThread::ZMapPixels2Encoder(cv::Mat &mat)
{
    //mapping pixel diff to encoder diff,create linear relations.
    //define box size.
    int iBoxWidth=100,iBoxHeight=100;
    cv::Rect rectCenter(mat.cols/2-iBoxWidth/2,mat.rows/2-iBoxHeight/2,iBoxWidth,iBoxHeight);

    //define four ROI area: left,right,top,bottom.
    cv::Rect rectROILft(0,mat.rows/2-iBoxHeight/2,iBoxWidth,iBoxHeight);
    cv::Rect rectROIRht(mat.cols-iBoxWidth,mat.rows/2-iBoxHeight/2,iBoxWidth,iBoxHeight);
    cv::Rect rectROITop(mat.cols/2,0,iBoxWidth,iBoxHeight);
    cv::Rect rectROIBtm(mat.cols/2,mat.rows-iBoxHeight,iBoxWidth,iBoxHeight);

    //define the source cvMat for ROI & matched.
    static cv::Mat srcLftROI,srcLftMatched;
    static cv::Mat srcRhtROI,srcRhtMatched;
    static cv::Mat srcTopROI,srcTopMatched;
    static cv::Mat srcBtmROI,srcBtmMatched;

    //define destination cvMat for ROI & matched.
    cv::Mat dstLftROI(mat,cv::Rect(0,0,iBoxWidth,iBoxHeight));
    cv::Mat dstLftMatched(mat,cv::Rect(iBoxWidth,0,iBoxWidth,iBoxHeight));
    cv::Mat dstRhtROI(mat,cv::Rect(0,iBoxHeight*1,iBoxWidth,iBoxHeight));
    cv::Mat dstRhtMatched(mat,cv::Rect(iBoxWidth,iBoxHeight*1,iBoxWidth,iBoxHeight));
    cv::Mat dstTopROI(mat,cv::Rect(0,iBoxHeight*2,iBoxWidth,iBoxHeight));
    cv::Mat dstTopMatched(mat,cv::Rect(iBoxWidth,iBoxHeight*2,iBoxWidth,iBoxHeight));
    cv::Mat dstBtmROI(mat,cv::Rect(0,iBoxHeight*3,iBoxWidth,iBoxHeight));
    cv::Mat dstBtmMatched(mat,cv::Rect(iBoxWidth,iBoxHeight*3,iBoxWidth,iBoxHeight));

    //pixel coordinate & encoder coordinate values.
    static QPoint ptPixCenter,ptEncCenter;
    static QPoint ptPixLft,ptEncLft;
    static QPoint ptPixRht,ptEncRht;
    static QPoint ptPixTop,ptEncTop;
    static QPoint ptPixBtm,ptEncBtm;

    //process the finite state machine.
    switch(gGblPara.m_CalibrateFSM)
    {
    case FSM_Calibrate_Start:
        //remember pixel(x,y) and encoder(x axis and y axis) of center point.
        ptPixCenter=QPoint(mat.cols/2,mat.rows/2);
        ptEncCenter=QPoint(gGblPara.m_iXAxisCurPos,gGblPara.m_iYAxisCurPos);
        break;
    case FSM_Calibrate_Left:
    {
        //draw a rectangle to indicate the left ROI.
        cv::rectangle(mat,rectROILft,cv::Scalar(255,0,0),2);

        //save the source left ROI.
        srcLftROI=cv::Mat(mat,rectROILft);

        //draw the left ROI on left-top corner.
        cv::copyTo(srcLftROI,dstLftROI,srcLftROI);

#if 0
        //(xx,xx)/(xx,xx)->(xx,xx)/(xx,xx)  (0,iBoxHeight*4)
        //(xx,xx)/(xx,xx)->(xx,xx)/(xx,xx)  (0,iBoxHeight*3)
        //(xx,xx)/(xx,xx)->(xx,xx)/(xx,xx)  (0,iBoxHeight*2)
        //(xx,xx)/(xx,xx)->(xx,xx)/(xx,xx)  (0,iBoxHeight*1)
        cv::Point ptOrg[4]={{0,mat.rows-iBoxHeight*4},{0,mat.rows-iBoxHeight*3},{0,mat.rows-iBoxHeight*2},{0,mat.rows-iBoxHeight*1}};
        //get text height.
        cv::Size textSize=cv::getTextSize("(12,34)",cv::FONT_HERSHEY_COMPLEX,1,1,NULL);
        //draw the left calibrate area data (pixel & encoder value).
        char bufferLeft[128];
        sprintf(bufferLeft,"(%d,%d)/(%d,%d)->(?,?)/(?,?)",///<
                rectROILft.x+rectROILft.width/2,///<pixel x.
                rectROILft.y+rectROILft.height/2,///<pixel y.
                gGblPara.m_iXAxisCurPos,///<encoder x value.
                gGblPara.m_iYAxisCurPos///<encoder y value.
                );
        string strPixLft(bufferLeft);
        ptOrg[0].y+=(iBoxHeight-textSize.height)/2;//center it.
        cv::putText(mat,strPixLft,ptOrg[0],cv::FONT_HERSHEY_COMPLEX,1,cv::Scalar(0,0,255),1);
#endif
    }
        break;
    case FSM_Calibrate_LeftConfirm:
    {
        //select the current matched mat based on center.
        srcLftMatched=cv::Mat(mat,rectCenter);

        //draw the left ROI on left-top corner.
        cv::copyTo(srcLftROI,dstLftROI,srcLftROI);
        //draw the left matched on the left-top corner.
        cv::copyTo(srcLftMatched,dstLftMatched,srcLftMatched);

        //save the encoder value of left ROI.
        ptEncLft=QPoint(gGblPara.m_iXAxisCurPos,gGblPara.m_iYAxisCurPos);
        //save the center point of left ROI.
        ptPixLft=QPoint(rectROILft.x+rectROILft.width/2,rectROILft.y+rectROILft.height/2);

#if 0
        //(xx,xx)/(xx,xx)->(xx,xx)/(xx,xx)  (0,iBoxHeight*4)
        //(xx,xx)/(xx,xx)->(xx,xx)/(xx,xx)  (0,iBoxHeight*3)
        //(xx,xx)/(xx,xx)->(xx,xx)/(xx,xx)  (0,iBoxHeight*2)
        //(xx,xx)/(xx,xx)->(xx,xx)/(xx,xx)  (0,iBoxHeight*1)
        cv::Point ptOrg[4]={{0,mat.rows-iBoxHeight*4},{0,mat.rows-iBoxHeight*3},{0,mat.rows-iBoxHeight*2},{0,mat.rows-iBoxHeight*1}};
        //get text height.
        cv::Size textSize=cv::getTextSize("(12,34)",cv::FONT_HERSHEY_COMPLEX,1,1,NULL);
        //draw the left calibrate area data (pixel & encoder value).
        char bufferLeft[128];
        sprintf(bufferLeft,"(%d,%d)/(%d,%d)->(?,?)/(?,?)",///<
                rectROILft.x+rectROILft.width/2,///<pixel x.
                rectROILft.y+rectROILft.height/2,///<pixel y.
                gGblPara.m_iXAxisCurPos,///<encoder x value.
                gGblPara.m_iYAxisCurPos///<encoder y value.
                );
        string strPixLft(bufferLeft);
        ptOrg[0].y+=(iBoxHeight-textSize.height)/2;//center it.
        cv::putText(mat,strPixLft,ptOrg[0],cv::FONT_HERSHEY_COMPLEX,1,cv::Scalar(0,0,255),1);
#endif
    }
        break;
    case FSM_Calibrate_Right:
    {
        //first draw the previous mateched result.
        //draw the left ROI on left-top corner.
        cv::copyTo(srcLftROI,dstLftROI,srcLftROI);
        //draw the left matched on left-top corner.
        cv::copyTo(srcLftMatched,dstLftMatched,srcLftMatched);

        ////////////////////////////////////////////////////////////////
        //draw a rectangle to indicate the right ROI.
        cv::rectangle(mat,rectROIRht,cv::Scalar(255,0,0),2);
        //save the source right ROI.
        srcRhtROI=cv::Mat(mat,rectROIRht);

        //draw the right ROI on left-top corner.
        cv::copyTo(srcRhtROI,dstRhtROI,srcRhtROI);
    }
        break;
    case FSM_Calibrate_RightConfirm:
    {
        //first draw the previous mateched result.
        //draw the left ROI on left-top corner.
        cv::copyTo(srcLftROI,dstLftROI,srcLftROI);
        //draw the left matched on left-top corner.
        cv::copyTo(srcLftMatched,dstLftMatched,srcLftMatched);
        //draw the right ROI on left-top corner.
        cv::copyTo(srcRhtROI,dstRhtROI,srcRhtROI);
        ////////////////////////////////////////////////////////////////

        //select the current matched mat based on center.
        srcRhtMatched=cv::Mat(mat,rectCenter);

        //draw the right matched on the left-top corner.
        cv::copyTo(srcRhtMatched,dstRhtMatched,srcRhtMatched);

        //save the encoder value of right ROI.
        ptEncRht=QPoint(gGblPara.m_iXAxisCurPos,gGblPara.m_iYAxisCurPos);
        //save the center point of right ROI.
        ptPixRht=QPoint(rectROIRht.x+rectROIRht.width/2,rectROIRht.y+rectROIRht.height/2);
    }
        break;
    case FSM_Calibrate_Top:
    {
        //first draw the previous mateched result.
        //draw the left ROI on left-top corner.
        cv::copyTo(srcLftROI,dstLftROI,srcLftROI);
        //draw the left matched on left-top corner.
        cv::copyTo(srcLftMatched,dstLftMatched,srcLftMatched);
        //draw the right ROI on left-top corner.
        cv::copyTo(srcRhtROI,dstRhtROI,srcRhtROI);
        //draw the right matched on the left-top corner.
        cv::copyTo(srcRhtMatched,dstRhtMatched,srcRhtMatched);
        ///////////////////////////////////////////////////////////

        //draw a rectangle to indicate the top ROI.
        cv::rectangle(mat,rectROITop,cv::Scalar(255,0,0),2);

        //save the source top ROI.
        srcTopROI=cv::Mat(mat,rectROITop);

        //draw the top ROI on left-top corner.
        cv::copyTo(srcTopROI,dstTopROI,srcTopROI);
    }
        break;
    case FSM_Calibrate_TopConfirm:
    {
        //first draw the previous mateched result.
        //draw the left ROI on left-top corner.
        cv::copyTo(srcLftROI,dstLftROI,srcLftROI);
        //draw the left matched on left-top corner.
        cv::copyTo(srcLftMatched,dstLftMatched,srcLftMatched);
        //draw the right ROI on left-top corner.
        cv::copyTo(srcRhtROI,srcRhtROI,srcRhtROI);
        //draw the right matched on the left-top corner.
        cv::copyTo(srcRhtMatched,dstRhtMatched,srcRhtMatched);
        //draw the top ROI on left-top corner.
        cv::copyTo(srcTopROI,dstTopROI,srcTopROI);
        ///////////////////////////////////////////////////////////

        //select the current matched mat based on center.
        srcTopMatched=cv::Mat(mat,rectCenter);

        //draw the top matched on the left-top corner.
        cv::copyTo(srcTopMatched,dstTopMatched,srcTopMatched);

        //remember the encoder top.
        ptEncTop=QPoint(gGblPara.m_iXAxisCurPos,gGblPara.m_iYAxisCurPos);
        //remember the center point of top.
        ptPixTop=QPoint(rectROITop.x+rectROITop.width/2,rectROITop.y+rectROITop.height/2);
    }
        break;
    case FSM_Calibrate_Bottom:
    {
        //first draw the previous mateched result.
        //draw the left ROI on left-top corner.
        cv::copyTo(srcLftROI,dstLftROI,srcLftROI);
        //draw the left matched on left-top corner.
        cv::copyTo(srcLftMatched,dstLftMatched,srcLftMatched);
        //draw the right ROI on left-top corner.
        cv::copyTo(srcRhtROI,srcRhtROI,srcRhtROI);
        //draw the right matched on the left-top corner.
        cv::copyTo(srcRhtMatched,dstRhtMatched,srcRhtMatched);
        //draw the top ROI on left-top corner.
        cv::copyTo(srcTopROI,dstTopROI,srcTopROI);
        //draw the top matched on the left-top corner.
        cv::copyTo(srcTopMatched,dstTopMatched,srcTopMatched);
        ///////////////////////////////////////////////////////////

        //draw a rectangle to indicate the bottom ROI.
        cv::rectangle(mat,rectROIBtm,cv::Scalar(255,0,0),2);
        //save the source top ROI.
        srcBtmROI=cv::Mat(mat,rectROIBtm);

        //draw the bottom ROI on left-top corner.
        cv::copyTo(srcBtmROI,dstBtmROI,srcBtmROI);
    }
        break;
    case FSM_Calibrate_BottomConfirm:
    {
        //first draw the previous mateched result.
        //draw the left ROI on left-top corner.
        cv::copyTo(srcLftROI,dstLftROI,srcLftROI);
        //draw the left matched on left-top corner.
        cv::copyTo(srcLftMatched,dstLftMatched,srcLftMatched);
        //draw the right ROI on left-top corner.
        cv::copyTo(srcRhtROI,srcRhtROI,srcRhtROI);
        //draw the right matched on the left-top corner.
        cv::copyTo(srcRhtMatched,dstRhtMatched,srcRhtMatched);
        //draw the top ROI on left-top corner.
        cv::copyTo(srcTopROI,dstTopROI,srcTopROI);
        //draw the top matched on the left-top corner.
        cv::copyTo(srcTopMatched,dstTopMatched,srcTopMatched);
        //draw the bottom ROI on left-top corner.
        cv::copyTo(srcBtmROI,dstBtmROI,srcBtmROI);
        ///////////////////////////////////////////////////////////

        //select the current matched mat based on center.
        srcBtmMatched=cv::Mat(mat,rectCenter);

        //draw the bottom matched on the left-top corner.
        cv::copyTo(srcBtmMatched,dstBtmMatched,srcBtmMatched);

        //save the encoder value of bottom ROI.
        ptEncBtm=QPoint(gGblPara.m_iXAxisCurPos,gGblPara.m_iYAxisCurPos);
        //save the center point of bottom ROI.
        ptPixBtm=QPoint(rectROIBtm.x+rectROIBtm.width/2,rectROIBtm.y+rectROIBtm.height/2);
    }
        break;
    case FSM_Calibrate_Done:
        qDebug("Lft: Pix(%d,%d)->(%d,%d),Diff=[%d,%d] Enc(%d,%d)->(%d,%d),Diff=[%d,%d]\n",///<
               rectCenter.x+rectCenter.width/2,rectCenter.y+rectCenter.height/2,///< center point.
               ptPixLft.x(),ptPixLft.y(),///< left ROI new point.
               ptPixLft.x()-(rectCenter.x+rectCenter.width/2),ptPixLft.y()-(rectCenter.y+rectCenter.height/2),///< diff.
               ptEncCenter.x(),ptEncCenter.y(),///<center encoder value.
               ptEncLft.x(),ptEncLft.y(),///< left ROI encoder value.
               ptEncLft.x()-ptEncCenter.x(),ptEncLft.y()-ptEncCenter.y());///<diff.

        qDebug("Rht: Pix(%d,%d)->(%d,%d),Diff=[%d,%d] Enc(%d,%d)->(%d,%d),Diff=[%d,%d]\n",///<
               rectCenter.x+rectCenter.width/2,rectCenter.y+rectCenter.height/2,///< center point.
               ptPixRht.x(),ptPixRht.y(),///< right ROI new point.
               ptPixRht.x()-(rectCenter.x+rectCenter.width/2),ptPixRht.y()-(rectCenter.y+rectCenter.height/2),///< diff.
               ptEncCenter.x(),ptEncCenter.y(),///<center encoder value.
               ptEncRht.x(),ptEncRht.y(),///< right ROI encoder value.
               ptEncRht.x()-ptEncCenter.x(),ptEncRht.y()-ptEncCenter.y());///<diff.

        qDebug("Top: Pix(%d,%d)->(%d,%d),Diff=[%d,%d] Enc(%d,%d)->(%d,%d),Diff=[%d,%d]\n",///<
               rectCenter.x+rectCenter.width/2,rectCenter.y+rectCenter.height/2,///< center point.
               ptPixTop.x(),ptPixTop.y(),///< top ROI new point.
               ptPixTop.x()-(rectCenter.x+rectCenter.width/2),ptPixTop.y()-(rectCenter.y+rectCenter.height/2),///< diff.
               ptEncCenter.x(),ptEncCenter.y(),///<center encoder value.
               ptEncTop.x(),ptEncTop.y(),///< top ROI encoder value.
               ptEncTop.x()-ptEncCenter.x(),ptEncTop.y()-ptEncCenter.y());///<diff.

        qDebug("Btm: Pix(%d,%d)->(%d,%d),Diff=[%d,%d] Enc(%d,%d)->(%d,%d),Diff=[%d,%d]\n",///<
               rectCenter.x+rectCenter.width/2,rectCenter.y+rectCenter.height/2,///< center point.
               ptPixBtm.x(),ptPixBtm.y(),///< bottom ROI new point.
               ptPixBtm.x()-(rectCenter.x+rectCenter.width/2),ptPixBtm.y()-(rectCenter.y+rectCenter.height/2),///< diff.
               ptEncCenter.x(),ptEncCenter.y(),///<center encoder value.
               ptEncBtm.x(),ptEncBtm.y(),///< bottom ROI encoder value.
               ptEncBtm.x()-ptEncCenter.x(),ptEncBtm.y()-ptEncCenter.y());///<diff.
        break;
    default:
        break;
    }
}
void ZProcessingThread::ZDrawCrossIndicator(cv::Mat &mat)
{
    return;
    //we draw a radius=100 circle.//RGB  //BGR
    cv::circle(mat,cv::Point(mat.cols/2,mat.rows/2),100,cv::Scalar(0,255,0),4,1);
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
void ZProcessingThread::ZDrawOnQImage(QImage &img)
{
    QPainter p;
    //p.setRenderHints(QPainter::Antialiasing,true);
    p.begin(&img);
    //draw a rectangle indicator.
    //    ___       ___
    //   |             |
    //
    //
    //   |             |
    //    ___       ___
    p.save();
    //move the (0,0) to the center of image.
    p.translate(img.width()/2,img.height()/2);
    p.setPen(QPen(Qt::green,4));
    p.drawLine(QPointF(70,-50),QPointF(100,-50));
    p.drawLine(QPointF(100,-50),QPointF(100,-20));
    ///
    p.drawLine(QPointF(70,50),QPointF(100,50));
    p.drawLine(QPointF(100,50),QPointF(100,20));
    ///
    p.drawLine(QPointF(-70,-50),QPointF(-100,-50));
    p.drawLine(QPointF(-100,-50),QPointF(-100,-20));
    ///
    p.drawLine(QPointF(-70,50),QPointF(-100,50));
    p.drawLine(QPointF(-100,50),QPointF(-100,20));
    //draw a half-tranparent mask and red color center point.
    p.setPen(Qt::NoPen);
    p.setBrush(QBrush(QColor(0,255,0,20)));
    //draw a ellipse with x radius=100,y radius=50.
    p.drawEllipse(QPoint(0,0),100,50);
    p.setBrush(QBrush(QColor(255,0,0,255)));
    p.drawEllipse(QPoint(0,0),6,6);
    p.restore();

    //draw the big scale.
    p.save();
    p.translate(img.width()/2,img.height());
    p.setPen(QPen(Qt::white,4));
    //we draw 9 lines in (180-18-18) degree.
    //so,(180-18-18) degreen /9=16 degree.
    for(int i=0;i<13;i++)
    {
        p.drawLine(QPointF(180,0),QPointF(200,0));
        p.rotate(-15);
    }
    p.restore();

    //draw the small scale.
    p.save();
    p.translate(img.width()/2,img.height());
    p.setPen(QPen(Qt::white,2));
    p.rotate(-7.5);
    p.drawLine(QPointF(190,0),QPointF(200,0));
    for(int i=0;i<11;i++)
    {
        p.rotate(-15);
        p.drawLine(QPointF(190,0),QPointF(200,0));
    }
    p.restore();

    //draw a arrow.
    p.save();
    p.translate(img.width()/2,img.height());
    p.setPen(QPen(Qt::white,2));
    p.setBrush(Qt::white);
    p.drawEllipse(QPoint(0,0),20,20);
    QPointF pt[]={{-6,0},{0,-150},{6,0}};
    p.rotate(30);
    p.drawConvexPolygon(pt,3);
    p.restore();
    p.end();
}
