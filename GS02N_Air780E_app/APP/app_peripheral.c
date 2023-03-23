/*
 * app_peripheral.c
 *
 *  Created on: Feb 25, 2022
 *      Author: idea
 */
#include "app_peripheral.h"
#include "app_instructioncmd.h"
#include "app_port.h"
#include "app_sys.h"
#include "app_task.h"
#include "app_param.h"
#include "app_jt808.h"

/*
 * 全局变量声明
 */
tmosTaskID appPeripheralTaskId = INVALID_TASK_ID;
gapBondCBs_t appPeripheralGapBondCallBack;
gapRolesCBs_t appPeripheralGapRolesCallBack;
connectionInfoStruct appPeripheralConn;
/*
 * 函数声明
 */
static tmosEvents appPeripheralEventProcess(tmosTaskID taskID,
        tmosEvents events);
static void appPeripheralGapRolesRssiRead(uint16_t connHandle, int8_t newRSSI);
static void appPeripheralGapRolesStateNotify(gapRole_States_t newState,
        gapRoleEvent_t *pEvent);
static void appPeripheralGapRolesParamUpdate(uint16_t connHandle,
        uint16_t connInterval, uint16_t connSlaveLatency, uint16_t connTimeout);

/********************************************************
 * *定义UUID
 ********************************************************/

#define APP_SERVICE_UUID            0xFFE0
#define APP_CHARACTERISTIC1_UUID    0xFFE1

/********************************************************
 * *UUID换成小端
 ********************************************************/

static uint8 ServiceUUID[ATT_BT_UUID_SIZE] = { LO_UINT16(APP_SERVICE_UUID), HI_UINT16(APP_SERVICE_UUID)};
static uint8 Char1UUID[ATT_BT_UUID_SIZE] = { LO_UINT16(APP_CHARACTERISTIC1_UUID), HI_UINT16(APP_CHARACTERISTIC1_UUID)};

/********************************************************
 * *服务UUID配置信息
 ********************************************************/
static gattAttrType_t ServiceProfile =
{
    ATT_BT_UUID_SIZE, ServiceUUID
};

/********************************************************
 * *特征的属性值
 ********************************************************/
static uint8 char1_Properties = GATT_PROP_READ | GATT_PROP_WRITE
                                | GATT_PROP_NOTIFY;

/********************************************************
 * *特征存储区
 ********************************************************/
static uint8 char1ValueStore[4];
static gattCharCfg_t char1ClientConfig[4];

/********************************************************
 * *特征描述
 ********************************************************/

static uint8 char1Description[] = "appchar1";

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
    }
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
        //LogPrintf(DEBUG_ALL, "UUID[0x%X]==>Write Request,size:%d", uuid, len);
        debugLen = len > 50 ? 50 : len;
        byteToHexString(pValue, debugStr, debugLen);
        debugStr[debugLen * 2] = 0;
        //LogPrintf(DEBUG_ALL, "%s", debugStr);
        switch (uuid)
        {
            case APP_CHARACTERISTIC1_UUID:
                LogMessage(DEBUG_ALL, "- + - - + - - + - - + - - + - - + - - + - ^");
                LogPrintf(DEBUG_ALL, "BLE receive:%s", debugStr);
                LogMessage(DEBUG_ALL, "- + - - + - - + - - + - - + - - + - - + -");
                appPeripheralProtocolRecv(pValue, len);
                break;
            case GATT_CLIENT_CHAR_CFG_UUID:
                ret = GATTServApp_ProcessCCCWriteReq(connHandle, pAttr, pValue, len,
                                                     offset, GATT_CLIENT_CFG_NOTIFY);
                if(pValue[0] == 1)
                {
                    LogMessage(DEBUG_ALL, "Notify Success, Ble Connect Success");
                    sysinfo.bleConnStatus = 1;
                }
                break;
        }
    }
    else
    {
        ret = ATT_ERR_INVALID_HANDLE;
    }
    return ret;
}
static gattServiceCBs_t gattServiceCallBack = { appReadAttrCB, appWriteAttrCB,
                                                NULL,
                                              };

static void appHandleConnStatusCB(uint16 connHandle, uint8 changeType)
{
    // Make sure this is not loopback connection
    if (connHandle != LOOPBACK_CONNHANDLE)
    {
        // Reset Client Char Config if connection has dropped
        if ((changeType == LINKDB_STATUS_UPDATE_REMOVED)
                || ((changeType == LINKDB_STATUS_UPDATE_STATEFLAGS)
                    && (!linkDB_Up(connHandle))))
        {
            GATTServApp_InitCharCfg(connHandle, char1ClientConfig);
            sysinfo.bleConnStatus = 0;
        }
    }
}

static void appAddServer(void)
{
    // Initialize Client Characteristic Configuration attributes
    GATTServApp_InitCharCfg(INVALID_CONNHANDLE, char1ClientConfig);
    // Register with Link DB to receive link status change callback
    linkDB_Register(appHandleConnStatusCB);
    GATTServApp_RegisterService(appAttributeTable,
                                GATT_NUM_ATTRS(appAttributeTable), GATT_MAX_ENCRYPT_KEY_SIZE,
                                &gattServiceCallBack);
}

/*
 * 配置广播包信息
 */
void appPeripheralBroadcastInfoCfg(uint8 *broadcastnmae)
{
    uint8 len, advLen, u8Value;
    uint8 advertData[31];
    len = tmos_strlen(broadcastnmae);

    advLen = 0;
    advertData[advLen++] = len + 1;
    advertData[advLen++] = GAP_ADTYPE_LOCAL_NAME_COMPLETE;
    tmos_memcpy(advertData + advLen, broadcastnmae, len);
    advLen += len;
    //设置广播信息
    GAPRole_SetParameter(GAPROLE_ADVERT_DATA, advLen, advertData);
    u8Value = TRUE;
    GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(u8Value), &u8Value);
}

void appPeripheralDevNameCfg(void)
{
    char name[30];
    sprintf(name, "GS02PET-%s", sysparam.SN + 10);
    appPeripheralBroadcastInfoCfg(name);
}

/*
 * 外设程序初始化
 */
void appPeripheralInit(void)
{
    uint8 u8Value;
    uint16 u16Value;
    //外设初始化
    GAPRole_PeripheralInit();
    //注册任务
    appPeripheralTaskId = TMOS_ProcessEventRegister(appPeripheralEventProcess);

    appPeripheralGapBondCallBack.pairStateCB = NULL;
    appPeripheralGapBondCallBack.passcodeCB = NULL;
    appPeripheralGapRolesCallBack.pfnParamUpdate =
        appPeripheralGapRolesParamUpdate;
    appPeripheralGapRolesCallBack.pfnRssiRead = appPeripheralGapRolesRssiRead;
    appPeripheralGapRolesCallBack.pfnStateChange =
        appPeripheralGapRolesStateNotify;

    tmos_memset(&appPeripheralConn, 0, sizeof(appPeripheralConn));
    appPeripheralConn.connectionHandle = GAP_CONNHANDLE_INIT;
    //参数配置
    //开启广播
    appPeripheralDevNameCfg();
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

    appAddServer();
    tmos_set_event(appPeripheralTaskId, APP_PERIPHERAL_START_EVENT);
}

/*
 * *notify data
 */
static bStatus_t appNotify(uint16 connHandle, attHandleValueNoti_t *pNoti)
{
    uint16 value = GATTServApp_ReadCharCfg(connHandle, char1ClientConfig);

    // If notifications enabled
    if (value & GATT_CLIENT_CFG_NOTIFY)
    {
        // Set the handle
        pNoti->handle = appAttributeTable[2].handle;

        // Send the notification
        return GATT_Notification(connHandle, pNoti, FALSE);
    }
    return bleIncorrectMode;
}

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
    LogPrintf(DEBUG_ALL, "appGapMsgProcess==>OptionCode:0x%X", pMsg->gap.opcode);
    switch (pMsg->gap.opcode)
    {
        case GAP_SCAN_REQUEST_EVENT:
            byteToHexString(debug, pMsg->scanReqEvt.scannerAddr,
                                       B_ADDR_LEN);
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
    }
}
static void appPerihperalSysEventMsg(tmos_event_hdr_t *msg)
{
    LogPrintf(DEBUG_ALL, "SysMsgEvent=0x%X,Status=0x%X", msg->event, msg->status);
    switch (msg->event)
    {
        case GAP_MSG_EVENT:
            appGapMsgProcess((gapRoleEvent_t *) msg);
            break;
    }
}

/*
 * 外设事件处理
 */
static tmosEvents appPeripheralEventProcess(tmosTaskID taskID,
        tmosEvents events)
{
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
        GAPRole_PeripheralStartDevice(appPeripheralTaskId,
                                      &appPeripheralGapBondCallBack, &appPeripheralGapRolesCallBack);
        return events ^ APP_PERIPHERAL_START_EVENT;
    }
    if (events & APP_PERIPHERAL_PARAM_UPDATE_EVENT)
    {
        LogPrintf(DEBUG_ALL, "start app peripheral param updates");
        GAPRole_PeripheralConnParamUpdateReq(appPeripheralConn.connectionHandle, 6, 20, 0, 300, appPeripheralTaskId);
        return events ^ APP_PERIPHERAL_PARAM_UPDATE_EVENT;
    }
    return 0;
}
/*
 * 设备建立链接
 */
static void appEstablished(gapRoleEvent_t *pEvent)
{
    uint8 debug[20];
    if (pEvent->gap.opcode == GAP_LINK_ESTABLISHED_EVENT)
    {
        appPeripheralConn.connectionHandle = pEvent->linkCmpl.connectionHandle;
        appPeripheralConn.connInterval = pEvent->linkCmpl.connInterval;
        appPeripheralConn.connLatency = pEvent->linkCmpl.clockAccuracy;
        appPeripheralConn.connRole = pEvent->linkCmpl.connRole;
        appPeripheralConn.connTimeout = pEvent->linkCmpl.connTimeout;
        byteToHexString(debug, pEvent->linkCmpl.devAddr, 6);
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
        tmos_start_task(appPeripheralTaskId, APP_PERIPHERAL_PARAM_UPDATE_EVENT, MS1_TO_SYSTEM_TIME(3000));
    }
}

static void appGaproleWaitting(gapRoleEvent_t *pEvent)
{
    uint8 u8Value;
    //被断开
    if (pEvent->gap.opcode == GAP_LINK_TERMINATED_EVENT)
    {
        LogPrintf(DEBUG_ALL, "-------------------------------------");
        LogPrintf(DEBUG_ALL, "*****Device Terminate*****");
        LogPrintf(DEBUG_ALL, "TerminateHandle:%d", pEvent->linkTerminate.connectionHandle);
        LogPrintf(DEBUG_ALL, "TerminateRole:%d", pEvent->linkTerminate.connRole);
        LogPrintf(DEBUG_ALL, "TerminateReason:0x%X", pEvent->linkTerminate.reason);
        LogPrintf(DEBUG_ALL, "-------------------------------------");
    }
    //主动断开
    else if(pEvent->gap.opcode == GAP_END_DISCOVERABLE_DONE_EVENT)
    {
        LogMessage(DEBUG_ALL, "Waiting for advertising..");
    }
    u8Value = TRUE;
    GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8), &u8Value);

}

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

static void appPeripheralGapRolesRssiRead(uint16_t connHandle, int8_t newRSSI)
{
    LogPrintf(DEBUG_ALL, "ConnHandle %d, RSSI :%d", connHandle, newRSSI);
}

/*
 * 状态事件
 */
static void appPeripheralGapRolesStateNotify(gapRole_States_t newState,
        gapRoleEvent_t *pEvent)
{

    LogPrintf(DEBUG_ALL, "OptionCode=%d", pEvent->gap.opcode);
    switch (newState & GAPROLE_STATE_ADV_MASK)
    {
        case GAPROLE_STARTED:
            LogPrintf(DEBUG_ALL, "GAPRole started");
            break;
        case GAPROLE_ADVERTISING:
            LogPrintf(DEBUG_ALL, "GAPRole advertising");
            break;
        case GAPROLE_WAITING:
            LogPrintf(DEBUG_ALL, "GAPRole waitting");
            appGaproleWaitting(pEvent);
            break;
        case GAPROLE_CONNECTED:
            LogPrintf(DEBUG_ALL, "GAPRole connected");
            appEstablished(pEvent);
            break;
        case GAPROLE_CONNECTED_ADV:
            LogPrintf(DEBUG_ALL, "GAPRole connected adv");
            break;
        case GAPROLE_ERROR:
            LogPrintf(DEBUG_ALL, "GAPRole error");
            break;
        default:
            break;
    }
}

/*从机主动断连*/
void appPeripheralTerminateLink(void)
{
    GAPRole_TerminateLink(appPeripheralConn.connectionHandle);
    sysinfo.bleConnStatus = 0;
}


void appPeripheralProtocolRecv(uint8_t *rebuf, uint16_t len)
{
	jt808ReceiveParser(rebuf, len);
}



