#include "zrdservooutthread.h"
#include "zgblpara.h"
#include <QDebug>
#include <stdio.h>
#include <unistd.h>
#include <sched.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <sys/resource.h>
#include <time.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>

ZRdServoOutThread::ZRdServoOutThread()
{

}
void ZRdServoOutThread::run()
{
    while(!gGblPara.m_bExitFlag)
    {
        int len;
        char buffer[256];
        //first read 4 bytes block length.
        if(read(gGblPara.m_fdServoFIFOOut,(void*)&len,sizeof(len))==sizeof(len))
        {
            //qDebug("read len=%d\n",len);
            //second read N bytes block data.
            if(read(gGblPara.m_fdServoFIFOOut,(void*)buffer,len)==len)
            {
                //qDebug("read data=%s\n",buffer);
                //format input.
                int servo_id;
                int status_word;
                int velocity;
                int position;
                sscanf(buffer,"%d/%d/%d/%d\n",&servo_id,&status_word,&velocity,&position);
                switch(servo_id)
                {
                case 0:
                    gGblPara.servo0_statusWord=status_word;
                    gGblPara.servo0_velocity=velocity;
                    gGblPara.servo0_position=position;
                    break;
                case 1:
                    gGblPara.servo1_statusWord=status_word;
                    gGblPara.servo1_velocity=velocity;
                    gGblPara.servo1_position=position;
                    break;
                }
            }
        }
    }
}
