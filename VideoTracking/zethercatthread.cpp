#include "zethercatthread.h"
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
}

//used to emit UI.
int gStatusWord=0;
int gActVelocity=0;
int gActPosition=0;
int gTarPosition=0;

int gStatusWord2=0;
int gActVelocity2=0;
int gActPosition2=0;
int gTarPosition2=0;

/*EtherCAT slave address on the bus*/
#define CopleySlavePos    0, 0
#define CopleySlavePos2    0, 1

/*vendor_id, product_id*/
//vendor_id can be read from (0x1018,1)
//product_id can be read from (0x1018,2)
#define Copley_VID_PID  0x000000ab, 0x00001030

enum{
    FSM_Power_On,
    FSM_SafeOp,
    FSM_Homing,
    FSM_CheckHoming,
    FSM_RunPV,
    FSM_ConfigSlave,
    FSM_CheckStatus,
    FSM_DoMove,
    FSM_IdleStatus,
};
int g_SysFSM=FSM_Power_On;
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

/**< Pointer to a variable to store the PDO entry's (byte-)offset in the process data. */
//offset for PDO entries just like a alias or symbol.
//we only use its offset,we use its address.
static uint32_t offsetCtrlWord[2];
static uint32_t offsetTarPos[2];
static uint32_t offsetStatusWord[2];
static uint32_t offsetActVel[2];
static uint32_t offsetPosActVal[2];

const static ec_pdo_entry_reg_t domainOutput_regs[]=
{
    { CopleySlavePos, Copley_VID_PID, 0x6040, 0, &offsetCtrlWord[0], NULL },
    { CopleySlavePos, Copley_VID_PID, 0x607A, 0, &offsetTarPos[0], NULL },

    { CopleySlavePos2, Copley_VID_PID, 0x6040, 0, &offsetCtrlWord[1], NULL },
    { CopleySlavePos2, Copley_VID_PID, 0x607A, 0, &offsetTarPos[1], NULL },
    {}
};
const static ec_pdo_entry_reg_t domainInput_regs[]=
{
    { CopleySlavePos, Copley_VID_PID, 0x6041, 0, &offsetStatusWord[0], NULL },
    { CopleySlavePos, Copley_VID_PID, 0x606C, 0, &offsetActVel[0], NULL },
    { CopleySlavePos, Copley_VID_PID, 0x6063, 0, &offsetPosActVal[0], NULL },

    { CopleySlavePos2, Copley_VID_PID, 0x6041, 0, &offsetStatusWord[1], NULL },
    { CopleySlavePos2, Copley_VID_PID, 0x606C, 0, &offsetActVel[1], NULL },
    { CopleySlavePos2, Copley_VID_PID, 0x6063, 0, &offsetPosActVal[1], NULL },
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
    { 0x6063, 0x00, 32 }, /*actual position,uint32*/
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
    //Reads the state of a domain.
    //Using this method, the process data exchange can be monitored in realtime.
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
    //Reads the state of a domain.
    //Using this method, the process data exchange can be monitored in realtime.
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
void print_bits_splitter(int bits)
{
    //bits splitter.
    char bitsBuffer[40];
    int index=0;
    int p1=bits;
    for(int i=0;i<32;i++)
    {
        bitsBuffer[index++]=(p1&0x80000000)?'1':'0';
        if(0==((i+1)%4))
        {
            bitsBuffer[index++]=',';
        }
        p1<<=1;
    }
    printf("< %s >",bitsBuffer);
}
void cyclic_task()
{
    static int iReverFlag=0;
    static int i00TarPos=0,i01TarPos=0;
    static int cycle_counter=0;
    cycle_counter++;
    if(cycle_counter>=1000*2)
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
    case FSM_SafeOp:
        //check if master is in OP mode,if not then turn to OP mode.
        //Master state.
        //unsigned int al_states : 4;
        //Application-layer states of all slaves.The states are coded in the lower 4 bits.
        //If a bit is set, it means that at least one slave in the bus is in the corresponding state:
        //- Bit 0: \a INIT
        //- Bit 1: \a PREOP
        //- Bit 2: \a SAFEOP
        //- Bit 3: \a OP */
        //3,2,1,0:1000=0x08.(OP mode).
        if((master_state.al_states&0x08))
        {
            //unsigned int al_state : 4;
            //The application-layer state of the slave.
            //- 1: \a INIT,- 2: \a PREOP,- 4: \a SAFEOP,- 8: \a OP
            //Note that each state is coded in a different bit!
            if((sc_copley_state[0].al_state!=0x08) || (sc_copley_state[1].al_state!=0x08))
            {
                //do not change FSM,wait for next time checking.
            }else{
                //master in OP mode and all slaves are in OP mode.
                //change to next FSM.
                g_SysFSM=FSM_Homing;
                qDebug()<<"FSM --->>> FSM_Homing";
            }
        }
        break;
    case FSM_Homing:
    {
        int curPos;
        //Mode of Operation.
        //(0x6060,0)=6:Homing mode.
        ecrt_slave_config_sdo8(sc_copley[0],0x6060,0,6);
        ecrt_slave_config_sdo8(sc_copley[1],0x6060,0,6);

        //Home offset=0.
        ecrt_slave_config_sdo32(sc_copley[0],0x607C,0,0);
        ecrt_slave_config_sdo32(sc_copley[1],0x607C,0,0);

        //Home velocity-fast.
        ecrt_slave_config_sdo32(sc_copley[0],0x6099,1,10000);
        ecrt_slave_config_sdo32(sc_copley[1],0x6099,1,10000);
        //Home velocity-slow.
        ecrt_slave_config_sdo32(sc_copley[0],0x6099,2,500);
        ecrt_slave_config_sdo32(sc_copley[1],0x6099,2,500);

        //Homing Acceleration.
        ecrt_slave_config_sdo32(sc_copley[0],0x609A,0,200000);
        ecrt_slave_config_sdo32(sc_copley[1],0x609A,0,200000);

        //Homing method.
        ecrt_slave_config_sdo8(sc_copley[0],0x6098,0,-1);
        ecrt_slave_config_sdo8(sc_copley[1],0x6098,0,-1);
        usleep(100);

        curPos=EC_READ_S32(domainInput_pd+offsetPosActVal[0]);
        EC_WRITE_S32(domainOutput_pd+offsetTarPos[0],EC_READ_S32(domainInput_pd+offsetPosActVal[0]));
        qDebug()<<"slave(0):current positon:"<<curPos;

        curPos=EC_READ_S32(domainInput_pd+offsetPosActVal[1]);
        EC_WRITE_S32(domainOutput_pd+offsetTarPos[1],EC_READ_S32(domainInput_pd+offsetPosActVal[1]));
        qDebug()<<"slave(1):current positon:"<<curPos;

        //0x0006=0000,0000,0000,0110.
        //bit1:Enable Voltage.This bit must be set to enable the amplifier.
        //bit2:Quick Stop. If this bit is clear,then the amplifier is commanded to perform a quick stop.
        EC_WRITE_U16(domainOutput_pd+offsetCtrlWord[0],0x06);
        EC_WRITE_U16(domainOutput_pd+offsetCtrlWord[1],0x06);
        usleep(100);

        //0x0007=0000,0000,0000,0111.
        //bit0:Switch On.This bit must be set to enable the amplifier.
        //bit1:Enable Voltage.This bit must be set to enable the amplifier.
        //bit2:Quick Stop. If this bit is clear,then the amplifier is commanded to perform a quick stop.
        EC_WRITE_U16(domainOutput_pd+offsetCtrlWord[0],0x07);
        EC_WRITE_U16(domainOutput_pd+offsetCtrlWord[1],0x07);
        usleep(100);

        //0x000F=0000,0000,0000,1111.
        //bit0:Switch On.This bit must be set to enable the amplifier.
        //bit1:Enable Voltage.This bit must be set to enable the amplifier.
        //bit2:Quick Stop. If this bit is clear,then the amplifier is commanded to perform a quick stop.
        //bit3:Enable Operation.This bit must be set to enable the amplifier.
        EC_WRITE_U16(domainOutput_pd+offsetCtrlWord[0],0x0F);
        EC_WRITE_U16(domainOutput_pd+offsetCtrlWord[1],0x0F);
        usleep(100);

        EC_WRITE_U16(domainOutput_pd+offsetCtrlWord[0],0x1F);
        EC_WRITE_U16(domainOutput_pd+offsetCtrlWord[1],0x1F);

        g_SysFSM=FSM_CheckHoming;
        qDebug()<<"FSM --->>> FSM_CheckHoming";
    }
        break;
    case FSM_CheckHoming:
    {
        bool bS0HomingOk=false,bS1HomingOk=false;
        uint16_t s0,s1;
        //0x5237:0101,0010,0011,0111.
        //bit12:Homing attained(Homing Mode).
        //bit13:Homing error(Homing Mode).
        //check slave0 status word.
        s0=EC_READ_U16(domainInput_pd+offsetStatusWord[0]);
        if(s0&(0x1<<12)  && (s0&(0x1<<13))==0)
        {
            bS0HomingOk=true;
            qDebug("slave(0): Homing attained without error(0x%x).\n",s0);
        }
        //check slave1 status word.
        s1=EC_READ_U16(domainInput_pd+offsetStatusWord[1]);
        if(s1&(0x1<<12)  && (s1&(0x1<<13))==0)
        {
            bS1HomingOk=true;
            qDebug("slave(1): Homing attained without error(0x%x).\n",s1);
        }

        if(bS0HomingOk && bS1HomingOk)
        {
            //g_SysFSM=FSM_ConfigSlave;
            g_SysFSM=FSM_RunPV;
            qDebug()<<"FSM --->>> FSM_RunPV";
        }
    }
        break;
    case FSM_RunPV:
    {

        //Mode of Operation.
        //(0x6060,0)=3,Profile Velocity mode.
        ecrt_slave_config_sdo8(sc_copley[0],0x6060,0,3);
        //Profile Acceleration.
        ecrt_slave_config_sdo32(sc_copley[0],0x6083,0,1000);
        //Profile Deceleration.
        ecrt_slave_config_sdo32(sc_copley[0],0x6084,0,1000);
        //Target Velocity.
        ecrt_slave_config_sdo32(sc_copley[0],0x60FF,0,100000);
        //Motion Profile Type.
        ecrt_slave_config_sdo16(sc_copley[0],0x6086,0,-1);
        //Profile Velocity.
        ecrt_slave_config_sdo32(sc_copley[0],0x6081,0,100000);

        EC_WRITE_U16(domainOutput_pd+offsetCtrlWord[0],0x0080);
        usleep(100);
        EC_WRITE_U16(domainOutput_pd+offsetCtrlWord[0],0x0006);
        usleep(100);
        EC_WRITE_U16(domainOutput_pd+offsetCtrlWord[0],0x0007);
        usleep(100);
        EC_WRITE_U16(domainOutput_pd+offsetCtrlWord[0],0x000f);
        usleep(100);
        EC_WRITE_U16(domainOutput_pd+offsetCtrlWord[0],0x001f);
        usleep(100);
        g_SysFSM=FSM_IdleStatus;

#if 0
        //Target Velocity(0x60FF,0)
        ecrt_slave_config_sdo32(sc_copley[0],0x60FF,0,28388608);
        ecrt_slave_config_sdo32(sc_copley[1],0x60FF,0,38388608);

        //Reset Fault.
        //A low-to-high transition of this bit makes the amplifier attempt to clear any latched fault condition.
        EC_WRITE_U16(domainOutput_pd+offsetCtrlWord[0],0x0080);
        EC_WRITE_U16(domainOutput_pd+offsetCtrlWord[1],0x0080);
        usleep(100);

        EC_WRITE_U16(domainOutput_pd+offsetCtrlWord[0],0x06);
        EC_WRITE_U16(domainOutput_pd+offsetCtrlWord[1],0x06);
        usleep(100);

        EC_WRITE_U16(domainOutput_pd+offsetCtrlWord[0],0x07);
        EC_WRITE_U16(domainOutput_pd+offsetCtrlWord[1],0x07);
        usleep(100);

        EC_WRITE_U16(domainOutput_pd+offsetCtrlWord[0],0x0F);
        EC_WRITE_U16(domainOutput_pd+offsetCtrlWord[1],0x0F);
        usleep(100);


        EC_WRITE_U16(domainOutput_pd+offsetCtrlWord[0],0x1F);
        EC_WRITE_U16(domainOutput_pd+offsetCtrlWord[1],0x1F);
        g_SysFSM=FSM_IdleStatus;
#endif
    }
        break;
    case FSM_ConfigSlave:
        //Mode of Operation.
        //(0x6060,0)=8 CSP:Cyclic Synchronous Position mode
        ecrt_slave_config_sdo8(sc_copley[0],0x6060,0,8);
        ecrt_slave_config_sdo8(sc_copley[0],0x60c2,1,1);

        ecrt_slave_config_sdo8(sc_copley[1],0x6060,0,8);
        ecrt_slave_config_sdo8(sc_copley[1],0x60c2,1,1);

        //Control Word(0x6040)
        //15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0
        //0 ,0 ,0 ,0 ,0 ,0 ,0,0,1,0,0,0,0,0,0,0
        //bit7=1.
        //Reset Fault.
        //A low-to-high transition of this bit makes the amplifier attempt to clear any latched fault condition.
        EC_WRITE_U16(domainOutput_pd+offsetCtrlWord[0],0x0080);
        EC_WRITE_U16(domainOutput_pd+offsetCtrlWord[1],0x0080);

        //read PositionActualValue(0x6063,int32) from slave0.
        i00TarPos=EC_READ_S32(domainInput_pd+offsetPosActVal[0]);
        EC_WRITE_S32(domainOutput_pd+offsetTarPos[0],i00TarPos);
        printf("slave(0): sync PositionActualValue to TargetPosition:%d\n",i00TarPos);

        //read PositionActualValue(0x6063,int32) from slave1.
        i01TarPos=EC_READ_S32(domainInput_pd+offsetPosActVal[1]);
        EC_WRITE_S32(domainOutput_pd+offsetTarPos[1],i01TarPos);
        printf("slave(1): sync PositionActualValue to TargetPosition:%d\n",i01TarPos);


        //0x0006=0000,0000,0000,0110.
        //bit1:Enable Voltage.This bit must be set to enable the amplifier.
        //bit2:Quick Stop. If this bit is clear,then the amplifier is commanded to perform a quick stop.
        EC_WRITE_U16(domainOutput_pd+offsetCtrlWord[0],0x06);
        EC_WRITE_U16(domainOutput_pd+offsetCtrlWord[1],0x06);

        //0x0007=0000,0000,0000,0111.
        //bit0:Switch On.This bit must be set to enable the amplifier.
        //bit1:Enable Voltage.This bit must be set to enable the amplifier.
        //bit2:Quick Stop. If this bit is clear,then the amplifier is commanded to perform a quick stop.
        EC_WRITE_U16(domainOutput_pd+offsetCtrlWord[0],0x07);
        EC_WRITE_U16(domainOutput_pd+offsetCtrlWord[1],0x07);

        //0x000F=0000,0000,0000,1111.
        //bit0:Switch On.This bit must be set to enable the amplifier.
        //bit1:Enable Voltage.This bit must be set to enable the amplifier.
        //bit2:Quick Stop. If this bit is clear,then the amplifier is commanded to perform a quick stop.
        //bit3:Enable Operation.This bit must be set to enable the amplifier.
        EC_WRITE_U16(domainOutput_pd+offsetCtrlWord[0],0x0F);
        EC_WRITE_U16(domainOutput_pd+offsetCtrlWord[1],0x0F);

        g_SysFSM=FSM_CheckStatus;
        qDebug()<<"FSM --->>> FSM_CheckStatus";
        break;
    case FSM_CheckStatus:
    {
        //Status Word(0x6041)
        //0x0004:0000,0000,0000,0100
        //bit2: Operation Enabled.Set when the amplifier is enabled.
        bool bSlave0Fault=false,bSlave1Fault=false;
        uint16_t s0,s1;
        //check slave0 status word.
        s0=EC_READ_U16(domainInput_pd+offsetStatusWord[0]);
        if((s0&0x0004)==0)
        {
            //bit2 was not set by slave,something wrong happened.
            bSlave0Fault=true;
            //0x0100:0000,0001,0000,0000
            //bit8:Set if the last trajectory was aborted rather than finishing normally.
            if(s0&0x0100)
            {
                qDebug()<<"slave(0):the last trajectory was aborted rather than finishing normally.";
            }
            //0x0800:0000,1000,0000,0000
            //bit11:Internal Limit Active.
            //This bit is set when one of the amplifier limits(current,voltage,velocity or position) is active.
            if(s0&0x0800)
            {
                qDebug()<<"slave(0):Internal Limit Active.";
            }
        }
        //check slave1 status word.
        s1=EC_READ_U16(domainInput_pd+offsetStatusWord[1]);
        if((s1&0x0004)==0)
        {
            //bit2 was not set by slave,something wrong happened.
            bSlave1Fault=true;
            //0x0100:0000,0001,0000,0000
            //bit8:Set if the last trajectory was aborted rather than finishing normally.
            if(s1&0x0100)
            {
                qDebug()<<"slave(1):the last trajectory was aborted rather than finishing normally.";
            }
            //0x0800:0000,1000,0000,0000
            //bit11:Internal Limit Active.
            //This bit is set when one of the amplifier limits(current,voltage,velocity or position) is active.
            if(s1&0x0800)
            {
                qDebug()<<"slave(1):Internal Limit Active.";
            }
        }
        //re-init slaves whatever which slave has fault.
        if(bSlave0Fault || bSlave1Fault)
        {
            qDebug()<<"5s re-config slaves!";
            usleep(1000*1000*5);
            g_SysFSM=FSM_ConfigSlave;
        }else{
            //all slaves status word are okay.
            g_SysFSM=FSM_DoMove;
            qDebug()<<"FSM --->>> FSM_DoMove";
        }
    }
        break;
    case FSM_DoMove:
    {
        if(!(cycle_counter%100))
        {
            uint16_t status;
            float act_velocity;
            int act_position;

            status = EC_READ_U16(domainInput_pd + offsetStatusWord[0]);
            act_velocity = EC_READ_S32(domainInput_pd + offsetActVel[0])/1000.0;
            act_position = EC_READ_S32(domainInput_pd + offsetPosActVal[0]);
            printf("\n\n0:aclVel=%.1f rpm,aclPos=%d,",act_velocity, act_position);
            printf("Status Word=0x%x\n",status);
            print_bits_splitter(status);
            gStatusWord=status;
            gActVelocity=act_velocity;
            gActPosition=act_position;
            gTarPosition=0;

            status = EC_READ_U16(domainInput_pd + offsetStatusWord[1]);
            act_velocity = EC_READ_S32(domainInput_pd + offsetActVel[1])/1000.0;
            act_position = EC_READ_S32(domainInput_pd + offsetPosActVal[1]);
            printf("\n\n1:aclVel=%.1f rpm,aclPos=%d,",act_velocity, act_position);
            printf("Status Word=0x%x\n",status);
            print_bits_splitter(status);
            gStatusWord2=status;
            gActVelocity2=act_velocity;
            gActPosition2=act_position;
            gTarPosition2=0;
        }

        //curpos+=200;//clockwise direction.
        //curpos-=200;//anti-clockwise direction.
        //if this value is set bigger,will cause error.
        //we can check status word to see what happened exactly.
        //curpos+=100;
        //curpos-=10;

        //slave0 move by +100/-100 positon.

        i00TarPos+=200;
        i01TarPos+=100;
        EC_WRITE_S32(domainOutput_pd+offsetTarPos[0],i00TarPos);

        //slave1 move by +100/-100 positon.
        //gGblPara.m_i01PosActVal-=100;

        EC_WRITE_S32(domainOutput_pd+offsetTarPos[1],i01TarPos);
        //        ecstate=0;
        //        g_SysFSM=FSM_SafeOp;

        //ecrt_master_reset(master).
    }
        break;
    case FSM_IdleStatus:
    {
        uint16_t s0,s1;
        s0 = EC_READ_U16(domainInput_pd + offsetStatusWord[0]);
        s1 = EC_READ_U16(domainInput_pd + offsetStatusWord[1]);
        qDebug("s0: 0x%x, s1: 0x%x\n",s0,s1);
        //0x5237: 0101,0010,0011,0111
    }
        break;
    default:
        break;
    }
    /*send process data*/
    ecrt_domain_queue(domainOutput);
    ecrt_domain_queue(domainInput);
    ecrt_master_send(master);
}
int g_do_init()
{
    g_SysFSM=FSM_Power_On;
    //Lock memory.(will faile if we are not root).
#if 0
    if(mlockall( MCL_CURRENT|MCL_FUTURE )==-1)
    {
        perror("mlockall failed");
        return -1;
    }
#endif

    //Requests an EtherCAT master for realtime operation.
    master=ecrt_request_master(0);
    if(!master)
    {
        printf("error:failed to request master 0!\n");
        return -1;
    }
    printf("request master 0 okay.\n");

    for(int i=0;i<2;i++)
    {
        ec_slave_info_t scInfo;
        if(ecrt_master_get_slave(master,i,&scInfo)==0)
        {
            printf("slave(%d):%x,%x,%s\n",i,scInfo.vendor_id,scInfo.product_code,scInfo.name);
        }
    }

    //Creates a new process data domain.
    //For process data exchange, at least one process data domain is needed.
    //This method creates a new process data domain and returns a pointer to the new domain object.
    //This object can be used for registering PDOs and exchanging them in cyclic operation.
    domainInput=ecrt_master_create_domain(master);
    domainOutput=ecrt_master_create_domain(master);
    if(!domainInput || !domainOutput)
    {
        printf("error:failed to create domain Input/Output!\n");
        return -1;
    }
    printf("create domain okay.\n");

    //Obtains a slave configuration.
    //If the slave with the given address is found during the bus configuration,
    //its vendor ID and product code are matched against the given value.
    //On mismatch, the slave is not configured and an error message is raised.
    if(!(sc_copley[0]=ecrt_master_slave_config(master,CopleySlavePos,Copley_VID_PID)))
    {
        printf("error:failed to get slave configuration for Copley-0.\n");
        return -1;
    }
    if(!(sc_copley[1]=ecrt_master_slave_config(master,CopleySlavePos2,Copley_VID_PID)))
    {
        printf("error:failed to get slave configuration for Copley-1.\n");
        return -1;
    }
    printf("get slave configuration okay!\n");

    //Specify a complete PDO configuration.
    //This function is a convenience wrapper for the functions
    //ecrt_slave_config_sync_manager(), ecrt_slave_config_pdo_assign_clear(),
    //ecrt_slave_config_pdo_assign_add(), ecrt_slave_config_pdo_mapping_clear()
    //and ecrt_slave_config_pdo_mapping_add(), that are better suitable for
    //automatic code generation.
    printf("Configuring PDOs...\n");
    if(ecrt_slave_config_pdos(sc_copley[0],EC_END,copley_syncs))
    {
        printf("error:failed to configure Copley PDOs.\n" );
        return -1;
    }
    if(ecrt_slave_config_pdos(sc_copley[1],EC_END,copley_syncs))
    {
        printf("error:failed to configure Copley PDOs.\n" );
        return -1;
    }
    printf("configure PDOs done.\n");

    if(ecrt_domain_reg_pdo_entry_list(domainInput,domainInput_regs))
    {
        printf("error:PDO entry registration failed!\n" );
        return -1;
    }
    if(ecrt_domain_reg_pdo_entry_list(domainOutput,domainOutput_regs))
    {
        printf("error:PDO entry registration failed!\n" );
        return -1;
    }
    printf("PDO entry registration done.\n");

    printf("Creating SDO request...\n");
    //    //Mode of Operation.
    //    //(0x6060,0)=8 CSP:Cyclic Synchronous Position mode
    //    ecrt_slave_config_sdo8(sc_copley[0],0x6060,0,8);
    //    ecrt_slave_config_sdo8(sc_copley[0],0x60c2,1,1);

    //    ecrt_slave_config_sdo8(sc_copley[1],0x6060,0,8);
    //    ecrt_slave_config_sdo8(sc_copley[1],0x60c2,1,1);


    //Finishes the configuration phase and prepares for cyclic operation.
    //This function tells the master that the configuration phase is finished and the realtime operation will begin.
    //The function allocates internal memory for the domains and calculates the logical FMMU addresses for domain members.
    //It tells the master state machine that the bus configuration is now to be applied.
    printf("Activating master...\n");
    if(ecrt_master_activate(master))
    {
        printf("error:failed to activate master!\n");
        return -1;
    }
    printf("master activated.\n");

    // Returns the domain's process data.
    if(!(domainInput_pd=ecrt_domain_data(domainInput)))
    {
        printf("error:domain data error!\n");
        return -1;
    }
    if(!(domainOutput_pd=ecrt_domain_data(domainOutput)))
    {
        printf("error:domain data error!\n");
        return -1;
    }

    g_SysFSM=FSM_SafeOp;
    return 0;
}
void g_do_uninit()
{
    ecrt_release_master(master);
    master=NULL;
    munlockall();
}
ZEtherCATThread::ZEtherCATThread()
{

}
void ZEtherCATThread::run()
{
    if(g_do_init()<0)
    {
        return;
    }
    while(!gGblPara.m_bExitFlag)
    {
        if(1/*gGblPara.m_iSlavesEnBitMask!=0*/)
        {
            //1000us=1ms.
            //usleep(1000000/TASK_FREQUENCY);
            //if the time is less, the motor has no time to run.
            //so we set the time longer to wait for the motor executed the previous command.
            usleep(3000);
            //usleep(8000);
            //usleep(10000);
            //usleep(20000);
            cyclic_task();

            emit this->ZSigPDO(0,gActPosition,gTarPosition,gActVelocity,gStatusWord);
            emit this->ZSigPDO(1,gActPosition2,gTarPosition2,gActVelocity2,gStatusWord2);
        }else{
            usleep(1000*1000);
        }
    }
    g_do_uninit();
    return;
}
