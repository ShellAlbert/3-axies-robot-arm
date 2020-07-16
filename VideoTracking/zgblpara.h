#ifndef ZGBLPARA_H
#define ZGBLPARA_H

#include <QObject>
#include <QVector>
#include <QMutex>
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
};
extern ZGblPara gGblPara;

#include <QImage>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
extern QImage cvMat2QImage(const cv::Mat &mat);

#endif // ZGBLPARA_H
