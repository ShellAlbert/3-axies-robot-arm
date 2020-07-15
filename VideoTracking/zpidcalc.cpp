#include "zpidcalc.h"

ZPIDCalc::ZPIDCalc()
{
    //we move by a small step to avoid amplifier driver error.
    this->m_iMaxLimit=1000;

    //PID init.
    this->m_fKp=1.0;
    this->m_fKi=0.0;
    this->m_fKd=0.0;

    this->m_fPOut=0;
    this->m_fIOut=0;
    this->m_fDOut=0;

    this->m_iErr=0;
    this->m_iErrLast=0;
}
int ZPIDCalc::doPidCalc(int iCurPos,int iTarPos)
{
    int iOut=0;

    //the different error.
    this->m_iErr=iTarPos-iCurPos;
    //P.
    this->m_fPOut=this->m_fKp*this->m_iErr;
    //I.
    this->m_fIOut+=this->m_fKi*this->m_iErr;
    //D.
    this->m_fDOut=this->m_fKd*(this->m_iErr-this->m_iErrLast);

    //the real out.
    iOut=this->m_fPOut+this->m_fIOut+this->m_fDOut;
     //we move by a small step to avoid amplifier driver error.
    if(iOut>this->m_iMaxLimit)
    {
        iOut=this->m_iMaxLimit;
    }
    if(iOut<-(this->m_iMaxLimit))
    {
        iOut=-(this->m_iMaxLimit);
    }

    this->m_iErrLast=this->m_iErr;
    return iOut;
}
