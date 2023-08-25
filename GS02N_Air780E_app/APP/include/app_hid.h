/*
 * app_blehid.h
 *
 *  Created on: Oct 21, 2022
 *      Author: nimo
 */

#ifndef APP_INCLUDE_APP_HID_H_
#define APP_INCLUDE_APP_HID_H_

#include "config.h"
#include "app_sys.h"
#include "app_instructioncmd.h"
#include "app_param.h"
#include "app_net.h"

//服务特征相关参数
/* HID protocol mode values */
#define HID_PROTOCOL_MODE_BOOT            0x00  // Boot Protocol Mode
#define HID_PROTOCOL_MODE_REPORT          0x01  // Report Protocol Mode

/* HID Report type */
#define HID_REPORT_TYPE_INPUT             1
#define HID_REPORT_TYPE_OUTPUT            2
#define HID_REPORT_TYPE_FEATURE           3
/* Attribute value lengths */
#define HID_PROTOCOL_MODE_LEN             1     // HID Protocol Mode
#define HID_INFORMATION_LEN               4     // HID Information
#define HID_REPORT_REF_LEN                2     // HID Report Reference Descriptor


//TMOS任务事件
#define APP_HID_SATRT_EVENT				0x0001
#define APP_HID_PARAM_UPDATE_EVENT		0x0002
#define APP_HID_SEND_DATA_TEST_EVENT    0x0004
#define APP_HID_ENABLE_NOTIFY_EVENT     0x0008
#define APP_HID_VERIFY_EVENT            0x0010







typedef struct{
    uint16_t connectionHandle;   //!< Connection Handle from controller used to ref the device
    uint8_t connRole;            //!< Connection formed as Master or Slave
    uint16_t connInterval;       //!< Connection Interval
    uint16_t connLatency;        //!< Connection Latency
    uint16_t connTimeout;        //!< Connection Timeout
    uint8_t *addr;
    uint8_t addrType;
}hidConnectionInfoStruct;

typedef struct
{
	uint16_t handle;     // Handle of report characteristic
    uint16_t cccdHandle; // Handle of CCCD for report characteristic
    uint8_t  id;         // Report ID
    uint8_t  type;       // Report type
    uint8_t  mode;       // Protocol mode (report or boot)
}hidRptInfoStruct;



void appHidBroadcasterCfg(uint8_t *devName);
void appHidDeviveNameCfg(void);
void appHidPeripheralInit(void);
uint8_t appHidReadAttrCB( uint16_t connHandle, gattAttribute_t *pAttr, uint8_t *pValue,
                            uint16_t *pLen, uint16_t offset, uint16_t maxLen, uint8_t method );
uint8_t appHidWriteAttrCB( uint16_t connHandle, gattAttribute_t *pAttr, uint8_t *pValue,
                            uint16_t len, uint16_t offset, uint8_t method );
void appHidSendReport(uint8_t id, uint8_t type, uint16_t len, uint8_t *pData);
uint8_t appEnableNotifyCtl(uint16_t connHandle, uint8_t enable);
void appHidSendNotifyData(uint8 *data, uint16 len);
void appHidRemoveBond(uint8_t number);
void appHidTerminalLink(void);
void appHidBroadcastTypeCtl(uint8_t onoff, uint8_t type);





#endif /* APP_INCLUDE_APP_HID_H_ */
