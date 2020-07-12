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
    FSM_CheckHomingAttained,
    FSM_RunPV,
    FSM_CfgCSP,
    FSM_RunCSP,
    FSM_IdleStatus,
};
int g_SysFSM=FSM_Power_On;

/*EtherCAT master*/
static ec_master_t *master=NULL;
static ec_master_state_t master_state={};
/*slave process data input,used to write slave PDO, master->slave.*/
static ec_domain_t *domainIn=NULL;
static ec_domain_state_t domainIn_state={};
static uint8_t *domainIn_pd=NULL;
/*slave process data tx*/
static ec_domain_t *domainOut=NULL;
static ec_domain_state_t domainOut_state={};
static uint8_t *domainOut_pd=NULL;
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
static uint32_t offsetPosError[2];
static uint32_t offsetTorActVal[2];

//define slave0 PDO entries.
static ec_pdo_entry_info_t slave0_pdo_entries[] = {
    /*RxPDO*/
    { 0x6040, 0x00, 16 }, /*Control Word,uint16*/
    { 0x607A, 0x00, 32 }, /*Profile Target Position,uint32*/

    /*TxPDO*/
    { 0x6041, 0x00, 16 }, /*Status Word,uint16*/
    { 0x6064, 0x00, 32 }, /*Position Actual Value,uint32*/
    { 0x60F4, 0x00, 32 }, /*Position Error,uint32*/
    { 0x606C, 0x00, 32 }, /*Actual Velocity,uint32*/

    /*Another TxPDO*/
    { 0x6077, 0x00, 32 }, /*Torque Actual Value,uint32*/
};
//define slave0 PDO itself.
static ec_pdo_info_t slave0_pdos[] = {
    { 0x1600, 2, slave0_pdo_entries },/*RxPDO-1*/
    { 0x1a00, 4, slave0_pdo_entries+2 },/*TxPDO-1*/
    { 0x1a01, 1, slave0_pdo_entries+6 },/*TxPDO-2*/
};
//define slave0 Sync Manager.
static ec_sync_info_t slave0_syncs[] = {
    { 0, EC_DIR_OUTPUT, 0, NULL, EC_WD_DISABLE },//SM0:MBoxOut.
    { 1, EC_DIR_INPUT, 0, NULL, EC_WD_DISABLE },//SM1:MBoxIn.
    { 2, EC_DIR_OUTPUT, 1, slave0_pdos, EC_WD_DISABLE },//SM2:Outputs.
    { 3, EC_DIR_INPUT, 2, slave0_pdos+1, EC_WD_DISABLE },//SM3:Inputs.
    { 0xFF }
};
//define slave1 PDO entries.
static ec_pdo_entry_info_t slave1_pdo_entries[] = {
    /*RxPDO*/
    { 0x6040, 0x00, 16 }, /*Control Word,uint16*/
    { 0x607A, 0x00, 32 }, /*Profile Target Position,uint32*/

    /*TxPDO*/
    { 0x6041, 0x00, 16 }, /*Status Word,uint16*/
    { 0x6064, 0x00, 32 }, /*Position Actual Value,uint32*/
    { 0x60F4, 0x00, 32 }, /*Position Error,uint32*/
    { 0x606C, 0x00, 32 }, /*Actual Velocity,uint32*/

    /*Another TxPDO*/
    { 0x6077, 0x00, 32 }, /*Torque Actual Value,uint32*/
};
//define slave1 PDO itself.
static ec_pdo_info_t slave1_pdos[] = {
    { 0x1600, 2, slave1_pdo_entries },/*RxPDO-1*/
    { 0x1a00, 4, slave1_pdo_entries+2 },/*TxPDO-1*/
    { 0x1a01, 1, slave1_pdo_entries+6 },/*TxPDO-2*/
};
//define slave1 Sync Manager.
static ec_sync_info_t slave1_syncs[] = {
    { 0, EC_DIR_OUTPUT, 0, NULL, EC_WD_DISABLE },//SM0:MBoxOut.
    { 1, EC_DIR_INPUT, 0, NULL, EC_WD_DISABLE },//SM1:MBoxIn.
    { 2, EC_DIR_OUTPUT, 1, slave1_pdos, EC_WD_DISABLE },//SM2:Outputs.
    { 3, EC_DIR_INPUT, 2, slave1_pdos+1, EC_WD_DISABLE },//SM3:Inputs.
    { 0xFF }
};
const static ec_pdo_entry_reg_t domainIn_regs[]=
{
    //RxPDOs.
    { CopleySlavePos, Copley_VID_PID, 0x6040, 0, &offsetCtrlWord[0], NULL },
    { CopleySlavePos, Copley_VID_PID, 0x607A, 0, &offsetTarPos[0], NULL },

    { CopleySlavePos2, Copley_VID_PID, 0x6040, 0, &offsetCtrlWord[1], NULL },
    { CopleySlavePos2, Copley_VID_PID, 0x607A, 0, &offsetTarPos[1], NULL },
    {}
};
const static ec_pdo_entry_reg_t domainOut_regs[]=
{
    //TxPDOs.
    { CopleySlavePos, Copley_VID_PID, 0x6041, 0, &offsetStatusWord[0], NULL },
    { CopleySlavePos, Copley_VID_PID, 0x6064, 0, &offsetPosActVal[0], NULL },
    { CopleySlavePos, Copley_VID_PID, 0x60F4, 0, &offsetPosError[0], NULL },
    { CopleySlavePos, Copley_VID_PID, 0x606C, 0, &offsetActVel[0], NULL },
    { CopleySlavePos, Copley_VID_PID, 0x6077, 0, &offsetTorActVal[0], NULL },

    { CopleySlavePos2, Copley_VID_PID, 0x6041, 0, &offsetStatusWord[1], NULL },
    { CopleySlavePos2, Copley_VID_PID, 0x6064, 0, &offsetPosActVal[1], NULL },
    { CopleySlavePos2, Copley_VID_PID, 0x60F4, 0, &offsetPosError[1], NULL },
    { CopleySlavePos2, Copley_VID_PID, 0x606C, 0, &offsetActVel[1], NULL },
    { CopleySlavePos2, Copley_VID_PID, 0x6077, 0, &offsetTorActVal[1], NULL },

    {}
};

void check_domain_state(void)
{
    ec_domain_state_t ds1={};
    ec_domain_state_t ds2={};

    //domainInput.
    //Reads the state of a domain.
    //Using this method, the process data exchange can be monitored in realtime.
    ecrt_domain_state(domainIn,&ds1);
    if(ds1.working_counter!=domainIn_state.working_counter)
    {
        printf("domainInput: WC %u.\n",ds1.working_counter);
    }
    if(ds1.wc_state!=domainIn_state.wc_state)
    {
        printf("domainInput: State %u.\n",ds1.wc_state);
    }
    domainIn_state=ds1;

    //domainOutput.
    //Reads the state of a domain.
    //Using this method, the process data exchange can be monitored in realtime.
    ecrt_domain_state(domainOut,&ds2);
    if(ds2.working_counter!=domainOut_state.working_counter)
    {
        printf("domainOutput: WC %u.\n",ds2.working_counter);
    }
    if(ds2.wc_state!=domainOut_state.wc_state)
    {
        printf("domainOutput: State %u.\n",ds2.wc_state);
    }
    domainOut_state=ds2;
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
void ZEtherCATThread::ZDoCyclicTask()
{
    static int i00TarPos=0,i01TarPos=0;
    static int cycle_counter=0;
    cycle_counter++;
    if(cycle_counter>=1000*2)
    {
        cycle_counter=0;
    }

    //Retrieves datagram from ethercat frame.
    //NIC driver -> Master module
    ecrt_master_receive(master);

    //Processing datagram (Master module -> domain).
    ecrt_domain_process(domainIn);
    ecrt_domain_process(domainOut);

    //not mandatory.
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
                emit this->ZSigLog(false,"FSM --->>> FSM_Homing");
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

        //master read from slave domain Wr process data.
        //domainWr_pd:  slave output data to master.
        //domainRd_pd:  slave input data from master.
        curPos=EC_READ_S32(domainOut_pd+offsetPosActVal[0]);
        EC_WRITE_S32(domainIn_pd+offsetTarPos[0],EC_READ_S32(domainOut_pd+offsetPosActVal[0]));
        qDebug()<<"slave(0):current positon:"<<curPos;

        curPos=EC_READ_S32(domainOut_pd+offsetPosActVal[1]);
        EC_WRITE_S32(domainIn_pd+offsetTarPos[1],EC_READ_S32(domainOut_pd+offsetPosActVal[1]));
        qDebug()<<"slave(1):current positon:"<<curPos;

        //0x0006=0000,0000,0000,0110.
        //bit1:Enable Voltage.This bit must be set to enable the amplifier.
        //bit2:Quick Stop. If this bit is clear,then the amplifier is commanded to perform a quick stop.
        EC_WRITE_U16(domainIn_pd+offsetCtrlWord[0],0x06);
        EC_WRITE_U16(domainIn_pd+offsetCtrlWord[1],0x06);
        usleep(100);

        //0x0007=0000,0000,0000,0111.
        //bit0:Switch On.This bit must be set to enable the amplifier.
        //bit1:Enable Voltage.This bit must be set to enable the amplifier.
        //bit2:Quick Stop. If this bit is clear,then the amplifier is commanded to perform a quick stop.
        EC_WRITE_U16(domainIn_pd+offsetCtrlWord[0],0x07);
        EC_WRITE_U16(domainIn_pd+offsetCtrlWord[1],0x07);
        usleep(100);

        //0x000F=0000,0000,0000,1111.
        //bit0:Switch On.This bit must be set to enable the amplifier.
        //bit1:Enable Voltage.This bit must be set to enable the amplifier.
        //bit2:Quick Stop. If this bit is clear,then the amplifier is commanded to perform a quick stop.
        //bit3:Enable Operation.This bit must be set to enable the amplifier.
        EC_WRITE_U16(domainIn_pd+offsetCtrlWord[0],0x0F);
        EC_WRITE_U16(domainIn_pd+offsetCtrlWord[1],0x0F);
        usleep(100);

        EC_WRITE_U16(domainIn_pd+offsetCtrlWord[0],0x1F);
        EC_WRITE_U16(domainIn_pd+offsetCtrlWord[1],0x1F);

        g_SysFSM=FSM_CheckHomingAttained;
        emit this->ZSigLog(false,"FSM --->>> FSM_CheckHomingAttained");
    }
        break;
    case FSM_CheckHomingAttained:
    {
        bool bS0HomingOk=false,bS1HomingOk=false;
        uint16_t s0,s1;
        //0x5237:0101,0010,0011,0111.
        //bit12:Homing attained(Homing Mode).
        //bit13:Homing error(Homing Mode).
        //check slave0 status word.
        s0=EC_READ_U16(domainOut_pd+offsetStatusWord[0]);
        if(s0&(0x1<<12)  && (s0&(0x1<<13))==0)
        {
            bS0HomingOk=true;
            emit this->ZSigLog(false,QString("slave(0): Homing attained."));
        }
        //check slave1 status word.
        s1=EC_READ_U16(domainOut_pd+offsetStatusWord[1]);
        if(s1&(0x1<<12)  && (s1&(0x1<<13))==0)
        {
            bS1HomingOk=true;
            emit this->ZSigLog(false,QString("slave(1): Homing attained."));
        }

        if(bS0HomingOk && bS1HomingOk)
        {
            g_SysFSM=FSM_CfgCSP;
            //g_SysFSM=FSM_RunPV;
            emit this->ZSigLog(false,"FSM --->>> FSM_CfgCSP");
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

        EC_WRITE_U16(domainIn_pd+offsetCtrlWord[0],0x0080);
        usleep(100);
        EC_WRITE_U16(domainIn_pd+offsetCtrlWord[0],0x0006);
        usleep(100);
        EC_WRITE_U16(domainIn_pd+offsetCtrlWord[0],0x0007);
        usleep(100);
        EC_WRITE_U16(domainIn_pd+offsetCtrlWord[0],0x000f);
        usleep(100);
        EC_WRITE_U16(domainIn_pd+offsetCtrlWord[0],0x001f);
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
    case FSM_CfgCSP:
        //setup "Mode of Operation" via SDO.
        //Slave-0.
        //(0x6060,0)=8 CSP:Cyclic Synchronous Position mode
        ecrt_slave_config_sdo8(sc_copley[0],0x6060,0,8);
        //ecrt_slave_config_sdo8(sc_copley[0],0x60c2,1,100);
        //Motion Profile Type(0x6086).
        ecrt_slave_config_sdo16(sc_copley[0],0x6086,0,0);
        //Target Positon(0x607A).
        //Profile Velocity(0x6081).
        ecrt_slave_config_sdo32(sc_copley[0],0x6081,0,8000000);
        //Profile Acceleration(0x6083).
        ecrt_slave_config_sdo32(sc_copley[0],0x6083,0,5000);
        //Profile Deceleration(0x6084).
        ecrt_slave_config_sdo32(sc_copley[0],0x6084,0,5000);
        //Trajectory Jerk Limit(0x2121).

        //Slave-1.
        //(0x6060,0)=8 CSP:Cyclic Synchronous Position mode
        ecrt_slave_config_sdo8(sc_copley[1],0x6060,0,8);
        //ecrt_slave_config_sdo8(sc_copley[1],0x60c2,1,1);
        //Motion Profile Type(0x6086).
        ecrt_slave_config_sdo16(sc_copley[1],0x6086,0,0);
        //Target Positon(0x607A).
        //Profile Velocity(0x6081).
        ecrt_slave_config_sdo32(sc_copley[1],0x6081,0,8000000);
        //Profile Acceleration(0x6083).
        ecrt_slave_config_sdo32(sc_copley[1],0x6083,0,5000);
        //Profile Deceleration(0x6084).
        ecrt_slave_config_sdo32(sc_copley[1],0x6084,0,5000);
        //Trajectory Jerk Limit(0x2121).

        g_SysFSM=FSM_RunCSP;
        emit this->ZSigLog(false,"FSM --->>> FSM_RunCSP");
        break;
    case FSM_RunCSP:
    {
        uint16_t cmd;
        uint16_t s0,s1;
        int curPos0,curPos1;
        int tarPos0,tarPos1;
        curPos0=EC_READ_S32(domainOut_pd + offsetPosActVal[0]);
        curPos1=EC_READ_S32(domainOut_pd + offsetPosActVal[1]);
        tarPos0=curPos0;
        tarPos1=curPos1;
        //Status Word(0x6041).
        //bit6,bit5,bit3,bit2,bit1,bit0:determine the current states.
        s0=EC_READ_U16(domainOut_pd+offsetStatusWord[0]);
        s1=EC_READ_U16(domainOut_pd+offsetStatusWord[1]);
        //qDebug("s0: 0x%x, s1: 0x%x\n",s0,s1);

        //slave0 finite state machine.
        if((s0&0x004F)==0x0040)
        {
            //we conly concern bit6,bit3,bit2,bit1,bit0 in Switch On Disabled,
            //so we use 0000,0000,0100,1111=0x4F as the bit mask.

            //xxxx,xxxx,x1xx,0000=0x0040,Switch on Disabled.
            emit this->ZSigLog(false,"slave(0) in Switch on Disabled.");

            //issue "shutdown" command,From [Switch On Disabled] to [Ready to Switch On].
            //shutdown:bit7=0,bit3=x,bit2=1,bit1=1,bit0=0.
            //0000,0000,0000,0110=0x0006.
            cmd=0x0006;
        }else if((s0&0x006F)==0x0021)
        {
            //we conly concern bit6,bit3,bit2,bit1,bit0 in Ready to switch on,
            //so we use 0000,0000,0110,1111=0x6F as the bit mask.

            //xxxx,xxxx,x01x,0001=0x0021,Ready to switch on.
            emit this->ZSigLog(false,"slave(0) in Ready to switch on.");
            cmd=0x0007;
        }else if((s0&0x006F)==0x0023)
        {
            //we conly concern bit6,bit5,bit3,bit2,bit1,bit0 in Switch on,
            //so we use 0000,0000,0110,1111=0x006F as the bit mask.

            //xxxx,xxxx,x01x,0011=0x0023,Switch on.
            emit this->ZSigLog(false,"slave(0) in Switch on.");
            cmd=0x000F;
        }else if((s0&0x006F)==0x0027)
        {
            int iCurrentPos;
            //we conly concern bit6,bit5,bit3,bit2,bit1,bit0 in Operation Enabled,
            //so we use 0000,0000,0110,1111=0x006F as the bit mask.

            //xxxx,xxxx,x01x,0111=0x0027,Operation Enabled.
            //emit this->ZSigLog(false,"slave(0) in Operation Enabled.");
            cmd=0x001F;

            //slave0: up/down direction control.
            //we move by a small step to avoid amplifier driver error.
            int iMoveStep=200;
            if(gGblPara.m_pixelDiffY>0)//(gGblPara.m_pixelDiffY>0), move torward to down.
            {
                qDebug("currentPos:%d,diffY:%d\n",curPos0,gGblPara.m_pixelDiffY);
                if(gGblPara.m_pixelDiffY>=iMoveStep)
                {
                    qDebug("Y Move down by %d!\n",iMoveStep);
                    tarPos0+=iMoveStep;
                    gGblPara.m_pixelDiffY-=iMoveStep;
                }else{
                    qDebug("Y Move down by %d!\n",gGblPara.m_pixelDiffY);
                    tarPos0+=gGblPara.m_pixelDiffY;
                    gGblPara.m_pixelDiffY=0;
                }
            }else if(gGblPara.m_pixelDiffY<0)//if (gGblPara.m_pixelDiffY<0), move torward to up.
            {
                qDebug("currentPos:%d,diffY:%d\n",iCurrentPos,gGblPara.m_pixelDiffY);
                if(gGblPara.m_pixelDiffY<=-iMoveStep)
                {
                    qDebug("Y Move up by %d!\n",iMoveStep);
                    tarPos0-=iMoveStep;
                    gGblPara.m_pixelDiffY+=iMoveStep;
                }else{
                    qDebug("Y Move up by %d!\n",gGblPara.m_pixelDiffY);
                    tarPos0-=gGblPara.m_pixelDiffY;
                    gGblPara.m_pixelDiffY=0;
                }
            }else{
                //qDebug()<<"No need to move!";
            }

            //read related PDOs.
            int iPosActVal=EC_READ_S32(domainOut_pd + offsetPosActVal[0]);
            int iPosErr=EC_READ_S32(domainOut_pd + offsetPosError[0]);
            int iActVel=EC_READ_S32(domainOut_pd + offsetActVel[0]);
            int iTorActVal=EC_READ_S32(domainOut_pd + offsetTorActVal[0]);
            emit this->ZSigPDO(0,iPosActVal,tarPos0,iActVel);
        }else{
            //0x0100:0000,0001,0000,0000
            //bit8:Set if the last trajectory was aborted rather than finishing normally.
            if(s0&0x0100)
            {
                emit this->ZSigLog(true,"slave(0):the last trajectory was aborted rather than finishing normally.");
            }
            //0x0800:0000,1000,0000,0000
            //bit11:Internal Limit Active.
            //This bit is set when one of the amplifier limits(current,voltage,velocity or position) is active.
            if(s0&0x0800)
            {
                emit this->ZSigLog(true,"slave(0):Internal Limit Active.");
            }

            //Fault.
            //bit7:Reset Fault.
            //A low-to-high transition of this bit makes the amplifier attemp to clear any latched fault condition.
            cmd=0x0080;
        }

        //slave1 finite state machine.
        if((s1&0x004F)==0x0040)
        {
            //we conly concern bit6,bit3,bit2,bit1,bit0 in Switch On Disabled,
            //so we use 0000,0000,0100,1111=0x4F as the bit mask.

            //xxxx,xxxx,x1xx,0000=0x0040,Switch on Disabled.
            emit this->ZSigLog(false,"slave(1) in Switch on Disabled.");

            //issue "shutdown" command,From [Switch On Disabled] to [Ready to Switch On].
            //shutdown:bit7=0,bit3=x,bit2=1,bit1=1,bit0=0.
            //0000,0000,0000,0110=0x0006.
            cmd=0x0006;
        }else if((s1&0x006F)==0x0021)
        {
            //we conly concern bit6,bit3,bit2,bit1,bit0 in Ready to switch on,
            //so we use 0000,0000,0110,1111=0x6F as the bit mask.

            //xxxx,xxxx,x01x,0001=0x0021,Ready to switch on.
            emit this->ZSigLog(false,"slave(1) in Ready to switch on.");
            cmd=0x0007;
        }else if((s1&0x006F)==0x0023)
        {
            //we conly concern bit6,bit5,bit3,bit2,bit1,bit0 in Switch on,
            //so we use 0000,0000,0110,1111=0x006F as the bit mask.

            //xxxx,xxxx,x01x,0011=0x0023,Switch on.
            emit this->ZSigLog(false,"slave(1) in Switch on.");
            cmd=0x000F;
        }else if((s1&0x006F)==0x0027)
        {
            int iCurrentPos;
            //we conly concern bit6,bit5,bit3,bit2,bit1,bit0 in Operation Enabled,
            //so we use 0000,0000,0110,1111=0x006F as the bit mask.

            //xxxx,xxxx,x01x,0111=0x0027,Operation Enabled.
            //emit this->ZSigLog(false,"slave(0) in Operation Enabled.");
            cmd=0x001F;

            //slave0: up/down direction control.
            //we move by a small step to avoid amplifier driver error.
            int iMoveStep=200;
            if(gGblPara.m_pixelDiffX>0)//(gGblPara.m_pixelDiffX>0), move torward to left.
            {
                qDebug("currentPos:%d,diffX:%d\n",iCurrentPos,gGblPara.m_pixelDiffX);
                if(gGblPara.m_pixelDiffX>=iMoveStep)
                {
                    qDebug("X Move down by %d!\n",iMoveStep);
                    tarPos1-=iMoveStep;
                    gGblPara.m_pixelDiffX-=iMoveStep;
                }else{
                    qDebug("X Move down by %d!\n",gGblPara.m_pixelDiffX);
                    tarPos1-=gGblPara.m_pixelDiffX;
                    gGblPara.m_pixelDiffX=0;
                }
            }else if(gGblPara.m_pixelDiffX<0)//if (gGblPara.m_pixelDiffY<0), move torward to right.
            {
                qDebug("currentPos:%d,diffY:%d\n",curPos1,gGblPara.m_pixelDiffX);
                if(gGblPara.m_pixelDiffX<=-iMoveStep)
                {
                    qDebug("Y Move up by %d!\n",iMoveStep);
                    tarPos1+=iMoveStep;
                    gGblPara.m_pixelDiffX+=iMoveStep;
                }else{
                    qDebug("Y Move up by %d!\n",gGblPara.m_pixelDiffX);
                    tarPos1+=gGblPara.m_pixelDiffX;
                    gGblPara.m_pixelDiffX=0;
                }
            }else{
                //qDebug()<<"No need to move!";
            }

            //read related PDOs.
            int iPosActVal=EC_READ_S32(domainOut_pd + offsetPosActVal[1]);
            int iPosErr=EC_READ_S32(domainOut_pd + offsetPosError[1]);
            int iActVel=EC_READ_S32(domainOut_pd + offsetActVel[1]);
            int iTorActVal=EC_READ_S32(domainOut_pd + offsetTorActVal[1]);
            emit this->ZSigPDO(1,iPosActVal,tarPos1,iActVel);
        }else{
            //0x0100:0000,0001,0000,0000
            //bit8:Set if the last trajectory was aborted rather than finishing normally.
            if(s1&0x0100)
            {
                emit this->ZSigLog(true,"slave(1):the last trajectory was aborted rather than finishing normally.");
            }
            //0x0800:0000,1000,0000,0000
            //bit11:Internal Limit Active.
            //This bit is set when one of the amplifier limits(current,voltage,velocity or position) is active.
            if(s1&0x0800)
            {
                emit this->ZSigLog(true,"slave(1):Internal Limit Active.");
            }

            //Fault.
            //bit7:Reset Fault.
            //A low-to-high transition of this bit makes the amplifier attemp to clear any latched fault condition.
            cmd=0x0080;
        }

        //write Ctrl Word & Target Position.
        if(tarPos0!=curPos0)
        {
            EC_WRITE_S32(domainIn_pd+offsetTarPos[0],tarPos0);
        }
        if(tarPos1!=curPos1)
        {
            EC_WRITE_S32(domainIn_pd+offsetTarPos[1],tarPos1);
        }
        EC_WRITE_U16(domainIn_pd+offsetCtrlWord[0],cmd);
        EC_WRITE_U16(domainIn_pd+offsetCtrlWord[1],cmd);
    }
        break;
    case FSM_IdleStatus:
    {
        uint16_t s0,s1;
        s0 = EC_READ_U16(domainOut_pd + offsetStatusWord[0]);
        s1 = EC_READ_U16(domainOut_pd + offsetStatusWord[1]);
        qDebug("s0: 0x%x, s1: 0x%x\n",s0,s1);
        //0x5237: 0101,0010,0011,0111
    }
        break;
    default:
        break;
    }
    /*send process data*/
    ecrt_domain_queue(domainOut);
    ecrt_domain_queue(domainIn);
    ecrt_master_send(master);
}
ZEtherCATThread::ZEtherCATThread()
{

}
void ZEtherCATThread::run()
{
    int iThreadExitCode=0;
    do{
        g_SysFSM=FSM_Power_On;

        //lock all currently-mapped pages to prevent page swapping.
        //(will faile if we are not root).
        if(mlockall( MCL_CURRENT|MCL_FUTURE )==-1)
        {
            emit this->ZSigLog(true,"failed to mlockall.");
        }

        //Requests an EtherCAT master for realtime operation.
        master=ecrt_request_master(0);
        if(!master)
        {
            emit this->ZSigLog(true,"failed to request master(0).");
            iThreadExitCode=-1;
            break;
        }
        emit this->ZSigLog(false,"request master(0) okay.");

        for(int i=0;i<2;i++)
        {
            ec_slave_info_t scInfo;
            if(ecrt_master_get_slave(master,i,&scInfo)==0)
            {
                char buffer[256];
                sprintf(buffer,"find slave(%d):0x%x,0x%x,%s.",i,scInfo.vendor_id,scInfo.product_code,scInfo.name);
                emit this->ZSigLog(false,QString(buffer));
            }
        }

        //Creates a new process data domain.
        //For process data exchange, at least one process data domain is needed.
        //This method creates a new process data domain and returns a pointer to the new domain object.
        //This object can be used for registering PDOs and exchanging them in cyclic operation.
        domainIn=ecrt_master_create_domain(master);
        domainOut=ecrt_master_create_domain(master);
        if(!domainIn || !domainOut)
        {
            emit this->ZSigLog(true,"failed to create domain Input/Output!");
            iThreadExitCode=-1;
            break;
        }
        emit this->ZSigLog(false,"create domain okay.");

        //Obtains a slave configuration.
        //If the slave with the given address is found during the bus configuration,
        //its vendor ID and product code are matched against the given value.
        //On mismatch, the slave is not configured and an error message is raised.
        if(!(sc_copley[0]=ecrt_master_slave_config(master,CopleySlavePos,Copley_VID_PID)))
        {
            emit this->ZSigLog(true,"failed to get slave(0) configuration.");
            iThreadExitCode=-1;
            break;
        }
        if(!(sc_copley[1]=ecrt_master_slave_config(master,CopleySlavePos2,Copley_VID_PID)))
        {
            emit this->ZSigLog(true,"failed to get slave(1) configuration.");
            iThreadExitCode=-1;
            break;
        }
        emit this->ZSigLog(false,"get 2 slaves configuration okay!");

        //Specify a complete PDO configuration.
        //This function is a convenience wrapper for the functions
        //ecrt_slave_config_sync_manager(), ecrt_slave_config_pdo_assign_clear(),
        //ecrt_slave_config_pdo_assign_add(), ecrt_slave_config_pdo_mapping_clear()
        //and ecrt_slave_config_pdo_mapping_add(), that are better suitable for
        //automatic code generation.
        emit this->ZSigLog(false,"configuring PDOs...");
        if(ecrt_slave_config_pdos(sc_copley[0],EC_END,slave0_syncs))
        {
            emit this->ZSigLog(true,"failed to configure slave(0) PDOs.");
            iThreadExitCode=-1;
            break;
        }
        if(ecrt_slave_config_pdos(sc_copley[1],EC_END,slave1_syncs))
        {
            emit this->ZSigLog(true,"failed to configure slave(1) PDOs.");
            iThreadExitCode=-1;
            break;
        }
        emit this->ZSigLog(false,"configuring PDOs done.");

        if(ecrt_domain_reg_pdo_entry_list(domainIn,domainIn_regs))
        {
            emit this->ZSigLog(true,"failed to register domainIn.");
            iThreadExitCode=-1;
            break;
        }
        if(ecrt_domain_reg_pdo_entry_list(domainOut,domainOut_regs))
        {
            emit this->ZSigLog(true,"failed to register domainWr.");
            iThreadExitCode=-1;
            break;
        }
        emit this->ZSigLog(false,"PDO(s) entry registration done.");

        //Finishes the configuration phase and prepares for cyclic operation.
        //This function tells the master that the configuration phase is finished and the realtime operation will begin.
        //The function allocates internal memory for the domains and calculates the logical FMMU addresses for domain members.
        //It tells the master state machine that the bus configuration is now to be applied.
        if(ecrt_master_activate(master))
        {
            emit this->ZSigLog(false,"failed to activate master!");
            iThreadExitCode=-1;
            break;
        }
        emit this->ZSigLog(false,"master activated.");

        //get the address of mapped domains.
        if(!(domainIn_pd=ecrt_domain_data(domainIn)))
        {
            emit this->ZSigLog(false,"get in domain address error!");
            iThreadExitCode=-1;
            break;
        }
        if(!(domainOut_pd=ecrt_domain_data(domainOut)))
        {
            emit this->ZSigLog(false,"get out domain address error!");
            iThreadExitCode=-1;
            break;
        }

        g_SysFSM=FSM_SafeOp;

        while(!gGblPara.m_bExitFlag)
        {
            if(1/*gGblPara.m_iSlavesEnBitMask!=0*/)
            {
                //1000us=1ms.
                //usleep(1000000/TASK_FREQUENCY);
                //if the time is less, the motor has no time to run.
                //so we set the time longer to wait for the motor executed the previous command.
                usleep(1000);
                //usleep(8000);
                //usleep(10000);
                //usleep(20000);
                this->ZDoCyclicTask();
            }else{
                usleep(1000*1000);
            }
        }
        ecrt_release_master(master);
        master=NULL;
        munlockall();
    }while(0);
    if(iThreadExitCode<0)
    {
        emit this->ZSigLog(true,QString("EtherCAT thread exit with %1.").arg(iThreadExitCode));
    }else{
        emit this->ZSigLog(false,QString("EtherCAT thread exit with %1.").arg(iThreadExitCode));
    }
    return;
}
