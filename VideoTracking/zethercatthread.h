#ifndef ZETHERCATTHREAD_H
#define ZETHERCATTHREAD_H

#include "ecrt.h"
#include <stdio.h>
#include <unistd.h>
#include <sched.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <sys/resource.h>
#include <time.h>
#include <sys/types.h>

/*EtherCAT slave address on the bus*/
#define CopleySlavePos    0, 0
//#define CopleySlavePos    0, 1

/*vendor_id, product_id*/
//vendor_id can be read from (0x1018,1)
//product_id can be read from (0x1018,2)
#define Copley_VID_PID  0x000000ab, 0x00001030

#include <QThread>
class ZEtherCATThread : public QThread
{
    Q_OBJECT
public:
    ZEtherCATThread();
protected:
    void run();

private:

};

#endif // ZETHERCATTHREAD_H
