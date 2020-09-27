//x axis gyroscope stabilizier
//www.wit-motion.com
//WT901B
//date:2020/9/25 +8613522296239.
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <sys/epoll.h>
#include <inttypes.h>
#include <pthread.h>
#include <signal.h>

#define MAXEVENTS 64
struct Angle0x53{
    float x_roll;//fan gun jiao, toward right.
    float y_pitch;//fu yang jiao,toward front.
    float z_yaw;//pian hang jiao,toward top.
    float temperature;
};
typedef struct{
    //request to exit flag.
    int bRst2ExitFlag;

    //file descriptor for creating a pipe.
    //fd[0]:for read.
    //fd[1]:for write.
    int fd[2];

    //producer thread id.
    pthread_t pid;
    //consumer thread id.
    pthread_t cid;

    //gyro data.
    struct Angle0x53 data_angle;
}GyroDevice;
GyroDevice  gyroDev;

void g_SignalHandler(int signo)
{
    if(signo==SIGINT)
    {
        printf("SIGINT:request to exit.\n");
        gyroDev.bRst2ExitFlag=1;
    }
}
void parse_gyro_protocol(const char *buffer,int len)
{
#if 0
    printf("parse:\n");
    for(int i=0;i<len;i++)
    {
        printf("%x,",buffer[i]);
    }
    printf("\n");
#endif
    if(len!=11)
    {
        printf("parse:invalid protocol length!\n");
        return;
    }
    if(buffer[0]!=0x55)
    {
        printf("parse:invalid sync header!\n");
        return;
    }
    switch(buffer[1])
    {
    case 0x50:
        break;
    case 0x51:
        break;
    case 0x52:
        break;
    case 0x53:
    {
        short x_angle=((unsigned short)buffer[3]<<8)|buffer[2];
        short y_angle=((unsigned short)buffer[5]<<8)|buffer[4];
        short z_angle=((unsigned short)buffer[7]<<8)|buffer[6];
        short temperature=((unsigned short)buffer[9]<<8)|buffer[8];
        gyroDev.data_angle.x_roll=x_angle/32768.0*180.0;
        gyroDev.data_angle.y_pitch=y_angle/32768.0*180.0;
        gyroDev.data_angle.z_yaw=z_angle/32768.0*180.0;
        gyroDev.data_angle.temperature=temperature/100.0;

        printf("Roll:%.2f° Pitch:%.2f° Yaw:%.2f°  T:%.2f℃\n",///<
               gyroDev.data_angle.x_roll,///<
                gyroDev.data_angle.y_pitch,///<
                gyroDev.data_angle.z_yaw,///<
                gyroDev.data_angle.temperature );
    }
        break;
    case 0x54:
        break;
    case 0x55:
        break;
    case 0x56:
        break;
    case 0x57:
        break;
    case 0x58:
        break;
    default:
        break;
    }
}
void *producer_thread(void *p)
{
    GyroDevice *dev=(GyroDevice*)(p);
    int fd;
    int efd;
    struct epoll_event event;
    struct epoll_event *events;
    char buffer[1024];
    if( (fd=open("/dev/ttyUSB0",O_RDWR))<0 )         //| O_NOCTTY | O_NDELAY
    {
        perror("Can't Open Serial Port");
        dev->bRst2ExitFlag=1;
        return 0;
    }
    //9600,8N1.
    struct termios options;
    tcgetattr(fd, &options);

    cfsetispeed(&options,B9600);
    cfsetospeed(&options,B9600);

    options.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/
    options.c_oflag  &= ~OPOST;   /*Output*/

    //disable the parity enable bit,so no parity.
    options.c_cflag &= ~PARENB;
    //CSTOPB=2 Stop bits,here it is cleared so 1 stop bit.
    options.c_cflag &= ~CSTOPB;
    //clear the mask for setting the data size.
    options.c_cflag &= ~CSIZE;
    //set the data bits=8.
    options.c_cflag |= CS8;

    //no hardware flow control.
    options.c_cflag &= ~CRTSCTS;
    //enable reciever,ignore modem control lines.
    options.c_cflag |= CREAD|CLOCAL;
    //disable XON/XOFF flow control both i/p and o/p.
    options.c_iflag &= ~(IXON|IXOFF|IXANY|INLCR|ICRNL);

    tcsetattr(fd,TCSANOW,&options);


    efd = epoll_create1(0);//initial is 0
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;

    epoll_ctl (efd, EPOLL_CTL_ADD, fd, &event);
    /* Buffer where events are returned */
    events=calloc(MAXEVENTS, sizeof event);

    /* The event loop */
    while(!dev->bRst2ExitFlag)
    {
        int n=epoll_wait(efd,events,MAXEVENTS,5000);
        if(n>0)
        {
            int length=read(events[0].data.fd, buffer, sizeof(buffer));
            if(length>0)
            {
                buffer[length]=0;

                //dump in hex for checking.
                if(0)
                {
                    printf("read from gyroscope:%d\n",length);
                    for(int i=0;i<length;i++)
                    {
                        printf("%x,",buffer[i]);
                    }
                    printf("\n");
                }

                //loop to write to pipe until success or error occurs.
                int i_wr_total=length;
                int i_wr_bytes=0;
                while(i_wr_total>0)
                {
                    int ret=write(dev->fd[1],&buffer[i_wr_bytes],i_wr_total);
                    if(ret<0)
                    {
                        printf("error at write() to pipe.\n");
                        break;
                    }
                    i_wr_total-=ret;
                    i_wr_bytes+=ret;
                }
            }
        }else{
            printf("No data whthin 5 seconds.\n");
        }
        usleep(50);
        //sleep(2);
    }
    free(events);
    close (fd);
}
enum{
    FSM_RESET,
    FSM_READ_PIPE,
    FSM_SCAN_PROCESS,
};
void *consumer_thread(void *p)
{
    GyroDevice *dev=(GyroDevice*)(p);
    char buffer[512];
    int i_total_bytes;
    int fsm=FSM_RESET;
    while(!dev->bRst2ExitFlag)
    {
        switch(fsm)
        {
        case FSM_RESET:
            i_total_bytes=0;
            fsm=FSM_READ_PIPE;
            break;
        case FSM_READ_PIPE:
        {
            int ret=read(dev->fd[0],buffer+i_total_bytes,sizeof(buffer)-i_total_bytes);
            if(ret<0)
            {
                printf("read error:%d,reset!\n",ret);
                fsm=FSM_RESET;
            }else{
                i_total_bytes+=ret;
                if(i_total_bytes>11)
                {
                    fsm=FSM_SCAN_PROCESS;
                }
            }
        }
            break;
        case FSM_SCAN_PROCESS:
        {
            //case 1:  55,50,x,x,x,x,x,x,x,x,x,x,x,x
            //case 2:  x,x,x,x,55,55,50,x,x,x,x,x,x
            //case 3: x,x,x,x,x,x,x,x,x,x,x,x,55,50
            int rd_pos=0;
            while( (i_total_bytes-rd_pos)>=11 )
            {
                if( buffer[rd_pos]==0x55 && (buffer[rd_pos+1]>=0x50 && buffer[rd_pos+1]<=0x58) )
                {
                    //to avoid 0x55,0x55,0x51.... such sample data.
                    if( buffer[rd_pos+1]==0x55 && (buffer[rd_pos+2]>=0x50 && buffer[rd_pos+2]<=0x58) )
                    {
                        parse_gyro_protocol(&buffer[rd_pos+1],11);
                        rd_pos+=12;
                    }else{
                        parse_gyro_protocol(&buffer[rd_pos],11);
                        rd_pos+=11;
                    }
                }else{
                    rd_pos++;
                }
            }
            if( (i_total_bytes-rd_pos)>0 )
            {
                memmove(&buffer[0],&buffer[rd_pos],i_total_bytes-rd_pos);
                i_total_bytes=i_total_bytes-rd_pos;
            }else{
                i_total_bytes=0;
            }
            fsm=FSM_READ_PIPE;
        }
            break;
        }
    }
}

int main(void)
{
    int ret;

    //create pipe.
    if(pipe(gyroDev.fd)<0)
    {
        printf("error at pipe().\n");
        return -1;
    }

    //create theads.
    ret=pthread_create(&gyroDev.pid,NULL,producer_thread,(void*)&gyroDev);
    if(ret!=0)
    {
        printf("failed to create producer thread!\n");
        return -1;
    }
    ret=pthread_create(&gyroDev.cid,NULL,consumer_thread,(void*)&gyroDev);
    if(ret!=0)
    {
        printf("failed to create consumer thread!\n");
        return -1;
    }
    //SIGINT.
    signal(SIGINT,g_SignalHandler);

    pthread_join(gyroDev.pid,NULL);
    pthread_join(gyroDev.cid,NULL);
    printf("main done.\n");

    return 0;
}

