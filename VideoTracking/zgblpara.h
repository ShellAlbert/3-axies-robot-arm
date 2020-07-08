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

    //0,0(slave 0), Actural Position, Target Position.
    int m_i00ActPos;
    int m_i00TarPos;

    //0,1(slave 1), Actural Position, Target Position.
    int m_i01ActPos;
    int m_i01TarPos;
};
extern ZGblPara gGblPara;

#include <QImage>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
extern QImage cvMat2QImage(const cv::Mat &mat);
#endif // ZGBLPARA_H
