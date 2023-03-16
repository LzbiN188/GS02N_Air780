/*
 * app_peripheral.c
 *
 *  Created on: Feb 25, 2022
 *      Author: idea
 */
#include "app_peripheral.h"
#include "aes.h"
#include "app_instructioncmd.h"
#include "app_port.h"
#include "app_sys.h"
#include "app_server.h"
#include "app_protocol.h"
#include "app_param.h"
#include "app_encrypt.h"
#include "aes.h"
/*
 * 全局变量声明
 */
//外设任务ID
tmosTaskID appPeripheralTaskId = INVALID_TASK_ID;
//绑定回调
gapBondCBs_t appPeripheralGapBondCallBack;
//外设角色事件回调
gapRolesCBs_t appPeripheralGapRolesCallBack;
//建立连接时信息保存
connectionInfoStruct appPeripheralConn;
/*
 * 函数声明
 */
static tmosEvents appPeripheralEventProcess(tmosTaskID taskID, tmosEvents events);
static void appPeripheralGapRolesRssiRead(uint16_t connHandle, int8_t newRSSI);
static void appPeripheralGapRolesStateNotify(gapRole_States_t newState, gapRoleEvent_t *pEvent);
static void appPeripheralGapRolesParamUpdate(uint16_t connHandle, uint16_t connInterval, uint16_t connSlaveLatency,
        uint16_t connTimeout);

/********************************************************
 * *定义UUID
 ********************************************************/

#define APP_SERVICE_UUID            0xFFE0
#define APP_CHARACTERISTIC1_UUID    0xFFE1
#define APP_SERVICE2_UUID			0xFF00
#define APP_CHARACTERISTIC2_UUID    0xFFE2

/********************************************************
 * *UUID换成小端
 ********************************************************/

static uint8 ServiceUUID[ATT_BT_UUID_SIZE] = { LO_UINT16(APP_SERVICE_UUID), HI_UINT16(APP_SERVICE_UUID)};
static uint8 Char1UUID[ATT_BT_UUID_SIZE] = { LO_UINT16(APP_CHARACTERISTIC1_UUID), HI_UINT16(APP_CHARACTERISTIC1_UUID)};
static uint8 Service2UUID[ATT_BT_UUID_SIZE] = { LO_UINT16(APP_SERVICE2_UUID), HI_UINT16(APP_SERVICE2_UUID)};
static uint8 Char2UUID[ATT_BT_UUID_SIZE] = { LO_UINT16(APP_CHARACTERISTIC2_UUID), HI_UINT16(APP_CHARACTERISTIC2_UUID)};


/********************************************************
 * *服务UUID配置信息
 ********************************************************/
static gattAttrType_t ServiceProfile =
{
    ATT_BT_UUID_SIZE, ServiceUUID
};

static gattAttrType_t Service2Profile =
{
    ATT_BT_UUID_SIZE, Service2UUID
};

/********************************************************
 * *特征的属性值
 ********************************************************/
static uint8 char1_Properties = GATT_PROP_READ | GATT_PROP_WRITE
                                | GATT_PROP_NOTIFY;
static uint8 char2_Properties = GATT_PROP_READ | GATT_PROP_WRITE
                                | GATT_PROP_NOTIFY;

/********************************************************
 * *特征存储区
 ********************************************************/
static uint8 char1ValueStore[4];
static gattCharCfg_t char1ClientConfig[4];

static uint8 char2ValueStore[4];
static gattCharCfg_t char2ClientConfig[4];

/********************************************************
 * *特征描述
 ********************************************************/

static uint8 char1Description[] = "appchar1";
static uint8 char2Description[] = "appchar2";

/********************************************************
 * *服务特征表
 ********************************************************/
static gattAttribute_t appAttributeTable[] =
{
    //Service
    {   { ATT_BT_UUID_SIZE, primaryServiceUUID }, //type
        GATT_PERMIT_READ, 0,
        (uint8 *)& ServiceProfile
    },
    /*特征1*/
    //声明特征
    {   { ATT_BT_UUID_SIZE, characterUUID },
        GATT_PERMIT_READ, 0, &char1_Properties
    },
    //具体特征值
    {   { ATT_BT_UUID_SIZE, Char1UUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE, 0, char1ValueStore
    },
    //客户端配置用于NOTIFY
    {   { ATT_BT_UUID_SIZE, clientCharCfgUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE, 0, (uint8 *) char1ClientConfig
    },
    //具体特征的用户描述
    {   { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ, 0, char1Description
    },
};

static gattAttribute_t appAttributeTable2[] =
{
	//Service2
	{	{ ATT_BT_UUID_SIZE, primaryServiceUUID }, //type
		GATT_PERMIT_READ, 0,
		(uint8 *)& Service2Profile
	},
	/*特征1*/
	//声明特征
	{	{ ATT_BT_UUID_SIZE, characterUUID },
		GATT_PERMIT_READ, 0, &char2_Properties
	},
	//具体特征值
	{	{ ATT_BT_UUID_SIZE, Char2UUID },
		GATT_PERMIT_READ | GATT_PERMIT_WRITE, 0, char2ValueStore
	},
	//客户端配置用于NOTIFY
	{	{ ATT_BT_UUID_SIZE, clientCharCfgUUID },
		GATT_PERMIT_READ | GATT_PERMIT_WRITE, 0, (uint8 *) char2ClientConfig
	},
	//具体特征的用户描述
	{	{ ATT_BT_UUID_SIZE, charUserDescUUID },
		GATT_PERMIT_READ, 0, char2Description
	},

};


/********************************************************
 * *添加服务回调
 ********************************************************/
static bStatus_t appReadAttrCB(uint16 connHandle, gattAttribute_t *pAttr,
                               uint8 *pValue, uint16 *pLen, uint16 offset, uint16 maxLen, uint8 method)
{
    bStatus_t ret = SUCCESS;
    uint16 uuid;
    if (gattPermitAuthorRead(pAttr->permissions))
    {
        return ATT_ERR_INSUFFICIENT_AUTHOR;
    }
    if (offset > 0)
    {
        return ATT_ERR_ATTR_NOT_LONG;
    }
    if (pAttr->type.len == ATT_BT_UUID_SIZE)
    {
        uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);
        LogPrintf(DEBUG_ALL, "UUID[0x%X]==>Read Request", uuid);
        switch (uuid)
        {
            case APP_CHARACTERISTIC1_UUID:
                *pLen = 4;
                tmos_memcpy(pValue, pAttr->pValue, 4);
                break;
        }
    }
    else
    {
        *pLen = 0;
        ret = ATT_ERR_INVALID_HANDLE;
    }

    return ret;
}
//SN:999913436051195,292,3.77,46
void bleRecvParser(char *data, uint16_t len)
{
    ITEM item;
    insParam_s insparam;
    bleInfo_s devInfo;
    int index;
    char *rebuf;
    int16_t relen;
    char dec[256];
    uint8_t decLen, ret;
    char debugStr[100];
    tmos_stop_task(appPeripheralTaskId, APP_PERIPHERAL_TERMINATE_EVENT);
    ret = dencryptData(dec, &decLen, data, len);
    if (ret == 0)
    {
        //解析失败
        dencryptStr(data, len, dec, &decLen);
        tmos_memset(debugStr, 0, 100);
        byteToHexString(dec, debugStr, decLen);
        LogPrintf(DEBUG_ALL, "解密后[%s]", debugStr);
        return;
    }

    rebuf = dec;
    relen = decLen;

    if ((index = my_getstrindex(rebuf, "RE:", relen)) >= 0)
    {
        setInsId();
        insparam.data = rebuf;
        insparam.len = relen;
        protocolSend(BLE_LINK, PROTOCOL_21, &insparam);
    }
    else if ((index = my_getstrindex(rebuf, "SN:", relen)) >= 0)
    {
        rebuf += index + 3;
        relen -= index + 3;
        stringToItem(&item, rebuf, relen);
        if (item.item_cnt == 4)
        {
            strncpy(devInfo.imei, item.item_data[0], 15);
            devInfo.imei[15] = 0;
            devInfo.startCnt = atoi(item.item_data[1]);
            devInfo.vol = atof(item.item_data[2]);
            devInfo.batLevel = atoi(item.item_data[3]);

            bleServerAddInfo(devInfo);
            tmos_start_task(appPeripheralTaskId, APP_UPDATE_MCU_RTC_EVENT, MS1_TO_SYSTEM_TIME(200));
        }
    }
}


static bStatus_t appWriteAttrCB(uint16 connHandle, gattAttribute_t *pAttr,
                                uint8 *pValue, uint16 len, uint16 offset, uint8 method)
{
    bStatus_t ret = SUCCESS;
    uint16 uuid;
    uint8_t debugStr[101], debugLen;
    if (gattPermitAuthorWrite(pAttr->permissions))
    {
        return ATT_ERR_INSUFFICIENT_AUTHOR;
    }
    if (pAttr->type.len == ATT_BT_UUID_SIZE)
    {
        uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);
        LogPrintf(DEBUG_ALL, "UUID[0x%X]==>Write Request,size:%d", uuid, len);
        debugLen = len > 50 ? 50 : len;
        byteToHexString(pValue, debugStr, debugLen);
        debugStr[debugLen * 2] = 0;
        LogPrintf(DEBUG_ALL, "%s", debugStr);
        switch (uuid)
        {
            case APP_CHARACTERISTIC1_UUID:
                bleRecvParser(pValue, len);
                break;
            case GATT_CLIENT_CHAR_CFG_UUID:
                ret = GATTServApp_ProcessCCCWriteReq(connHandle, pAttr, pValue, len,
                                                     offset, GATT_CLIENT_CFG_NOTIFY);
                break;
        }
    }
    else
    {
        ret = ATT_ERR_INVALID_HANDLE;
    }
    return ret;
}
static gattServiceCBs_t gattServiceCallBack = { appReadAttrCB, appWriteAttrCB, NULL};

static void appHandleConnStatusCB(uint16 connHandle, uint8 changeType)
{
    if (connHandle != LOOPBACK_CONNHANDLE)
    {
        if ((changeType == LINKDB_STATUS_UPDATE_REMOVED)
                || ((changeType == LINKDB_STATUS_UPDATE_STATEFLAGS)
                    && (!linkDB_Up(connHandle))))
        {
            GATTServApp_InitCharCfg(connHandle, char1ClientConfig);
            GATTServApp_InitCharCfg(connHandle, char2ClientConfig);
        }
    }
}
/*
 * 添加新服务
 */

static void appAddServer(void)
{
	GATTServApp_InitCharCfg(INVALID_CONNHANDLE, char2ClientConfig);
    linkDB_Register(appHandleConnStatusCB);
    GATTServApp_RegisterService(appAttributeTable2, GATT_NUM_ATTRS(appAttributeTable2), GATT_MAX_ENCRYPT_KEY_SIZE,
                                &gattServiceCallBack);

    GATTServApp_InitCharCfg(INVALID_CONNHANDLE, char1ClientConfig);
    linkDB_Register(appHandleConnStatusCB);
    GATTServApp_RegisterService(appAttributeTable, GATT_NUM_ATTRS(appAttributeTable), GATT_MAX_ENCRYPT_KEY_SIZE,
                                &gattServiceCallBack);

}

/*
 * 配置广播包信息
 */
void appPeripheralBroadcastInfoCfg(uint8 *broadcastnmae)
{
    uint8 len, advLen;
    uint8 advertData[31];
    len = tmos_strlen(broadcastnmae);
    advLen = 0;
    advertData[advLen++] = len + 1;
    advertData[advLen++] = GAP_ADTYPE_LOCAL_NAME_COMPLETE;
    tmos_memcpy(advertData + advLen, broadcastnmae, len);
    advLen += len;
    GAPRole_SetParameter(GAPROLE_ADVERT_DATA, advLen, advertData);
    	uint8_t u8Value;
	u8Value = TRUE;
    GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8), &u8Value);
}

void appPeripheralCancel(void)
{
	uint8_t u8Value;
	u8Value = FALSE;
    GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8), &u8Value);
}
/*
 * 外设程序初始化
 */
void appPeripheralInit(void)
{
    char broadCastNmae[30];
    uint8 u8Value;
    uint16 u16Value;
    //外设初始化
    GAPRole_PeripheralInit();
    //注册任务
    appPeripheralTaskId = TMOS_ProcessEventRegister(appPeripheralEventProcess);

    appPeripheralGapBondCallBack.pairStateCB = NULL;
    appPeripheralGapBondCallBack.passcodeCB = NULL;
    appPeripheralGapRolesCallBack.pfnParamUpdate = appPeripheralGapRolesParamUpdate;
    appPeripheralGapRolesCallBack.pfnRssiRead = appPeripheralGapRolesRssiRead;
    appPeripheralGapRolesCallBack.pfnStateChange = appPeripheralGapRolesStateNotify;

    tmos_memset(&appPeripheralConn, 0, sizeof(appPeripheralConn));
    appPeripheralConn.connectionHandle = GAP_CONNHANDLE_INIT;
    //参数配置
    //开启广播
    u8Value = TRUE;
    GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8), &u8Value);
    //配置广播信息
    sprintf(broadCastNmae, "%s-%s", "AUTO", sysparam.SN + 9);
    appPeripheralBroadcastInfoCfg(broadCastNmae);
    //配置最短连接间隔
    u16Value = 0x0006;    //6*1.25=7.5ms
    GAPRole_SetParameter(GAPROLE_MIN_CONN_INTERVAL, sizeof(uint16), &u16Value);
    //配置最长连接间隔
    u16Value = 0x0c80;    //3200*1.25=4000ms
    GAPRole_SetParameter(GAPROLE_MIN_CONN_INTERVAL, sizeof(uint16), &u16Value);
    //配置最短广播间隔
    //unit:0.625ms*160=100ms
    GAP_SetParamValue(TGAP_DISC_ADV_INT_MIN, 160);
    //配置最长广播间隔
    GAP_SetParamValue(TGAP_DISC_ADV_INT_MAX, 160);
    //使能扫描应答通知
    GAP_SetParamValue(TGAP_ADV_SCAN_REQ_NOTIFY, FALSE);
    //添加服务
    GGS_AddService(GATT_ALL_SERVICES);           // GAP
    GATTServApp_AddService(GATT_ALL_SERVICES);   // GATT attributes
    appAddServer();
    //GATT_InitClient();
    tmos_set_event(appPeripheralTaskId, APP_PERIPHERAL_START_EVENT);
}

/*
 * *notify data
 */
static bStatus_t appNotify(uint16 connHandle, attHandleValueNoti_t *pNoti)
{
    uint16 value = GATTServApp_ReadCharCfg(connHandle, char1ClientConfig);
    if (value & GATT_CLIENT_CFG_NOTIFY)
    {
        pNoti->handle = appAttributeTable[2].handle;
        return GATT_Notification(connHandle, pNoti, FALSE);
    }
    return bleIncorrectMode;
}
/*
 * 发送通知
 */

void appSendNotifyData(uint8 *data, uint16 len)
{
    attHandleValueNoti_t notify;
    notify.len = len;
    notify.pValue = GATT_bm_alloc(appPeripheralConn.connectionHandle, ATT_HANDLE_VALUE_NOTI, notify.len, NULL, 0);
    if (notify.pValue == NULL)
    {
        LogPrintf(DEBUG_ALL, "appSendNotifyData==>alloc memory fail");
        return;
    }
    tmos_memcpy(notify.pValue, data, notify.len);
    if (appNotify(appPeripheralConn.connectionHandle, &notify) != SUCCESS)
    {
        GATT_bm_free((gattMsg_t *)&notify, ATT_HANDLE_VALUE_NOTI);
        LogPrintf(DEBUG_ALL, "Notify fail");
    }
    else
    {
        LogPrintf(DEBUG_ALL, "Notify success");
    }
}

static void appGapMsgProcess(gapRoleEvent_t *pMsg)
{
    int8 debug[20];
    switch (pMsg->gap.opcode)
    {
        case GAP_SCAN_REQUEST_EVENT:
            byteToHexString(pMsg->scanReqEvt.scannerAddr, debug, B_ADDR_LEN);
            debug[B_ADDR_LEN * 2] = 0;
            LogPrintf(DEBUG_ALL, "ScannerMac:%s", debug);
            break;
        case GAP_PHY_UPDATE_EVENT:
            LogPrintf(DEBUG_ALL, "-------------------------------------");
            LogPrintf(DEBUG_ALL, "*****LinkPhyUpdate*****");
            LogPrintf(DEBUG_ALL, "connHandle:%d", pMsg->linkPhyUpdate.connectionHandle);
            LogPrintf(DEBUG_ALL, "connRxPHYS:%d", pMsg->linkPhyUpdate.connRxPHYS);
            LogPrintf(DEBUG_ALL, "connTxPHYS:%d", pMsg->linkPhyUpdate.connTxPHYS);
            LogPrintf(DEBUG_ALL, "-------------------------------------");
            break;
        default:

            LogPrintf(DEBUG_ALL, "appGapMsgProcess==>OptionCode:0x%X,Unrealize!!!", pMsg->gap.opcode);
            break;
    }
}

static void appGattMsgProcess(gattMsgEvent_t *pMsg)
{
    LogPrintf(DEBUG_ALL, "doGattMsgMethod==>%#X", pMsg->method);
    switch (pMsg->method)
    {
        case ATT_EXCHANGE_MTU_RSP:
            LogPrintf(DEBUG_ALL, "Server rx mtu==>%d", pMsg->msg.exchangeMTURsp.serverRxMTU);
            break;
        case ATT_MTU_UPDATED_EVENT:
            LogPrintf(DEBUG_ALL, "Att mtu update to==>%d", pMsg->msg.mtuEvt.MTU);
            break;
        default:
            LogPrintf(DEBUG_ALL, "unprocess");
            break;
    }
}
static void appPerihperalSysEventMsg(tmos_event_hdr_t *msg)
{
    switch (msg->event)
    {
        case GAP_MSG_EVENT:
            appGapMsgProcess((gapRoleEvent_t *) msg);
            break;
        case GATT_MSG_EVENT:
            appGattMsgProcess((gattMsgEvent_t *)msg);
            break;
        default:
            LogPrintf(DEBUG_ALL, "Undo Event[0x%X][0x%X]", msg->event, msg->status);
            break;
    }
}
static void appChangeMtu(void)
{
    bStatus_t ret;
    attExchangeMTUReq_t pReq;
    pReq.clientRxMTU = BLE_BUFF_MAX_LEN - 4;
    ret = GATT_ExchangeMTU(appPeripheralConn.connectionHandle, &pReq, appPeripheralTaskId);
    LogPrintf(DEBUG_ALL, "appChangeMtu==>Ret:0x%X", ret);
}

static void sendRtcDateTime(void)
{
    uint16_t year;
    uint8_t  month;
    uint8_t date;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    char msg[20];
    char encrypt[128];
    uint8_t encryptLen;
    portGetRtcDateTime(&year, &month, &date, &hour, &minute, &second);
    sprintf(msg, "TIME,%d,%d,%d,%d,%d,%d", year, month, date, hour, minute, second);
    encryptData(encrypt, &encryptLen, msg, strlen(msg));
    LogMessage(DEBUG_ALL, "send datetime");
    appSendNotifyData(encrypt, encryptLen);
}

void appLocalNotifyAutoCtl(void)
{
    uint16 cccd = 0;
    uint8 ret;
    cccd |= GATT_CLIENT_CFG_NOTIFY;
    ret = GATTServApp_WriteCharCfg(appPeripheralConn.connectionHandle, char1ClientConfig, cccd);
    LogPrintf(DEBUG_ALL, "enable notify ret:%d", ret);
}


/*
 * 外设事件处理
 */
static tmosEvents appPeripheralEventProcess(tmosTaskID taskID, tmosEvents events)
{
	bStatus_t status;
    if (events & SYS_EVENT_MSG)
    {
        uint8 *pMsg;
        if ((pMsg = tmos_msg_receive(appPeripheralTaskId)) != NULL)
        {
            appPerihperalSysEventMsg((tmos_event_hdr_t *) pMsg);
            tmos_msg_deallocate(pMsg);
        }
        return (events ^ SYS_EVENT_MSG);
    }
    if (events & APP_PERIPHERAL_START_EVENT)
    {
        status = GAPRole_PeripheralStartDevice(appPeripheralTaskId,
                                      &appPeripheralGapBondCallBack, &appPeripheralGapRolesCallBack);
        if (status == SUCCESS)
        {
			LogMessage(DEBUG_ALL, "slavor role init..");
        }
        else 
        {
			LogPrintf(DEBUG_ALL, "slavor role init error, ret:0x%0x2", status);
        }
        return events ^ APP_PERIPHERAL_START_EVENT;
    }
    if (events & APP_PERIPHERAL_PARAM_UPDATE_EVENT)
    {
        GAPRole_PeripheralConnParamUpdateReq(appPeripheralConn.connectionHandle, 6, 20, 0, 300, appPeripheralTaskId);
        return events ^ APP_PERIPHERAL_PARAM_UPDATE_EVENT;
    }
    if (events & APP_PERIPHERAL_MTU_CHANGE_EVENT)
    {
        appChangeMtu();
        return events ^ APP_PERIPHERAL_MTU_CHANGE_EVENT;
    }
    if (events & APP_UPDATE_MCU_RTC_EVENT)
    {
        sendRtcDateTime();
        return events ^ APP_UPDATE_MCU_RTC_EVENT;
    }
    if (events & APP_PERIPHERAL_TERMINATE_EVENT)
    {
        GAPRole_TerminateLink(appPeripheralConn.connectionHandle);
        return events ^ APP_PERIPHERAL_TERMINATE_EVENT;
    }
    if (events & APP_PERIPHERAL_NOTIFY_EVENT)
    {
        appLocalNotifyAutoCtl();
        return events ^ APP_PERIPHERAL_NOTIFY_EVENT;
    }
    if (events & APP_START_AUTH_EVENT)
    {
        uint32_t result;
        result = tmos_rand();
        LogPrintf(DEBUG_ALL, "rand:%u", result % 0xFF);
        result = startAuthentication(result % 0xFF, 1, 2, appSendNotifyData);
        LogPrintf(DEBUG_ALL, "Auth Result:%d", result);
        return events ^ APP_START_AUTH_EVENT;
    }
    return 0;
}
/*
 * 设备建立链接
 */
static void appPeripheralConnected(gapRoleEvent_t *pEvent)
{
    uint8 debug[20];
    if (pEvent->gap.opcode == GAP_LINK_ESTABLISHED_EVENT)
    {
        appPeripheralConn.connectionHandle = pEvent->linkCmpl.connectionHandle;
        appPeripheralConn.connInterval = pEvent->linkCmpl.connInterval;
        appPeripheralConn.connLatency = pEvent->linkCmpl.clockAccuracy;
        appPeripheralConn.connRole = pEvent->linkCmpl.connRole;
        appPeripheralConn.connTimeout = pEvent->linkCmpl.connTimeout;
        byteToHexString(pEvent->linkCmpl.devAddr, debug, 6);
        debug[12] = 0;
        LogPrintf(DEBUG_ALL, "-------------------------------------");
        LogPrintf(DEBUG_ALL, "*****Device Connection*****");
        LogPrintf(DEBUG_ALL, "DeviceMac:%s", debug);
        LogPrintf(DEBUG_ALL, "DeviceType:%d", pEvent->linkCmpl.devAddrType);

        LogPrintf(DEBUG_ALL, "connectionHandle:%d", pEvent->linkCmpl.connectionHandle);
        LogPrintf(DEBUG_ALL, "connInterval:%d", pEvent->linkCmpl.connInterval);
        LogPrintf(DEBUG_ALL, "connLatency:%d", pEvent->linkCmpl.connLatency);
        LogPrintf(DEBUG_ALL, "connRole:%d", pEvent->linkCmpl.connRole);
        LogPrintf(DEBUG_ALL, "connTimeout:%d", pEvent->linkCmpl.connTimeout);
        LogPrintf(DEBUG_ALL, "-------------------------------------");
        tmos_set_event(appPeripheralTaskId, APP_PERIPHERAL_MTU_CHANGE_EVENT);
        //5秒后断开链接，除非鉴权通过
        tmos_start_task(appPeripheralTaskId, APP_PERIPHERAL_TERMINATE_EVENT, MS1_TO_SYSTEM_TIME(10000));
        tmos_start_task(appPeripheralTaskId, APP_PERIPHERAL_NOTIFY_EVENT, MS1_TO_SYSTEM_TIME(100));
        tmos_start_task(appPeripheralTaskId, APP_START_AUTH_EVENT, MS1_TO_SYSTEM_TIME(1200));
    }
}
/*
 * 设备等待
 */

static void appGaproleWaitting(gapRoleEvent_t *pEvent)
{
    uint8 u8Value;
    if (pEvent->gap.opcode == GAP_LINK_TERMINATED_EVENT)
    {
        pEvent->linkTerminate.connectionHandle;
        LogPrintf(DEBUG_ALL, "-------------------------------------");
        LogPrintf(DEBUG_ALL, "*****Device Terminate*****");
        LogPrintf(DEBUG_ALL, "TerminateHandle:%d", pEvent->linkTerminate.connectionHandle);
        LogPrintf(DEBUG_ALL, "TerminateRole:%d", pEvent->linkTerminate.connRole);
        LogPrintf(DEBUG_ALL, "TerminateReason:0x%X", pEvent->linkTerminate.reason);
        LogPrintf(DEBUG_ALL, "-------------------------------------");
        u8Value = TRUE;
        appPeripheralConn.connectionHandle = INVALID_CONNHANDLE;
        GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8), &u8Value);

        tmos_stop_task(appPeripheralTaskId, APP_PERIPHERAL_TERMINATE_EVENT);
    }
}
/*
 * 参数更新
 */
static void appPeripheralGapRolesParamUpdate(uint16_t connHandle,
        uint16_t connInterval, uint16_t connSlaveLatency, uint16_t connTimeout)
{
    if (appPeripheralConn.connectionHandle == connHandle)
    {
        appPeripheralConn.connInterval = connInterval;
        appPeripheralConn.connLatency = connSlaveLatency;
        appPeripheralConn.connTimeout = connTimeout;
    }
    LogPrintf(DEBUG_ALL, "-------------------------------------");
    LogPrintf(DEBUG_ALL, "*****ParamUpdate*****");
    LogPrintf(DEBUG_ALL, "connectionHandle:%d", connHandle);
    LogPrintf(DEBUG_ALL, "connInterval:%d", connInterval);
    LogPrintf(DEBUG_ALL, "connLatency:%d", connSlaveLatency);
    LogPrintf(DEBUG_ALL, "connTimeout:%d", connTimeout);
    LogPrintf(DEBUG_ALL, "-------------------------------------");
}
/*
 * 信号值
 */
static void appPeripheralGapRolesRssiRead(uint16_t connHandle, int8_t newRSSI)
{
    LogPrintf(DEBUG_ALL, "peripheral==>Conn[%d],rssi:%d", connHandle, newRSSI);
}
/*
 * 状态事件
 */
static void appPeripheralGapRolesStateNotify(gapRole_States_t newState,        gapRoleEvent_t *pEvent)
{

    switch (newState & GAPROLE_STATE_ADV_MASK)
    {
        case GAPROLE_STARTED:
            LogPrintf(DEBUG_ALL, "peripheral==>start");
            break;
        case GAPROLE_ADVERTISING:
            LogPrintf(DEBUG_ALL, "peripheral==>advertising");
            break;
        case GAPROLE_WAITING:
            LogPrintf(DEBUG_ALL, "peripheral==>waitting");
            appGaproleWaitting(pEvent);
            break;
        case GAPROLE_CONNECTED:
            LogPrintf(DEBUG_ALL, "peripheral==>connected");
            appPeripheralConnected(pEvent);
            break;
        case GAPROLE_CONNECTED_ADV:
            LogPrintf(DEBUG_ALL, "peripheral==>connected and adv");
            break;
        case GAPROLE_ERROR:
            LogPrintf(DEBUG_ALL, "peripheral==>error");
            break;
        default:
            break;
    }
}
