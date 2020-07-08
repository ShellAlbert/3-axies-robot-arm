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
};
extern ZGblPara gGblPara;
#endif // ZGBLPARA_H
