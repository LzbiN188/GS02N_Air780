/*
 * app_central.h
 *
 *  Created on: Jun 27, 2022
 *      Author: idea
 */

#ifndef APP_INCLUDE_APP_CENTRAL_H_
#define APP_INCLUDE_APP_CENTRAL_H_

#include "config.h"

#define APP_START_EVENT             				0x0001
#define APP_DISCOVER_SERVICES_EVENT                 0x0002
#define APP_DISCOVER_CHAR_EVENT						0x0004
#define APP_DISCOVER_NOTIFY_EVENT					0x0008
#define APP_NOTIFY_ENABLE_EVENT						0x0010
#define APP_WRITEDATA_EVENT							0x1000
#define APP_LINK_TIMEOUT_EVENT                      0x0020

#define SCAN_LIST_SIZE              25

#define BLE_SCAN_INTERVAL           180



typedef struct
{
    uint8_t devAddr[B_ADDR_LEN]; //!< Device address of link
    uint8_t devAddrType;         //!< Device address type: @ref GAP_ADDR_TYPE_DEFINES
    uint8_t connRole;            //!< Connection formed as Master or Slave
    uint8_t clockAccuracy;       //!< Clock Accuracy
    uint8_t findIt;
    uint16_t connInterval;       //!< Connection Interval
    uint16_t connLatency;        //!< Connection Latency
    uint16_t connTimeout;        //!< Connection Timeout
    uint16_t connectionHandle;   //!< Connection Handle from controller used to ref the device
    uint16_t findUUID;
	uint16_t findStart;
	uint16_t findEnd;
	uint16_t notifyHandle;
	uint16_t writeHandle;

} centralConnInfo_s;

typedef struct
{
    uint8 addr[6];
    uint8 broadcaseName[31];
    uint8 request;
    uint8 elec;
    uint8 userId;
    uint8 type;
    uint8 version;
    uint8 lockStatus;
    uint8 scanCnt;
    uint8 lost;
    uint8 scanMinute;
    int8  rssi;
    uint8 addrtype;
    uint8 chainStatus;

}scanDevInfo_s;

typedef struct
{
    scanDevInfo_s devlist[SCAN_LIST_SIZE];
    uint8_t devcnt;
    uint8_t targetDev[6];
}scanList_s;


extern  tmosTaskID appCentralTaskId;

void appCentralInit(void);
void appCentralStartScan(void);
void appCentralStopScan(void);
void appCentralEstablish(uint8_t * mac);
void appCentralTeiminate(void);
uint8 appCentralWriteData(uint16 handle, uint8 *data, uint8 len);

void appShowScanList(char *message);
void appUpdateScanList(void);
void centralScanRequestSet(void);
void centralScanRequestClear(void);
int appDevListCheck(void);
void appCentralScanTask(void);

scanList_s *getDevScanlist(void);

uint8_t appCentralSendData(uint8_t *data, uint8_t len);

#endif /* APP_INCLUDE_APP_CENTRAL_H_ */
