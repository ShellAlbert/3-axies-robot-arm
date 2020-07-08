#include "zethercatthread.h"
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
    FSM_Op,
    FSM_Link_Down,
    FSM_Idle_Status,
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

int ecstate=0;

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
    { CopleySlavePos, Copley_VID_PID, 0x6063, 0, &actPosition[0], NULL },

    { CopleySlavePos2, Copley_VID_PID, 0x6041, 0, &statusWord[1], NULL },
    { CopleySlavePos2, Copley_VID_PID, 0x606C, 0, &actVelocity[1], NULL },
    { CopleySlavePos2, Copley_VID_PID, 0x6063, 0, &actPosition[1], NULL },
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

    //500ms.
    if(!(cycle_counter%100))
    {
        check_master_state();
        check_slave_config_state();
    }

    switch(g_SysFSM)
    {
    case FSM_SafeOp:
        //check if master is in OP mode,if not then turn to OP mode.
        check_master_state();
        check_slave_config_state();
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
            //make sure all slaves are in OP mode.
            if((sc_copley_state[0].al_state!=0x08) || (sc_copley_state[1].al_state!=0x08))
            {
                //do not change FSM,wait for next time checking.
            }else{
                //master in OP mode and all slaves are in OP mode.
                //change to next FSM.
                ecstate=0;
                g_SysFSM=FSM_Op;
                printf("FSM -> FSM_Op\n");
            }
        }
        break;
    case FSM_Op:
        ecstate++;
        if(ecstate<=16)
        {
            switch(ecstate)
            {
            case 1:
                //bit7=1.
                //Reset Fault.
                //A low-to-high transition of this bit makes the amplifier attempt to clear any latched fault condition.
                EC_WRITE_U16(domainOutput_pd+ctrlWord[0],0x80);
                EC_WRITE_U16(domainOutput_pd+ctrlWord[1],0x80);
                break;
            case 7:
                //write current position to target position
                gGblPara.m_i00ActPos=EC_READ_S32(domainInput_pd+actPosition[0]);
                EC_WRITE_S32(domainOutput_pd+targetPosition[0],EC_READ_S32(domainInput_pd+actPosition[0]));
                printf("slave 0: current position:%d\n",gGblPara.m_i00ActPos);

                gGblPara.m_i01ActPos=EC_READ_S32(domainInput_pd+actPosition[1]);
                EC_WRITE_S32(domainOutput_pd+targetPosition[1],EC_READ_S32(domainInput_pd+actPosition[1]));
                printf("slave 1: current position:%d\n",gGblPara.m_i01ActPos);
                break;
            case 9:
                //0x06=0000,0110.
                EC_WRITE_U16(domainOutput_pd+ctrlWord[0],0x06);
                EC_WRITE_U16(domainOutput_pd+ctrlWord[1],0x06);
                break;
            case 11:
                //0x07=0000,0111.
                EC_WRITE_U16(domainOutput_pd+ctrlWord[0],0x07);
                EC_WRITE_U16(domainOutput_pd+ctrlWord[1],0x07);
                break;
            case 13:
                //0x0F=0000,1111.
                EC_WRITE_U16(domainOutput_pd+ctrlWord[0],0x0F);
                EC_WRITE_U16(domainOutput_pd+ctrlWord[1],0x0F);
                break;
            default:
                break;
            }
        }else{
            //Status Word(0x6041)
            //0x0004:0000,0000,0000,0100
            //bit2: Operation Enabled.Set when the amplifier is enabled.
            uint16_t s0,s1;
            s0=EC_READ_U16(domainInput_pd+statusWord[0]);
            s1=EC_READ_U16(domainInput_pd+statusWord[1]);
            if( ((s0&0x0004)==0) || ((s1&0x0004)==0) )
            {
                //at least one amplifier is not enabled,
                //so we donot change FSM,checking next time.
                printf("At least one amplifier is not enabled!\n");
                printf("0x%x,0x%x\n",s0,s1);
//                ecstate=0;
//                g_SysFSM=FSM_SafeOp;
            }else{
                //all slaves are "Operation Enabled".
                ecstate=0;
                g_SysFSM=FSM_Idle_Status;
                printf("FSM -> FSM_Idle_Status\n");
            }
        }
        break;
    default:
    {
        if(!(cycle_counter%100))
        {
            uint16_t status;
            float act_velocity;
            int act_position;

            status = EC_READ_U16(domainInput_pd + statusWord[0]);
            act_velocity = EC_READ_S32(domainInput_pd + actVelocity[0])/1000.0;
            act_position = EC_READ_S32(domainInput_pd + actPosition[0]);
            printf("\n\n0:aclVel=%.1f rpm,aclPos=%d,",act_velocity, act_position);
            printf("Status Word=0x%x\n",status);
            print_bits_splitter(status);
            gStatusWord=status;
            gActVelocity=act_velocity;
            gActPosition=act_position;
            gTarPosition=0;

            status = EC_READ_U16(domainInput_pd + statusWord[1]);
            act_velocity = EC_READ_S32(domainInput_pd + actVelocity[1])/1000.0;
            act_position = EC_READ_S32(domainInput_pd + actPosition[1]);
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
        gGblPara.m_i00ActPos-=100;
        EC_WRITE_S32(domainOutput_pd+targetPosition[0],gGblPara.m_i00ActPos);
        gGblPara.m_i01ActPos-=100;
        EC_WRITE_S32(domainOutput_pd+targetPosition[1],gGblPara.m_i01ActPos);

//        ecstate=0;
//        g_SysFSM=FSM_SafeOp;
    }
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
    //Mode of Operation.
    //(0x6060,0)=8 CSP:Cyclic Synchronous Position mode
    ecrt_slave_config_sdo8(sc_copley[0],0x6060,0,8);
    ecrt_slave_config_sdo8(sc_copley[0],0x60c2,1,1);

    ecrt_slave_config_sdo8(sc_copley[1],0x6060,0,8);
    ecrt_slave_config_sdo8(sc_copley[1],0x60c2,1,1);

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
