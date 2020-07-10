#ifndef ZGBLPARA_H
#define ZGBLPARA_H

#include <QObject>
class ZGblPara
{
public:
    ZGblPara();

public:
    bool m_bExitFlag;

public:
    //slaves enabled bit mask.
    //enabled if bit was set,disabled if bit was clear.
    int m_iSlavesEnBitMask;

    //slave0 Target Position.
    int m_iSlave0TarPos;

    //slave1 Target Position.
    int m_iSlave1TarPos;
};
extern ZGblPara gGblPara;

#include <QImage>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
extern QImage cvMat2QImage(const cv::Mat &mat);

#endif // ZGBLPARA_H
