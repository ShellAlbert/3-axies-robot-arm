#ifndef ZGBLPARA_H
#define ZGBLPARA_H

#include <QObject>
#include <QVector>
#include <QPoint>
#include <QMutex>
enum{
    FSM_Calibrate_Start,
    FSM_Calibrate_Left,
    FSM_Calibrate_LeftConfirm,
    FSM_Calibrate_Right,
    FSM_Calibrate_RightConfirm,
    FSM_Calibrate_Top,
    FSM_Calibrate_TopConfirm,
    FSM_Calibrate_Bottom,
    FSM_Calibrate_BottomConfirm,
    FSM_Calibrate_Done,
};
class ZGblPara
{
public:
    ZGblPara();

public:
    bool m_bExitFlag;

public:
    //the servo motor encoder current position.
    int m_iXAxisCurPos;
    int m_iYAxisCurPos;
public:
    //pixel diff.
    int m_moveDiffX;
    int m_moveDiffY;

    //tracking enabled.
    bool m_bTrackingEnabled;
    int m_trackDiffX;
    int m_trackDiffY;
    bool m_bTargetLocked;
    int m_iCostMSec;
    //the rectangle we're tracking in.
    int m_iTrackInRectW;
    int m_iTrackInRectH;


    //motor move step mode.
    //0:large step,1:middle step,2:small step.
    int m_iStepMode;

    //mapping pixel coordinate to motor move coordinate.
    int m_CalibrateFSM;

//    //pixel coordinate & Encoder coordinate value located on center point.
//    QPoint m_ptPixCenter;
//    QPoint m_ptEncCenter;

    //pixel coordinate & Encoder coordinate value located on left calibrate point.
    QPoint m_ptPixLft;
    QPoint m_ptEncLft;

    //pixel coordinate & Encoder coordinate value located on right calibrate point.
    QPoint m_ptPixRht;
    QPoint m_ptEncRht;

    //pixel coordinate & Encoder coordinate value located on top calibrate point.
    QPoint m_ptPixTop;
    QPoint m_ptEncTop;

    //pixel coordinate & Encoder coordinate value located on bottom calibrate point.
    QPoint m_ptPixBtm;
    QPoint m_ptEncBtm;


};
extern ZGblPara gGblPara;

#include <QImage>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
extern QImage cvMat2QImage(const cv::Mat &mat);

#endif // ZGBLPARA_H
