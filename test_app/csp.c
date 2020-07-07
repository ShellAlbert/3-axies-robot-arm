//zhangshaoyan 13522296239.
//TechServo motor control.
//gcc csp.c  -I/opt/ethercat/include -lethercat
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

#define TASK_FREQUENCY       1000  /* Hz */
#define TIMEOUT_CLEAR_ERROR  (2*TASK_FREQUENCY)
#define TARGET_POSITION      0

//Status Word(0x6041,uint16)
//bit11:Internal Limit Active.This bit is set when one of the amplifier limits active.(current,voltage,velocity or position).
//bit9:Set when the amplifier is being controlled by the CANopen interface.
//bit7:Warning. set if a warning condition is present on the amplifier.
//bi6:Switch on disabled.
//bit5:Quick Stop.when clear,the amplifier is performing a quick stop.
//bit4:Voltage enabled.
//bit3:Fault.If set,a latched fault condition is present in the amplifier.
//bit2:Operation enabled. Set when the amplifier is enabled.
//bit1:Switched On.
//bit0:Ready to switch on.
//15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0
//0 , 0, 0, 0,1, 0, 1,0,1,1,1,1,1,1,1,1 (0x0AFF)
#define STW_MASK       0xAFF  /*mask to remove manufacturer special bits and target_reached*/
//Switch On Disabled.
//15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0
//0, 0, 0, 0, 0, 0, 1,0,0,1,x,1,0,0,0,0 (0x250/270)
#define STW_SWON_DIS  0x250  /*switched on disabled*/
//Ready to Switch On
//15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0
//0, 0, 0, 0, 0, 0, 1,0,0,0,1,1,0,0,0,1 (0x231)
#define STW_RDY_SWON  0x231  /*ready to switch on*/
//Switch On.
//15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0
//0, 0, 0, 0, 0, 0, 1,0,0,0,1,1,0,0,1,1 (0x233)
#define STW_SWON  0x233  /*switched on*/
//Fault.
//15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0
//0, 0, 0, 0, 0, 0, 1,0,0,0,1,x,1,0,0,0 (0x228)
#define STW_FAULT      0x228  /*Fault*/

//Control Word(0x6040,uint16).
//bit2:Quick Stop. If this bit is clear,then the amplifier is commandded to perform a quick stop.
//bit1:Enable Voltage.This bit must be set to enable the amplifier.
#define CW_QSTOP  0x00   /*enable quick stop*/
//0x06:0000,0110.
#define CW_DIS_QSTOP  0x06   /*disable quick stop*/
//0x07:0000,0111.
#define CW_EN_SWON  0x07   /*enable switched on*/
//0x0F:0000,1111.
#define CW_EN_OP     0x0F   /*enable operation*/
//bit7:Reset Fault.A low-to-high transition of this bit makes the amplifier attempt to clear any latched fault condition.
//0x80:1000,0000.
#define CW_CLR_FAULT  0x80   /*clear error*/

/* EtherCAT */
static ec_master_t *master = NULL;
static ec_domain_t *domain1 = NULL;
static uint8_t *domain1_pd = NULL; /* process data */
static ec_slave_config_t *sc_copley = NULL; /* Copley slave configuration */

#define CopleySlavePos    0,0  /* EtherCAT address on the bus */
//vendor_id:(0x1018,1)
//product_id:(0x1018,2)
#define Copley_VID_PID  0x000000ab,0x00001030 /* vendor_id, product_id */

/* PDO entries offsets */
static struct {
    unsigned int ctrl_word;
    unsigned int target_position;
    unsigned int status_word;
    unsigned int act_velocity;
    unsigned int act_position;
} offset;

/** List record type for PDO entry mass-registration.
 *
 * This type is used for the array parameter of the
 * ecrt_domain_reg_pdo_entry_list()
 */
//typedef struct {
//    uint16_t alias; /**< Slave alias address. */
//    uint16_t position; /**< Slave position. */
//    uint32_t vendor_id; /**< Slave vendor ID. */
//    uint32_t product_code; /**< Slave product code. */
//    uint16_t index; /**< PDO entry index. */
//    uint8_t subindex; /**< PDO entry subindex. */
//    unsigned int *offset; /**< Pointer to a variable to store the PDO entry's
//                       (byte-)offset in the process data. */
//    unsigned int *bit_position; /**< Pointer to a variable to store a bit
//                                  position (0-7) within the \a offset. Can be
//                                  NULL, in which case an error is raised if the
//                                  PDO entry does not byte-align. */
//} ec_pdo_entry_reg_t;

const static ec_pdo_entry_reg_t domain1_regs[] = {
    { CopleySlavePos, Copley_VID_PID, 0x6040, 0, &offset.ctrl_word },
    { CopleySlavePos, Copley_VID_PID, 0x607A, 0, &offset.target_position },
    { CopleySlavePos, Copley_VID_PID, 0x6041, 0, &offset.status_word },
    { CopleySlavePos, Copley_VID_PID, 0x606C, 0, &offset.act_velocity },
    { CopleySlavePos, Copley_VID_PID, 0x6063, 0, &offset.act_position},
    {}
};

/** PDO entry configuration information.
 *
 * This is the data type of the \a entries field in ec_pdo_info_t.
 *
 * \see ecrt_slave_config_pdos().
 */
//typedef struct {
//    uint16_t index; /**< PDO entry index. */
//    uint8_t subindex; /**< PDO entry subindex. */
//    uint8_t bit_length; /**< Size of the PDO entry in bit. */
//} ec_pdo_entry_info_t;

ec_pdo_entry_info_t copley_pdo_entries[] = {
    /* RxPdo 0x1600 */
    { 0x6040, 0x00, 16 }, /*Control Word,uint16*/
    { 0x607A, 0x00, 32 }, /*Profile Target Position,uint32*/

    /* TxPDO 0x1a00 */
    { 0x6041, 0x00, 16 }, /*Status Word,uint16*/

    /* the maximum length for PDO is 8 bytes, so create another PDO
               for actual velocity and position */

    /* TxPDO 0x1a01 */
    { 0x606C, 0x00, 32 }, /* actual velocity, in rpm */
    { 0x6063, 0x00, 32 }, /* actual position */
};
//0x1600(Receive PDO 1 mapping parameter).
//(0x1600,0)=2,Number of mapped object RxPDO 1.
//(0x1600,1)=copley_pdo_entries[0]=(0x6040,0x00,16)=0x60400010.
//(0x1600,2)=copley_pdo_entries[1]=(0x607a,0x00,32)=0x607a0020.

//0x1a00(Transmit PDO 1 mapping parameter).
//(0x1a00,0)=1,Number of mapped object TxPDO 1.
//(0x1a00,1)=copley_pdo_entries[2]=(0x6041,0x00,16)=0x60410010.

//0x1a01(Transmit PDO 2 mapping parameter).
//(0x1a01,0)=2,Number of mapped object TxPDO 2.
//(0x1a01,1)=copley_pdo_entries[3]=(0x606c,0x00,32)=0x606c0020.
//(0x1a01,2)=copley_pdo_entries[4]=(0x6063,0x00,32)=0x60630020.
ec_pdo_info_t copley_pdos[] = {
    { 0x1600, 2, copley_pdo_entries + 0 },
    { 0x1a00, 1, copley_pdo_entries + 2 },
    { 0x1a01, 2, copley_pdo_entries + 3 },
};

/** Sync manager configuration information.
 *
 * This can be use to configure multiple sync managers including the PDO
 * assignment and PDO mapping. It is used as an input parameter type in
 * ecrt_slave_config_pdos().
 */
//typedef struct {
//    uint8_t index; /**< Sync manager index. Must be less
//                     than #EC_MAX_SYNC_MANAGERS for a valid sync manager,
//                     but can also be \a 0xff to mark the end of the list. */
//    ec_direction_t dir; /**< Sync manager direction. */
//    unsigned int n_pdos; /**< Number of PDOs in \a pdos. */
//    ec_pdo_info_t *pdos; /**< Array with PDOs to assign. This must contain
//                            at least \a n_pdos PDOs. */
//    ec_watchdog_mode_t watchdog_mode; /**< Watchdog mode. */
//} ec_sync_info_t;

ec_sync_info_t copley_syncs[] = {
    //SM0:MBoxOut.
    { 0, EC_DIR_OUTPUT, 0, NULL, EC_WD_DISABLE },
    //SM1:MBoxIn.
    { 1, EC_DIR_INPUT, 0, NULL, EC_WD_DISABLE },
    //SM2:Outputs -> copley_pdos[0] -> (0x1600,2)(RxPDO2) -> (0x6040,0x607A).
    { 2, EC_DIR_OUTPUT, 1, copley_pdos + 0, EC_WD_DISABLE },
    //SM3:Inputs -> copley_pdos[1]/copley_pdos[2] ->(0x1a00,1)/(0x1a01,2)(TxPDO2) -> (0x6041,0x606C,0x6063).
    { 3, EC_DIR_INPUT, 2, copley_pdos + 1, EC_WD_DISABLE },
    { 0xFF }
};

/****************************************************************************/
void cyclic_task()
{
    static unsigned int timeout_error = 0;
    static uint16_t command = CW_QSTOP;
    static int32_t target_position = /*TARGET_POSITION*/123456;

    uint16_t status;    /* DS402 status register, without manufacturer bits */
    float act_velocity; /* actual velocity in rpm */
    int act_position; /* actual position in encoder unit */

    /* receive process data */
    ecrt_master_receive(master);
    ecrt_domain_process(domain1);

    /* read inputs */
    status = EC_READ_U16(domain1_pd + offset.status_word) & STW_MASK;
    act_velocity = EC_READ_S32(domain1_pd + offset.act_velocity)/1000.0;
    act_position = EC_READ_S32(domain1_pd + offset.act_position);

    /* DS402 CANopen over EtherCAT status machine */
    if (status == STW_SWON_DIS && command != CW_DIS_QSTOP) {
        printf( "Copley: disable quick stop\n" );
        command = CW_DIS_QSTOP;

    } else if (status == STW_RDY_SWON && command != CW_EN_SWON ) {
        printf("Copley: enable switch on\n" );
        command = CW_EN_SWON;

    } else if ( status == STW_SWON  && command != CW_EN_OP ) {
        printf("Copley: start operation\n" );
        command = CW_EN_OP;

    } else if ( status == STW_FAULT && command != CW_CLR_FAULT ) {
        if ( timeout_error )
        {
            if (timeout_error == TIMEOUT_CLEAR_ERROR) {
                printf( "Copley: ERROR, wait for timeout\n" );
            }
            timeout_error--;
        } else {
            timeout_error = TIMEOUT_CLEAR_ERROR;
            command = CW_CLR_FAULT;
            printf( "Copley: clear error now\n" );
        }
    } else {
        /* print actual values, once per second */
        static time_t prev_second = 0;
        time_t now = time(NULL);
        if ( now != prev_second )
        {
            printf("Status Word=0x%x\n",EC_READ_U16(domain1_pd + offset.status_word));
            printf("Copley:ActualVelocity=%.1f rpm,ActualPosition=%d,TargetPosition=%d\n",act_velocity, act_position, target_position );
            prev_second = now;
        }
    }

    /* write output */
    EC_WRITE_S32( domain1_pd + offset.target_position, target_position );
    EC_WRITE_U16( domain1_pd + offset.ctrl_word, command );

    /* send process data */
    ecrt_domain_queue(domain1);
    ecrt_master_send(master);
}

/****************************************************************************/

int main(int argc, char **argv)
{
    static const int enable_realtime = 1;

    if (enable_realtime) {
        struct sched_param param;
        param.sched_priority = 49;
        if ( sched_setscheduler( 0, SCHED_FIFO, &param ) == -1) {
            perror("sched_setscheduler failed");
        }
        /* Lock memory */
        if( mlockall( MCL_CURRENT|MCL_FUTURE ) == -1) {
            perror("mlockall failed");
        }
    }

    //Requests an EtherCAT master for realtime operation.
    master = ecrt_request_master( 0 );
    if ( !master )
    {
        printf("error:failed to request master 0!\n");
        return -1;
    }
    printf("request master 0 okay.\n");

    //Creates a new process data domain.
    //For process data exchange, at least one process data domain is needed.
    //This method creates a new process data domain and returns a pointer to the
    //new domain object. This object can be used for registering PDOs and
    //exchanging them in cyclic operation.
    domain1 = ecrt_master_create_domain( master );
    if(!domain1)
    {
        printf("error:failed to create domain!\n");
        return -1;
    }
    printf("create domain okay.\n");

    //Obtains a slave configuration.
    //If the slave with the given address is found during the bus configuration,
    //its vendor ID and product code are matched against the given value.
    //On mismatch, the slave is not configured and an error message is raised.
    if(!(sc_copley=ecrt_master_slave_config( master, CopleySlavePos, Copley_VID_PID)))
    {
        printf("error:failed to get slave configuration for Copley.\n");
        return -1;
    }
    printf("get slave configuration okay!\n");

    /* Configure Copley flexible PDO */
    printf("Configuring Copley with flexible PDO...\n");
    /* Clear RxPdo */
    ecrt_slave_config_sdo8( sc_copley, 0x1C12, 0, 0 ); /* clear sm pdo 0x1c12 */
    ecrt_slave_config_sdo8( sc_copley, 0x1600, 0, 0 ); /* clear RxPdo 0x1600 */
    ecrt_slave_config_sdo8( sc_copley, 0x1601, 0, 0 ); /* clear RxPdo 0x1601 */
    ecrt_slave_config_sdo8( sc_copley, 0x1602, 0, 0 ); /* clear RxPdo 0x1602 */
    ecrt_slave_config_sdo8( sc_copley, 0x1603, 0, 0 ); /* clear RxPdo 0x1603 */

    /* Define RxPdo */
    ecrt_slave_config_sdo32( sc_copley, 0x1600, 1, 0x60400010 ); /* 0x6040:0/16bits, control word */
    ecrt_slave_config_sdo32( sc_copley, 0x1600, 2, 0x607A0020 ); /* 0x60C1:1/32bits target position*/
    ecrt_slave_config_sdo8( sc_copley, 0x1600, 0, 2 ); /* set number of PDO entries for 0x1600 */

    ecrt_slave_config_sdo16( sc_copley, 0x1C12, 1, 0x1600 ); /* list all RxPdo in 0x1C12:1-4 */
    ecrt_slave_config_sdo8( sc_copley, 0x1C12, 0, 1 ); /* set number of RxPDO */

    /* Clear TxPdo */
    ecrt_slave_config_sdo8( sc_copley, 0x1C13, 0, 0 ); /* clear sm pdo 0x1c13 */
    ecrt_slave_config_sdo8( sc_copley, 0x1A00, 0, 0 ); /* clear TxPdo 0x1A00 */
    ecrt_slave_config_sdo8( sc_copley, 0x1A01, 0, 0 ); /* clear TxPdo 0x1A01 */
    ecrt_slave_config_sdo8( sc_copley, 0x1A02, 0, 0 ); /* clear TxPdo 0x1A02 */
    ecrt_slave_config_sdo8( sc_copley, 0x1A03, 0, 0 ); /* clear TxPdo 0x1A03 */

    /* Define TxPdo */
    ecrt_slave_config_sdo32( sc_copley, 0x1A00, 1, 0x60410010 ); /* 0x6041:0/16bits, status word */
    ecrt_slave_config_sdo8( sc_copley, 0x1A00, 0, 1 ); /* set number of PDO entries for 0x1A00 */

    //Non-fixed TxPdo.
    //0x1A01:Transmit PDO 2,Sub index:1~8.
    //(0x606c,0):Actual Velocity,int32.
    //(0x6063,0):Position Actual Value,int32.
    //here we map (0x606c,0) to (0x1A01,1),map (0x6063,0) to (0x1A01,2).
    //So the data(0x606c,0x6063) will be gathered by TxPdo2.
    //All TxPdo data(includes TxPdo2) will be tx out by SyncManager2.
    ecrt_slave_config_sdo32( sc_copley, 0x1A01, 1, 0x606C0020 );  /* 0x606c:0/32bits, act velocity */
    ecrt_slave_config_sdo32( sc_copley, 0x1A01, 2, 0x60630020 );  /* 0x6063:0/32bits, act position */
    ecrt_slave_config_sdo8( sc_copley, 0x1A01, 0, 2 ); /* set number of PDO entries for 0x1A01 */

    //Sync Manager 2 PDO Assignment Object(0x1c12~0x1c13).
    //This object is used to assign a sync manager to PDOs.
    //here we use Sm2 to manage 0x1A00/0x1A01 automatically.
    ecrt_slave_config_sdo16( sc_copley, 0x1C13, 1, 0x1A00 ); /* list all TxPdo in 0x1C13:1-4 */
    ecrt_slave_config_sdo16( sc_copley, 0x1C13, 2, 0x1A01 ); /* list all TxPdo in 0x1C13:1-4 */
    ecrt_slave_config_sdo8( sc_copley, 0x1C13, 0, 2 ); /* set number of TxPDO */


    //Mode of Operation.
    //(0x6060,0)=8 CSP:Cyclic Synchronous Position mode
    ecrt_slave_config_sdo8(sc_copley,0x6060,0,8);

    //Specify a complete PDO configuration.
    //This function is a convenience wrapper for the functions
    //ecrt_slave_config_sync_manager(), ecrt_slave_config_pdo_assign_clear(),
    //ecrt_slave_config_pdo_assign_add(), ecrt_slave_config_pdo_mapping_clear()
    //and ecrt_slave_config_pdo_mapping_add(), that are better suitable for
    //automatic code generation.
    printf("Configuring PDOs...\n");
    if ( ecrt_slave_config_pdos( sc_copley, EC_END, copley_syncs ) )
    {
        printf("error:failed to configure Copley PDOs.\n" );
        return -1;
    }
    printf("configure PDOs done.\n");

    if ( ecrt_domain_reg_pdo_entry_list( domain1, domain1_regs ) )
    {
        printf("error:PDO entry registration failed!\n" );
        return -1;
    }
    printf("PDO entry register done.\n");


    //Finishes the configuration phase and prepares for cyclic operation.
    //This function tells the master that the configuration phase is finished and
    //the realtime operation will begin. The function allocates internal memory
    //for the domains and calculates the logical FMMU addresses for domain
    //members. It tells the master state machine that the bus configuration is
    //now to be applied.
    printf("Activating master...\n");
    if ( ecrt_master_activate( master ) )
    {
        printf("error:failed to activate master!\n");
        return -1;
    }
    printf("master activated.\n");

    // Returns the domain's process data.
    if(!(domain1_pd = ecrt_domain_data(domain1)))
    {
        printf("error:domain data error!\n");
        return -1;
    }

    printf("Started.\n");
    while (1) {
        usleep( 1000000 / TASK_FREQUENCY );
        cyclic_task();
    }

    printf("Copley slave test end.\n");
    return 0;
}

