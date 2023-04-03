#ifndef APP_SYS
#define APP_SYS

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define DEBUG_NONE				0
#define DEBUG_LOW				1
#define DEBUG_NET				2
#define DEBUG_GPS				3
#define DEBUG_FACTORY			4
#define DEBUG_BLE				5
#define DEBUG_ALL				9

#define MODE1									1
#define MODE2									2
#define MODE3									3
#define MODE21								4  //模式2的扩展，类似模式1
#define MODE23								5  //模式2的扩展，类似模式3

#define ITEMCNTMAX	8
#define ITEMSIZEMAX	60

#define ACCDETMODE0			0		//仅使用acc线
#define ACCDETMODE1			1		//acc线优先，其次电压
#define ACCDETMODE2			2		//acc线优先，其他gsensor
#define ACCDETMODE3         3       //acc线优先，其次电压+gsensor

#define NORMAL_LINK				0
#define BLE_LINK				1
#define JT808_LINK				2
#define HIDDEN_LINK				3
#define AGPS_LINK				4

typedef struct
{
    unsigned char item_cnt;
    char item_data[ITEMCNTMAX][ITEMSIZEMAX];
} ITEM;

typedef struct
{
    uint8_t nmeaOutPutCtl   		: 1;
    uint8_t lbsRequest				: 1;
    uint8_t wifiRequest				: 1;
    uint8_t rtcUpdate				: 1;
    uint8_t flag123					: 1;
    uint8_t gsensorOnoff			: 1;
    uint8_t gsensorErr				: 1;
    uint8_t hiddenServCloseReq		: 1;
    uint8_t agpsRequest				: 1;
    uint8_t doRelayFlag             : 1;
	uint8_t doSosFlag				: 1;
	uint8_t moduleRstFlag			: 1;
	uint8_t debugflag				: 1;
	uint8_t bleConnStatus			: 1;
    uint8_t lbsExtendEvt;
    uint8_t wifiExtendEvt;
    uint8_t ringWakeUpTick;
    uint8_t cmdTick;
    uint8_t runFsm;
    uint8_t gpsFsm;
    uint8_t gpsOnoff;
    uint8_t gsensorTapCnt;
    uint8_t terminalStatus;
    uint8_t sysLedState;
    uint8_t sysTaskId;
    uint8_t tapRecord[15];
    uint8_t logLevel;
    uint8_t noNmeaCnt;

    uint8_t alarmDate;
    uint8_t alarmHour;
    uint8_t alarmMinute;

    uint8_t taskId;

    uint16_t gpsuploadonepositiontime;
    uint16_t alarmRequest;

    uint32_t gpsRequest;	  /*GPS 开关请求*/
    uint32_t sysTick;    /*系统节拍*/
    uint32_t runStartTick;  /*开机节拍*/
    uint32_t gpsUpdatetick;

    float outsidevoltage;
	float insidevoltage;
    float lowvoltage;

	uint8_t moduleRstCnt;
} SystemInfoTypedef;

extern SystemInfoTypedef sysinfo;

uint16_t GetCrc16(const char *pData, int nLength);

void LogMessage(uint8_t level, char *debug);
void LogMessageWL(uint8_t level, char *buf, uint16_t len);
void LogPrintf(uint8_t level, const char *debug, ...);
void Log(uint8_t level, const char *debug, ...);
void LogWL(uint8_t level, uint8 *buf, uint16_t len);

uint8_t mycmdPatch(uint8_t *cmd1, uint8_t *cmd2);
int getCharIndex(uint8_t *src, int src_len, char ch);
int my_strpach(char *str1, const char *str2);
int my_getstrindex(char *str1, const char *str2, int len);
int my_strstr(char *str1, const char *str2, int len);
int distinguishOK(char *buf);
int16_t getCharIndexWithNum(uint8_t *src, uint16_t src_len, uint8_t ch, uint8_t num);
void byteToHexString(uint8_t *src, uint8_t *dest, uint16_t srclen);
int16_t changeHexStringToByteArray(uint8_t *dest, uint8_t *src, uint16_t size);
int16_t changeHexStringToByteArray_10in(uint8_t *dest, uint8_t *src, uint16_t size);
void stringToItem(ITEM *item, uint8_t *str, uint16_t len);
void strToUppper(char *data, uint16_t len);

void updateRTCtimeRequest(void);
void byteArrayInvert(uint8 *data, uint8 dataLen);
void stringToLowwer(char *str, uint16_t strlen);

#endif
