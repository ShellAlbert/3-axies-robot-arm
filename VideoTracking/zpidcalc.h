#ifndef ZPIDCALC_H
#define ZPIDCALC_H

#include <QObject>
class ZPIDCalc
{
public:
    ZPIDCalc();
    int doPidCalc(int iCurPos,int iTarPos);
private:
    int m_iMaxLimit;
    float m_fKp;
    float m_fKi;
    float m_fKd;

    float m_fPOut;
    float m_fIOut;
    float m_fDOut;
private:
    int m_iErr;
    int m_iErrLast;
};

#endif // ZPIDCALC_H
