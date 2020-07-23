#include "zgblpara.h"

ZGblPara::ZGblPara()
{
    this->m_bExitFlag=false;

    //application running mode.
    this->m_appMode=Free_Mode;

    //tracking enabled.
    this->m_bTrackingEnabled=false;
    this->m_trackDiffX=0;
    this->m_trackDiffY=0;
    this->m_bTargetLocked=false;
    this->m_iCostMSec=0;

    //pixel diff.
    this->m_moveDiffX=0;
    this->m_moveDiffY=0;

    //motor move step mode.
    //0:large step,1:middle step,2:small step.
    this->m_iStepMode=0;

    //mapping pixel coordinate to motor move coordinate.
    this->m_CalibrateFSM=FSM_Calibrate_Start;
}
ZGblPara gGblPara;

#include <QTime>
#include <QDebug>
QImage cvMat2QImage(const cv::Mat &mat)
{
    if(mat.type()==CV_8UC1)
    {
        QVector<QRgb> colorTable;
        for(int i=0;i<256;i++)
        {
            colorTable.push_back((qRgb(i,i,i)));
        }
        const uchar *qImageBuffer=(const uchar*)mat.data;
        QImage img(qImageBuffer,mat.cols,mat.rows,mat.step,QImage::Format_Indexed8);
        img.setColorTable(colorTable);
        return img;
    }
    if(mat.type()==CV_8UC3)
    {
        //copy input Mat.
        const uchar *qImageBuffer=(const uchar*)mat.data;
        //create QImage with same dimensions as input Mat.
        QImage img(qImageBuffer,mat.cols,mat.rows,mat.step,QImage::Format_RGB888);
        return img.rgbSwapped();
    }else{
        qDebug()<<"ERROR:Mat could not be converted to QImage.";
        return QImage();
    }
}
