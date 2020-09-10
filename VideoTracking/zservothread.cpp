#include "zservothread.h"
#include <zgblpara.h>
#include <QDebug>
extern "C"
{
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
#include <signal.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
}
/*EtherCAT slave address on the bus*/
#define CopleySlavePos    0, 0
#define CopleySlavePos2    0, 1

/*vendor_id, product_id*/
//vendor_id can be read from (0x1018,1)
//product_id can be read from (0x1018,2)
#define Copley_VID_PID  0x000000ab, 0x00001030

#define TASK_FREQUENCY       1000/*Hz*/

#define Bool int
#define false 0
#define true 1
#define ETHERCAT_STATUS_OP  0x08
#define STATUS_SERVO_ENABLE_BIT (0x04)


#define RANGE_LIMIT_MAX  28000
#define RANGE_LIMIT_MIN  -28000

//master status define.
enum
{
    SYS_WORKING_POWER_ON,
    SYS_WORKING_SAFE_MODE,
    SYS_WORKING_OPMODE,
    SYS_WORKING_UPDATE_POSITION,
};
int g_SysFSM=SYS_WORKING_POWER_ON;

int g_PPMPositionMode=PPM_POSITION_ABSOLUTE;

//global request to exit flag.
int g_bRst2ExitFlag=0;
void g_SignalHandler(int signo);

typedef struct
{
    int s0_login_zero;
    int s1_login_zero;
}ServoDev;
ServoDev gTechServo;
/*EtherCAT master*/
static ec_master_t *master=NULL;
static ec_master_state_t master_state={};
/*process data:input*/
static ec_domain_t *domainInput=NULL;
static ec_domain_state_t domainInput_state={};
static uint8_t *domainInput_pd=NULL;
/*process data:output*/
static ec_domain_t *domainOutput=NULL;
static ec_domain_state_t domainOutput_state={};
static uint8_t *domainOutput_pd=NULL;
/*Copley slave configuration*/
static ec_slave_config_t *sc_copley[2]={NULL,NULL};
static ec_slave_config_state_t sc_copley_state[2]={};

/*PDO entries offsets*/
static unsigned int ctrlWord[2];
static unsigned int targetPosition[2];
static unsigned int statusWord[2];
static unsigned int actVelocity[2];
static unsigned int actPosition[2];
const static ec_pdo_entry_reg_t domainOutput_regs[]=
{
    { CopleySlavePos, Copley_VID_PID, 0x6040, 0, &ctrlWord[0], NULL },
    { CopleySlavePos, Copley_VID_PID, 0x607A, 0, &targetPosition[0], NULL },

    { CopleySlavePos2, Copley_VID_PID, 0x6040, 0, &ctrlWord[1], NULL },
    { CopleySlavePos2, Copley_VID_PID, 0x607A, 0, &targetPosition[1], NULL },
    {}
};
const static ec_pdo_entry_reg_t domainInput_regs[]=
{
    { CopleySlavePos, Copley_VID_PID, 0x6041, 0, &statusWord[0], NULL },
    { CopleySlavePos, Copley_VID_PID, 0x606C, 0, &actVelocity[0], NULL },
    { CopleySlavePos, Copley_VID_PID, 0x6064, 0, &actPosition[0], NULL },

    { CopleySlavePos2, Copley_VID_PID, 0x6041, 0, &statusWord[1], NULL },
    { CopleySlavePos2, Copley_VID_PID, 0x606C, 0, &actVelocity[1], NULL },
    { CopleySlavePos2, Copley_VID_PID, 0x6064, 0, &actPosition[1], NULL },
    {}
};

//define TxPDO entries.(Master->Slave)
static ec_pdo_entry_info_t copley_pdo_entries_output[] = {
    { 0x6040, 0x00, 16 }, /*Control Word,uint16*/
    { 0x607A, 0x00, 32 }, /*Profile Target Position,uint32*/
};
//define TxPDO itself.
static ec_pdo_info_t copley_pdos_1600[] = {
    { 0x1600, 2, copley_pdo_entries_output },
};

//define RxPDO entries.(Master<-Slave)
static ec_pdo_entry_info_t copley_pdo_entries_input[] = {
    { 0x6041, 0x00, 16 }, /*Status Word,uint16*/
    { 0x606C, 0x00, 32 }, /*actual velocity,uint32*/
    { 0x6064, 0x00, 32 }, /*actual position,uint32*/
};
//define RxPDO itself.
static ec_pdo_info_t copley_pdos_1a00[] = {
    { 0x1a00, 3, copley_pdo_entries_input },
};
//Sync Manager configuration.
static ec_sync_info_t copley_syncs[] = {
    { 0, EC_DIR_OUTPUT, 0, NULL, EC_WD_DISABLE },//SM0:MBoxOut.
    { 1, EC_DIR_INPUT, 0, NULL, EC_WD_DISABLE },//SM1:MBoxIn.
    { 2, EC_DIR_OUTPUT, 1, copley_pdos_1600, EC_WD_DISABLE },//SM2:Outputs.
    { 3, EC_DIR_INPUT, 1, copley_pdos_1a00, EC_WD_DISABLE },//SM3:Inputs.
    { 0xFF }
};
void check_domain_state(void)
{
    ec_domain_state_t ds1={};
    ec_domain_state_t ds2={};

    //domainInput.
    ecrt_domain_state(domainInput,&ds1);
    if(ds1.working_counter!=domainInput_state.working_counter)
    {
        printf("domainInput: WC %u.\n",ds1.working_counter);
    }
    if(ds1.wc_state!=domainInput_state.wc_state)
    {
        printf("domainInput: State %u.\n",ds1.wc_state);
    }
    domainInput_state=ds1;

    //domainOutput.
    ecrt_domain_state(domainOutput,&ds2);
    if(ds2.working_counter!=domainOutput_state.working_counter)
    {
        printf("domainOutput: WC %u.\n",ds2.working_counter);
    }
    if(ds2.wc_state!=domainOutput_state.wc_state)
    {
        printf("domainOutput: State %u.\n",ds2.wc_state);
    }
    domainOutput_state=ds2;
}
void check_master_state(void)
{
    ec_master_state_t ms={};
    ecrt_master_state(master,&ms);
    if(ms.slaves_responding != master_state.slaves_responding)
    {
        printf("%u slave(s).\n",ms.slaves_responding);
    }
    if(ms.al_states != master_state.al_states)
    {
        printf("AL states:0x%02x.\n",ms.al_states);
    }
    if(ms.link_up != master_state.link_up)
    {
        printf("Link is %s.\n",ms.link_up?"up":"down");
    }
    master_state=ms;
}
void check_slave_config_state(void)
{
    //slave:0:0
    ec_slave_config_state_t s={};
    ecrt_slave_config_state(sc_copley[0],&s);
    if(s.al_state != sc_copley_state[0].al_state)
    {
        printf("sc_copley_state[0]: State 0x%02x.\n",s.al_state);
    }
    if(s.online != sc_copley_state[0].online)
    {
        printf("sc_copley_sate[0]: %s.\n",s.online ? "online" : "offline");
    }
    if(s.operational != sc_copley_state[0].operational)
    {
        printf("sc_copley_sate[0]: %soperational.\n",s.operational ? "" : "Not ");
    }
    sc_copley_state[0]=s;

    //slave:0:1
    ec_slave_config_state_t s1={};
    ecrt_slave_config_state(sc_copley[1],&s1);
    if(s1.al_state != sc_copley_state[1].al_state)
    {
        printf("sc_copley_state[1]: State 0x%02x.\n",s1.al_state);
    }
    if(s1.online != sc_copley_state[1].online)
    {
        printf("sc_copley_sate[1]: %s.\n",s1.online ? "online" : "offline");
    }
    if(s1.operational != sc_copley_state[1].operational)
    {
        printf("sc_copley_sate[1]: %soperational.\n",s1.operational ? "" : "Not ");
    }
    sc_copley_state[1]=s1;
}

ZServoThread::ZServoThread()
{

}
void ZServoThread::run()
{
    //Requests an EtherCAT master for realtime operation.
    master = ecrt_request_master(0);
    if (!master)
    {
        emit this->ZSigLog(true,"failed to request master 0!");
        return;
    }
    emit this->ZSigLog(false,"request master 0 okay.");

    //Creates a new process data domain.
    //For process data exchange, at least one process data domain is needed.
    //This method creates a new process data domain and returns a pointer to the new domain object.
    //This object can be used for registering PDOs and exchanging them in cyclic operation.
    domainInput = ecrt_master_create_domain( master );
    domainOutput = ecrt_master_create_domain( master );
    if(!domainInput || !domainOutput)
    {
        emit this->ZSigLog(true,"failed to create domain Input/Output!");
        return;
    }
    emit this->ZSigLog(false,"create domain okay.");

    //Obtains a slave configuration.
    //If the slave with the given address is found during the bus configuration,
    //its vendor ID and product code are matched against the given value.
    //On mismatch, the slave is not configured and an error message is raised.
    if(!(sc_copley[0]=ecrt_master_slave_config( master, CopleySlavePos, Copley_VID_PID)))
    {
        emit this->ZSigLog(true,"failed to get slave configuration for Copley-0.");
        return;
    }
    if(!(sc_copley[1]=ecrt_master_slave_config( master, CopleySlavePos2, Copley_VID_PID)))
    {
        emit this->ZSigLog(true,"failed to get slave configuration for Copley-1.");
        return;
    }
    emit this->ZSigLog(false,"get slave configuration okay!");

    //Specify a complete PDO configuration.
    //This function is a convenience wrapper for the functions
    //ecrt_slave_config_sync_manager(), ecrt_slave_config_pdo_assign_clear(),
    //ecrt_slave_config_pdo_assign_add(), ecrt_slave_config_pdo_mapping_clear()
    //and ecrt_slave_config_pdo_mapping_add(), that are better suitable for
    //automatic code generation.
    emit this->ZSigLog(false,"Configuring PDOs...");
    if(ecrt_slave_config_pdos(sc_copley[0], EC_END, copley_syncs))
    {
        emit this->ZSigLog(true,"failed to configure Copley PDOs." );
        return;
    }
    if(ecrt_slave_config_pdos(sc_copley[1], EC_END, copley_syncs))
    {
        emit this->ZSigLog(true,"failed to configure Copley PDOs." );
        return;
    }
    emit this->ZSigLog(false,"configure PDOs done.");

    if(ecrt_domain_reg_pdo_entry_list(domainInput, domainInput_regs ) )
    {
        emit this->ZSigLog(true,"PDO entry registration failed!" );
        return;
    }
    if(ecrt_domain_reg_pdo_entry_list(domainOutput, domainOutput_regs ) )
    {
        emit this->ZSigLog(true,"PDO entry registration failed!" );
        return;
    }
    emit this->ZSigLog(false,"PDO entry registration done.");

    //Finishes the configuration phase and prepares for cyclic operation.
    //This function tells the master that the configuration phase is finished and the realtime operation will begin.
    //The function allocates internal memory for the domains and calculates the logical FMMU addresses for domain members.
    //It tells the master state machine that the bus configuration is now to be applied.
    emit this->ZSigLog(false,"Activating master...");
    if ( ecrt_master_activate( master ) )
    {
        emit this->ZSigLog(true,"failed to activate master!");
        return;
    }
    emit this->ZSigLog(false,"master activated.");

    // Returns the domain's process data.
    if(!(domainInput_pd = ecrt_domain_data(domainInput)))
    {
        emit this->ZSigLog(true,"domain input data error!");
        return;
    }
    if(!(domainOutput_pd = ecrt_domain_data(domainOutput)))
    {
        emit this->ZSigLog(true,"domain output data error!");
        return;
    }

    //load config file.
    gTechServo.s0_login_zero=0;
    gTechServo.s1_login_zero=0;

    g_SysFSM=SYS_WORKING_SAFE_MODE;
    //signal(SIGINT,g_SignalHandler);
    int pixelDiffX,pixelDiffY;
    int PPMPositionMethod;
    while(!gGblPara.m_bExitFlag)
    {
        //1000us=1ms.
        //usleep(1000000/TASK_FREQUENCY);
        //if the time is less, the motor has no time to run.
        //so we set the time longer to wait for the motor executed the previous command.
        usleep(1000);
        //usleep(8000);
        //usleep(10000);
        //usleep(20000);

        static int cycle_counter=0;
        if(cycle_counter++>=1000*2)
        {
            cycle_counter=0;
        }

        /*receive EtherCAT frames*/
        ecrt_master_receive(master);
        ecrt_domain_process(domainOutput);
        ecrt_domain_process(domainInput);
        check_domain_state();

        check_master_state();
        check_slave_config_state();

        switch(g_SysFSM)
        {
        case SYS_WORKING_SAFE_MODE:
            //check if master is in OP mode,if not then turn to OP mode.
            if((master_state.al_states&ETHERCAT_STATUS_OP))
            {
                int tmp=1;
                if(sc_copley_state[0].al_state!=ETHERCAT_STATUS_OP)
                {
                    tmp=0;
                }
                if(sc_copley_state[1].al_state!=ETHERCAT_STATUS_OP)
                {
                    tmp=0;
                }
                if(tmp)
                {
                    g_SysFSM=SYS_WORKING_OPMODE;
                    emit this->ZSigLog(false,"FSM -> SYS_WORKING_OPMODE");
                }
            }
            break;
        case SYS_WORKING_OPMODE:
        {
            static int iTickCnt=0;
            switch(iTickCnt)
            {
            case 0://Reset.
                //bit7=1.
                //Reset Fault.A low-to-high transition of this bit makes the amplifier attempt to clear any latched fault condition.
                for(int i=0;i<2;i++)
                {
                    EC_WRITE_U16(domainOutput_pd+ctrlWord[i],0x80);
                }
                emit this->ZSigLog(false,"0:Reset.");
                iTickCnt++;
                break;

            case 1://set Operation Mode.
                //(0x6060,0)=1 PP:Profile Position mode.
                for(int i=0;i<2;i++)
                {
                    ecrt_slave_config_sdo8(sc_copley[i],0x6060,0,1);
                }
                emit this->ZSigLog(false,"1:Set Operation mode.");
                iTickCnt++;
                break;

            case 2://Set Parameter.
                for(int i=0;i<2;i++)
                {
                    //Sync Manager2,Synchronization Type=0:Free Run.
                    ecrt_slave_config_sdo16(sc_copley[i],0x1c32,0x01,0);

                    //Profile Velocity.
                    ecrt_slave_config_sdo32(sc_copley[i],0x6081,0x00,500000);
                    //Profile Acceleration.
                    ecrt_slave_config_sdo32(sc_copley[i],0x6083,0x00,5000);
                    //Profile Deceleration.
                    ecrt_slave_config_sdo32(sc_copley[i],0x6084,0x00,5000);
                    //Quick Stop Deceleration.
                    ecrt_slave_config_sdo32(sc_copley[i],0x6085,0x00,5000);
                    //Motion Profile Type=T.
                    ecrt_slave_config_sdo16(sc_copley[i],0x6086,0x00,0);
                }

                emit this->ZSigLog(false,"2:Set Parameters.");
                iTickCnt++;
                break;

            case 3://Enable Device.
                for(int i=0;i<2;i++)
                {
                    //0x06=0000,0110.
                    EC_WRITE_U16(domainOutput_pd+ctrlWord[i],0x06);
                    //0x0F=0000,1111.
                    EC_WRITE_U16(domainOutput_pd+ctrlWord[i],0x0F);
                }
                emit this->ZSigLog(false,"3:Enable Device.");
                iTickCnt++;
                break;

            case 4://Set Target Position to 0 at start up.
                EC_WRITE_S32(domainOutput_pd+targetPosition[0],gTechServo.s0_login_zero);
                EC_WRITE_S32(domainOutput_pd+targetPosition[1],gTechServo.s1_login_zero);
                emit this->ZSigLog(false,"4:Set Initial target position to 0.");
                //here we skip step 5.
                iTickCnt++;
                iTickCnt++;
                break;
            case 5://Set Target Position.
            {
                //100ms.
                if(gGblPara.usedSema->tryAcquire(1,1000))
                {
                    int s0_cur_pos,s1_cur_pos;
                    int bOverflow=0;

                    pixelDiffX=gGblPara.pixelDiffX;
                    pixelDiffY=gGblPara.pixelDiffY;
                    PPMPositionMethod=gGblPara.PPMPositionMethod;
                    gGblPara.freeSema->release();
                    QString str=QString("5:get new target position (%1,%2).").arg(pixelDiffX).arg(pixelDiffY);
                    emit this->ZSigLog(false,str);

                    /***********convert pixel diff to encoder value to archieve fast move***********/
                    //slave0:up-down direction.
                    //so here relate pixelDiffY with slave0.
                    int pos0=this->ZMapPixel2Servo(0,pixelDiffY);

                    //slave1:left-right direction.
                    //so here relate pixelDiffX with slave1.
                    int pos1=this->ZMapPixel2Servo(1,pixelDiffX);

                    /***********convert pixel diff to encoder value to archieve fast move***********/
                    switch(PPMPositionMethod)
                    {
                    case PPM_POSITION_ABSOLUTE:
                        //slave-0 minimum & maximum limit.
                        if(pos0<RANGE_LIMIT_MIN)
                        {
                            pos0=RANGE_LIMIT_MIN;
                            emit this->ZSigLog(true,QString("5:s0 absolute position minimum limit."));
                        }else if(pos0>RANGE_LIMIT_MAX)
                        {
                            pos0=RANGE_LIMIT_MAX;
                            emit this->ZSigLog(true,QString("5:s0 absolute position maximum limit."));
                        }
                        //slave-1 minimum & maximum limit.
                        if(pos1<RANGE_LIMIT_MIN)
                        {
                            pos1=RANGE_LIMIT_MIN;
                            emit this->ZSigLog(true,QString("5:s1 absolute position minimum limit."));
                        }else if(pos1>RANGE_LIMIT_MAX)
                        {
                            pos1=RANGE_LIMIT_MAX;
                            emit this->ZSigLog(true,QString("5:s1 absolute position maximum limit."));
                        }

                        EC_WRITE_S32(domainOutput_pd+targetPosition[0],pos0);
                        EC_WRITE_S32(domainOutput_pd+targetPosition[1],pos1);
                        emit this->ZSigLog(false,QString("5:set target absolute position(%1,%2).").arg(pos0).arg(pos1));
                        break;
                    case PPM_POSITION_RELATIVE:
                        s0_cur_pos=EC_READ_S32(domainInput_pd + actPosition[0]);
                        s1_cur_pos=EC_READ_S32(domainInput_pd + actPosition[1]);
                        //slave-0 minimum & maximum limit.
                        if((pos0+s0_cur_pos)<RANGE_LIMIT_MIN)
                        {
                            pos0=RANGE_LIMIT_MIN-s0_cur_pos;
                            emit this->ZSigLog(true,QString("5:s0 absolute position minimum limit."));
                        }else if((pos0+s0_cur_pos)>RANGE_LIMIT_MAX)
                        {
                            pos0=RANGE_LIMIT_MAX-s0_cur_pos;
                            emit this->ZSigLog(true,QString("5:s0 absolute position maximum limit."));
                        }
                        //slave-1 minimum & maximum limit.
                        if((pos1+s1_cur_pos)<RANGE_LIMIT_MIN)
                        {
                            pos1=RANGE_LIMIT_MIN-s1_cur_pos;
                            emit this->ZSigLog(true,QString("5:s1 absolute position minimum limit."));
                        }else if((pos1+s1_cur_pos)>RANGE_LIMIT_MAX)
                        {
                            pos1=RANGE_LIMIT_MAX-s1_cur_pos;
                            emit this->ZSigLog(true,QString("5:s1 absolute position maximum limit."));
                        }

                        EC_WRITE_S32(domainOutput_pd+targetPosition[0],pos0);
                        EC_WRITE_S32(domainOutput_pd+targetPosition[1],pos1);
                        emit this->ZSigLog(false,QString("5:set target relative position (%1,%2).").arg(pos0).arg(pos1));
                        break;
                    }
                    iTickCnt++;
                }
            }
                break;
            case 6://Start Positioning.
                switch(PPMPositionMethod)
                {
                case PPM_POSITION_ABSOLUTE:
                    for(int i=0;i<2;i++)
                    {
                        //Control word (absolute position,start immediately).
                        EC_WRITE_U16(domainOutput_pd+ctrlWord[i],0x3F);
                    }
                    break;
                case PPM_POSITION_RELATIVE:
                    for(int i=0;i<2;i++)
                    {
                        //Control word (relative position,start immediately).
                        EC_WRITE_U16(domainOutput_pd+ctrlWord[i],0x7F);
                    }
                    break;
                }
                emit this->ZSigLog(false,"6:Start positioning.");
                iTickCnt++;
                break;
            case 7://set point acknowledge.
            {
                static int iTimeout=1000;
                uint16_t status1,status2;
                status1 = EC_READ_U16(domainInput_pd + statusWord[0]);
                status2 = EC_READ_U16(domainInput_pd + statusWord[1]);
                if((status1&(0x1<<12)) && (status2&(0x1<<12)))
                {
                    emit this->ZSigLog(false,"7:set point acknowledge.");
                    iTickCnt++;
                }else{
                    emit this->ZSigLog(false,"7:set point not acknowledge.");
                    iTimeout--;
                    if(iTimeout==0)
                    {
                        iTimeout=1000;
                        emit this->ZSigLog(false,"7:set point reset.");
                        iTickCnt=10;
                    }
                }
            }
                break;
            case 8://RESET.
                for(int i=0;i<2;i++)
                {
                    //0x0F=0000,1111.
                    EC_WRITE_U16(domainOutput_pd+ctrlWord[i],0x0F);
                }
                emit this->ZSigLog(false,"8:Reset");
                iTickCnt++;
                break;
            case 9://Target reached ?
            {
                uint16_t status0,status1;
                int velocity0,velocity1;
                int curPos0,curPos1;

                //thus we move in relative mode,
                //so the target position is not normal value like 0,5000,10000,etc.
                //it maybe -2,75,4997,10001,etc.
                status0=EC_READ_U16(domainInput_pd + statusWord[0]);
                velocity0=EC_READ_S32(domainInput_pd + actVelocity[0]);
                curPos0=EC_READ_S32(domainInput_pd + actPosition[0]);

                status1=EC_READ_U16(domainInput_pd + statusWord[1]);
                velocity1=EC_READ_S32(domainInput_pd + actVelocity[1]);
                curPos1=EC_READ_S32(domainInput_pd + actPosition[1]);

                if((status0&(0x1<<10)) && (status1&(0x1<<10)))
                {
                    emit this->ZSigLog(false,QString("9:Target reached,position=(%1,%2)").arg(curPos0).arg(curPos1));
                    iTickCnt++;
                }else{
                    //printf("Target not reached,position=%d,%d\n",curPos0,curPos1);
                }
            }
                break;
            case 10:
                emit this->ZSigLog(false,"10:Finish.");
                iTickCnt=5;
                break;
            default:
                break;
            }
        }
            break;
        default:
            break;
        }
        //update PDOs.
        this->ZUpdatePDO();

        /*send process data*/
        ecrt_domain_queue(domainOutput);
        ecrt_domain_queue(domainInput);
        ecrt_master_send(master);
    }

    ecrt_release_master(master);
    master=NULL;

    qDebug("Copley slave test end.\n");
    return;
}
int ZServoThread::ZMapPixel2Servo(int servoID,int diff)
{
    switch(gGblPara.m_appMode)
    {
    case Free_Mode:
    {
        //for manual move operation.
        return diff;
    }
        break;
    case SelectROI_Mode:
        break;
    case Track_Mode:
    {
#if 1
        //PID:servo_relative_move_step=k*x+b.
        int servo_relative_move_step=0;
        float k=1.0;
        float b=0.0;
        switch(servoID)
        {
        case 0://slave-0: y axies.
            if(diff>300)
            {
                servo_relative_move_step=+400;
            }else if(diff>200 && diff<=300)
            {
                servo_relative_move_step=+300;
            }else if(diff>100 && diff<=200)
            {
                servo_relative_move_step=+200;
            }else if(diff>50 && diff<=100)
            {
                servo_relative_move_step=+50;
            }else if(diff>20 && diff<=50)
            {
                servo_relative_move_step=+10;
            }else if(diff>10 && diff<=20)
            {
                servo_relative_move_step=+5;
            }else if(diff>0){
                servo_relative_move_step=+1;
            }else if(diff<-300)
            {
                servo_relative_move_step=-400;
            }else if(diff<-200 && diff>=-300)
            {
                servo_relative_move_step=-300;
            }else if(diff<-100 && diff>=-200)
            {
                servo_relative_move_step=-200;
            }else if(diff<-50 && diff>=-100)
            {
                servo_relative_move_step=-50;
            }else if(diff<-20 && diff>=-50)
            {
                servo_relative_move_step=-20;
            }else if(diff<-10 && diff>=-20)
            {
                servo_relative_move_step=-10;
            }else if(diff<0 && diff>=-10)
            {
                servo_relative_move_step=-1;
            }
            break;
        case 1://slave-1:x axies.
            if(diff>300)
            {
                servo_relative_move_step=-400;
            }else if(diff>200 && diff<=300)
            {
                servo_relative_move_step=-300;
            }else if(diff>100 && diff<=200)
            {
                servo_relative_move_step=-200;
            }else if(diff>50 && diff<=100)
            {
                servo_relative_move_step=-50;
            }else if(diff>20 && diff<=50)
            {
                servo_relative_move_step=-10;
            }else if(diff>10 && diff<=20)
            {
                servo_relative_move_step=-5;
            }else if(diff>0){
                servo_relative_move_step=-1;
            }else if(diff<-300)
            {
                servo_relative_move_step=+400;
            }else if(diff<-200 && diff>=-300)
            {
                servo_relative_move_step=+300;
            }else if(diff<-100 && diff>=-200)
            {
                servo_relative_move_step=+200;
            }else if(diff<-50 && diff>=-100)
            {
                servo_relative_move_step=+50;
            }else if(diff<-20 && diff>=-50)
            {
                servo_relative_move_step=+50;
            }else if(diff<-10 && diff>=-20)
            {
                servo_relative_move_step=+10;
            }else if(diff<0 && diff>=-10)
            {
                servo_relative_move_step=+1;
            }
            break;
        }
        return servo_relative_move_step;
#endif
    }
        break;
    }
    return 0;
}
void ZServoThread::ZUpdatePDO(void)
{
    //flush servo status data to global variable.
    int statusWord0 = EC_READ_U16(domainInput_pd + statusWord[0]);
    int velocity0 = EC_READ_S32(domainInput_pd + actVelocity[0])/1000.0;
    int position0 = EC_READ_S32(domainInput_pd + actPosition[0]);
    gGblPara.m_servoCurPos[0]=position0;
    emit this->ZSigPDO(0,statusWord0,velocity0,position0);

    int statusWord1 = EC_READ_U16(domainInput_pd + statusWord[1]);
    int velocity1 = EC_READ_S32(domainInput_pd + actVelocity[1])/1000.0;
    int position1 = EC_READ_S32(domainInput_pd + actPosition[1]);
    gGblPara.m_servoCurPos[1]=position1;
    emit this->ZSigPDO(1,statusWord1,velocity1,position1);
}
