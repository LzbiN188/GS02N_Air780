#ifndef APP_EEPROM_H
#define APP_EEPROM_H
#include <stdint.h>

#include "app_sys.h"


/*
 * DataFlash �ֲ�
 * [0~1)        Total:1KB   bootloader
 * [1~9)        Total:8KB   systemparam
 * [9~10)		Total:1KB	dynamicparam
 * [10~30)      Total:20KB  database
 * [30~31)		Total:1KB	databaseInfo
 * [31~32)      Total:1KB   blestack
 */

//ĿǰԤ��ǰ1K�ֽڸ�bootloader������app��0x400��ʼ
#define BOOT_USER_PARAM_ADDR    0x00
#define APP_USER_PARAM_ADDR     0x400  //ʵ����0x00070000+APP_USER_PARAM_ADDR
#define APP_DYNAMIC_PARAM_ADDR	0x2400 //ʵ����0x00070000+APP_DYNAMIC_PARAM_ADDR
#define APP_PARAM_FLAG          0x1A
#define BOOT_PARAM_FLAG         0xB0

#define EEPROM_VERSION									"GS02N_Air780_V1.2.4LP"


#define JT808_PROTOCOL_TYPE			8
#define ZT_PROTOCOL_TYPE			0



typedef struct
{
    uint8_t VERSION;        /*��ǰ�����汾��*/
    uint8_t updateStatus;
    uint8_t apn[50];
    uint8_t apnuser[50];
    uint8_t apnpassword[50];
    uint8_t SN[20];
    uint8_t updateServer[50];
    uint8_t codeVersion[50];
    uint16_t updatePort;
} bootParam_s;

/*�洢��EEPROM�е�����*/
typedef struct
{
    uint8_t VERSION;        /*��ǰ�����汾��*/
    uint8_t MODE;           /*ϵͳ����ģʽ*/
    uint8_t ldrEn;
    uint8_t gapDay;
    uint8_t heartbeatgap;
    uint8_t ledctrl;
    uint8_t accdetmode;
    uint8_t poitype;
    uint8_t accctlgnss;
    uint8_t apn[50];
    uint8_t apnuser[50];
    uint8_t apnpassword[50];
    uint8_t lowvoltage;
    uint8_t fence;
    
    uint8_t Server[50];
    uint8_t agpsServer[50];
    uint8_t agpsUser[50];
    uint8_t agpsPswd[50];
    uint8_t protocol;

    uint8_t hiddenServOnoff;
    uint8_t hiddenServer[50];
    uint8_t jt808Server[50];
    uint8_t bleServer[50];

    uint8_t bleAutoDisc;
    uint8_t bleRfThreshold;
    uint8_t blePreShieldVoltage;
    uint8_t blePreShieldDetCnt;
    uint8_t blePreShieldHoldTime;
    uint8_t relayCtl;


    uint8_t bleConnMac[2][6];
    uint8_t relaySpeed;
    uint8_t bf;
    int8_t utc;

    uint16_t gpsuploadgap;
    uint16_t AlarmTime[5];  /*ÿ��ʱ�䣬0XFFFF����ʾδ���壬��λ����*/
    uint16_t gapMinutes;    /*ģʽ��������ڣ���λ����*/
    
    
    uint16_t agpsPort;
    uint16_t jt808Port;
    uint16_t hiddenPort;
    uint16_t ServerPort;
    uint16_t bleServerPort;
    uint16_t bleOutThreshold;

    float  adccal;
    float  accOnVoltage;
    float accOffVoltage;
    float protectVoltage;

	

	uint8_t cm;
	uint8_t sosNum[3][20];
	uint8_t centerNum[20];
	uint8_t sosalm;

    uint8_t gsdettime;
    uint8_t gsValidCnt;
    uint8_t gsInvalidCnt;

    uint8_t bleen;
    uint8_t agpsen;
    uint8_t cm2;
} systemParam_s;

/*����EEPROM��Ķ�̬����*/
typedef struct
{
	uint16_t runTime;  /*ģʽ���Ĺ���ʱ�䣬��λ����*/
	uint16_t startUpCnt;
	uint8_t SN[20];
	
	uint8_t jt808isRegister;
	uint8_t jt808sn[7];
	uint8_t jt808AuthCode[50];
    uint8_t jt808AuthLen;

	uint16_t noNmeaRstCnt;
}dynamicParam_s;


typedef enum{
	ALARM_TYPE_NONE,
	ALARM_TYPE_GPRS,
	ALARM_TYPE_GPRS_SMS,
	ALARM_TYPE_GPRS_SMS_TEL,
}AlarmType_e;

extern systemParam_s sysparam;
extern bootParam_s bootparam;
extern dynamicParam_s dynamicParam;

void bootParamSaveAll(void);
void bootParamGetAll(void);

void paramSaveAll(void);
void paramGetAll(void);

void dynamicParamSaveAll(void);
void dynamicParamGetAll(void);

void paramInit(void);
void paramDefaultInit(uint8_t type);


#endif
