#ifndef APP_TASK
#define APP_TASK
#include <stdint.h>

#define SYSTEM_LED_RUN					0X01
#define SYSTEM_LED_NETOK				0X02
#define SYSTEM_LED_GPSOK				0X04	//普通GPS

#define GPSLED1							0

#define GPS_REQUEST_UPLOAD_ONE			0X00000001
#define GPS_REQUEST_ACC_CTL				0X00000002
#define GPS_REQUEST_GPSKEEPOPEN_CTL		0X00000004
#define GPS_REQUEST_BLE					0X00000008
#define GPS_REQUEST_DEBUG				0X00000010

#define GPS_REQUEST_ALL					0xFFFFFFFF


#define ALARM_LIGHT_REQUEST				0x0001 //感光
#define ALARM_LOSTV_REQUEST				0x0002 //断电
#define ALARM_LOWV_REQUEST				0x0004 //低电
#define ALARM_SHUTTLE_REQUEST			0x0008 //震动报警
#define ALARM_ACCLERATE_REQUEST			0X0010
#define ALARM_DECELERATE_REQUEST		0X0020
#define ALARM_RAPIDRIGHT_REQUEST		0X0040
#define ALARM_RAPIDLEFT_REQUEST			0X0080
#define ALARM_SOS_REQUEST				0X0100
#define ALARM_BLE_LOST_REQUEST          0x0200
#define ALARM_BLE_RESTORE_REQUEST       0x0400
#define ALARM_OIL_RESTORE_REQUEST       0x0800
#define ALARM_BLE_ERR_REQUEST           0x1000
#define ALARM_OIL_CUTDOWN_REQUEST       0x2000
#define ALARM_SHIELD_REQUEST            0x4000
#define ALARM_PREWARN_REQUEST           0x8000


#define ACCURACY_INIT_STATUS			0
#define ACCURACY_INITWAIT_STATUS		1
#define ACCURACY_RUNNING_STATUS			2

#define ACCURACY_INIT_OK				0
#define ACCURACY_INIT_SDK_ERROR			1
#define ACCURACT_INIT_NET_ERROR			2
#define ACCURACT_INIT_NONE				3

#define APP_TASK_KERNAL_EVENT		    0x0001
#define APP_TASK_POLLUART_EVENT			0x0002

#define UART_RECV_BUFF_SIZE 			512
#define DEBUG_BUFF_SIZE					256


#define MODE_START						0
#define MODE_RUNING						1
#define MODE_STOP						2
#define MODE_DONE						3

//GPS_UPLOAD_GAP_MAX 以下，gps常开，以上(包含GPS_UPLOAD_GAP_MAX),周期开启
#define GPS_UPLOAD_GAP_MAX				60

#define ACC_READ		0
#define ACC_STATE_ON	1
#define ACC_STATE_OFF	0


#define DEV_EXTEND_OF_MY	0x01
#define DEV_EXTEND_OF_BLE	0x02

typedef struct
{
    uint32_t sys_tick;		//记录系统运行时间
  	uint8_t sysLedState;
    
    uint8_t	sys_led1_onoff;
	
    uint8_t sys_led1_on_time;
    uint8_t sys_led1_off_time;
	
	
} SystemLEDInfo;

typedef enum
{
    TERMINAL_WARNNING_NORMAL = 0, /*    0 		*/
    TERMINAL_WARNNING_SHUTTLE,    /*    1：震动报警       */
    TERMINAL_WARNNING_LOSTV,      /*    2：断电报警       */
    TERMINAL_WARNNING_LOWV,       /*    3：低电报警       */
    TERMINAL_WARNNING_SOS,        /*    4：SOS求救      */
    TERMINAL_WARNNING_CARDOOR,        /*    5：车门求救      */
    TERMINAL_WARNNING_SWITCH,        /*    6：开关      */
    TERMINAL_WARNNING_LIGHT,      /*    7：感光报警       */
} TERMINAL_WARNNING_TYPE;

typedef enum{
	GPSCLOSESTATUS,
	GPSWATISTATUS,
	GPSOPENSTATUS,

}GPSREQUESTFSMSTATUS;

typedef struct
{
	uint8_t initok;
	uint8_t fsmStep;
	uint8_t tick;
}AccuracyStruct;

typedef enum
{
    ACC_SRC,
    VOLTAGE_SRC,
    GSENSOR_SRC,
    SYS_SRC,
} motion_src_e;
typedef enum
{
    MOTION_STATIC = 0,
    MOTION_MOVING,
} motionState_e;


typedef struct
{
    uint8_t ind;
    motionState_e motionState;
    uint8_t tapInterrupt;
    uint8_t tapCnt[50];
} motionInfo_s;

void terminalDefense(void);
void terminalDisarm(void);
uint8_t getTerminalAccState(void);
void terminalAccon(void);
void terminalAccoff(void);
void terminalCharge(void);
void terminalunCharge(void);
uint8_t getTerminalChargeState(void);
void terminalGPSFixed(void);
void terminalGPSUnFixed(void);

void ledStatusUpdate(uint8_t status, uint8_t onoff);

void gpsRequestSet(uint32_t flag);
void gpsRequestClear(uint32_t flag);
uint32_t gpsRequestGet(uint32_t flag);
uint8_t gpsRequestOtherGet(uint32_t flag);
void gpsTcpSendRequest(void);
void saveHistoryGps(void);



void wakeUpByInt(uint8_t     type,uint8_t sec);


void alarmRequestSet(uint16_t request);
void alarmRequestClear(uint16_t request);

void lbsRequestSet(uint8_t ext);
void wifiRequestSet(uint8_t ext);
uint8_t motionGetSize(void);
void motionOccur(void);

void modeTryToStop(void);

void sosRequestSet(void);


void relayAutoRequest(void);
void relayAutoClear(void);

void myTaskPreInit(void);
void myTaskInit(void);
#endif
