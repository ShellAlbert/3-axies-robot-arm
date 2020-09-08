#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#define FIFO_NAME "/tmp/servo.0"
int g_bExitFlag=0;
int main(int argc,char **argv)
{
    int fd;
    int a=-20000,b=20000;
    fd=open(FIFO_NAME,O_WRONLY);
    if(fd<0)
    {
        fprintf(stderr,"failed to open fifo %s\n",FIFO_NAME);
        return -1;
    }
    srand(time(0));
    while(!g_bExitFlag)
    {
        char buffer[256];
        int pos=rand()%(b-a+1)+a;
        int pos2=rand()%(b-a+1)+a;
        int len;
        //Profile Position Mode - Absolute Position.
        sprintf(buffer,"abs_pos=%d,%d\n",pos,pos2);
        //Profile Position Mode - Relative Position.
        //sprintf(buffer,"rel_pos=%d,%d\n",pos,pos2);
        len=strlen(buffer);
        write(fd,(void*)&len,sizeof(len));
        write(fd,(void*)buffer,len);
        printf("%d:%s\n",len,buffer);
        sleep(1);
    }
    close(fd);
    return 0;
}