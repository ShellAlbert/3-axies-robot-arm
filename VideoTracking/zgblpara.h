#ifndef ZGBLPARA_H
#define ZGBLPARA_H

#include <QObject>
#include <QVector>
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
    int m_iS0CurPos;
    int m_iS1CurPos;
public:
    //pixel diff.
    int m_moveDiffX;
    int m_moveDiffY;

    //tracking enabled.
    bool m_bTrackingEnabled;
    int m_trackDiffX;
    int m_trackDiffY;
    bool m_bObjectLocked;


    //motor move step mode.
    //0:large step,1:middle step,2:small step.
    int m_iStepMode;

    //mapping pixel coordinate to motor move coordinate.
    int m_CalibrateFSM;
};
extern ZGblPara gGblPara;

#include <QImage>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
extern QImage cvMat2QImage(const cv::Mat &mat);

#endif // ZGBLPARA_H
