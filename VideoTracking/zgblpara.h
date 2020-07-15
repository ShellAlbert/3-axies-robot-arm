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
    int m_pixelDiffX;
    int m_pixelDiffY;

    //tracking enabled.
    bool m_bTrackingEnabled;
};
extern ZGblPara gGblPara;

#include <QImage>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
extern QImage cvMat2QImage(const cv::Mat &mat);

#endif // ZGBLPARA_H
