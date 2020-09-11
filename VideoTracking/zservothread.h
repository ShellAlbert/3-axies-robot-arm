#ifndef ZSERVOTHREAD_H
#define ZSERVOTHREAD_H

#include <QThread>
#include "zdifffifo.h"
typedef struct{
    int pixel_min;
    int pixel_max;
    int move_step;
}ZPix2MovTable;
class ZServoThread : public QThread
{
    Q_OBJECT
public:
    ZServoThread(ZDiffFIFO *fifoDIFF);

signals:
    void ZSigLog(bool bError,const QString &log);
    void ZSigPDO(int servoID,int statusWord,int velocity,int position);
    void ZSigTargetReached();
    void ZSigDiffAvailable(int nums);
protected:
    void run();
private:
    int ZMapPixel2Servo(int servoID,int diff);
    void ZUpdatePDO(void);
private:
    ZDiffFIFO *m_fifoDIFF;
};

#endif // ZSERVOTHREAD_H
