#include "app_central.h"
#include "app_sys.h"
#include "app_bleRelay.h"
#include "app_task.h"
#include "app_server.h"
#include "app_param.h"
//全局变量

tmosTaskID bleCentralTaskId = INVALID_TASK_ID;
static gapBondCBs_t bleBondCallBack;
static gapCentralRoleCB_t bleRoleCallBack;
static deviceConnInfo_s devInfoList[DEVICE_MAX_CONNECT_COUNT];
static bleScheduleInfo_s bleSchedule;

//函数声明
static tmosEvents bleCentralTaskEventProcess(tmosTaskID taskID, tmosEvents event);
static void bleCentralEventCallBack(gapRoleEvent_t *pEvent);
static void bleCentralHciChangeCallBack(uint16_t connHandle, uint16_t maxTxOctets, uint16_t maxRxOctets);
static void bleCentralRssiCallBack(uint16_t connHandle, int8_t newRSSI);
static void bleDevConnInit(void);

static void bleDevConnSuccess(uint8_t *addr, uint16_t connHandle);
static void bleDevDisconnect(uint16_t connHandle);
static void bleDevSetCharHandle(uint16_t connHandle, uint16_t handle);
static void bleDevSetNotifyHandle(uint16_t connHandle, uint16_t handle);

static void bleDevSetServiceHandle(uint16_t connHandle, uint16_t findS, uint16_t findE);
static void bleDevDiscoverAllServices(void);
static void bleDevDiscoverAllChars(uint16_t connHandle);
static void bleDevDiscoverNotify(uint16_t connHandle);
static uint8_t bleDevEnableNotify(void);
static uint8_t bleDevSendDataTest(void);

static void bleSchduleChangeFsm(bleFsm nfsm);
static void bleScheduleTask(void);


/**************************************************
@bref       BLE主机初始化
@param
@return
@note
**************************************************/

void bleCentralInit(void)
{
    bleDevConnInit();
    bleRelayInit();
    GAPRole_CentralInit();
    GAP_SetParamValue(TGAP_DISC_SCAN, 12800);
    GAP_SetParamValue(TGAP_CONN_EST_INT_MIN, 20);
    GAP_SetParamValue(TGAP_CONN_EST_INT_MAX, 100);
    GAP_SetParamValue(TGAP_CONN_EST_SUPERV_TIMEOUT, 100);

    bleCentralTaskId = TMOS_ProcessEventRegister(bleCentralTaskEventProcess);
    GATT_InitClient();
    GATT_RegisterForInd(bleCentralTaskId);

    bleBondCallBack.pairStateCB = NULL;
    bleBondCallBack.passcodeCB = NULL;

    bleRoleCallBack.eventCB = bleCentralEventCallBack;
    bleRoleCallBack.ChangCB = bleCentralHciChangeCallBack;
    bleRoleCallBack.rssiCB = bleCentralRssiCallBack;

    tmos_set_event(bleCentralTaskId, BLE_TASK_START_EVENT);
    tmos_start_reload_task(bleCentralTaskId, BLE_TASK_SCHEDULE_EVENT, MS1_TO_SYSTEM_TIME(1000));
}

/**************************************************
@bref       GATT系统消息事件处理
@param
@return
@note
**************************************************/

static void gattMessageHandler(gattMsgEvent_t *pMsgEvt)
{
    char debug[100];
    uint8 dataLen, infoLen, numOf, i;
    uint8 *pData;
    uint8_t uuid16[16];
    uint16 uuid, startHandle, endHandle, findHandle;
    bStatus_t status;
    LogPrintf(DEBUG_BLE, "Handle[%d],Method[0x%02X],Status[0x%02X]", pMsgEvt->connHandle, pMsgEvt->method,
              pMsgEvt->hdr.status);
    switch (pMsgEvt->method)
    {
        case ATT_ERROR_RSP:
            LogPrintf(DEBUG_BLE, "Error,Handle[%d],ReqOpcode[0x%02X],ErrCode[0x%02X]", pMsgEvt->msg.errorRsp.handle,
                      pMsgEvt->msg.errorRsp.reqOpcode, pMsgEvt->msg.errorRsp.errCode);
            break;
        //查找服务 BY UUID
        case ATT_FIND_BY_TYPE_VALUE_RSP:
            numOf = pMsgEvt->msg.findByTypeValueRsp.numInfo;
            pData = pMsgEvt->msg.findByTypeValueRsp.pHandlesInfo;
            //LogPrintf(DEBUG_ALL, "numInfo:%d", numOf);

            if (numOf != 0)
            {
                startHandle = ATT_ATTR_HANDLE(pData, 0);
                endHandle = ATT_GRP_END_HANDLE(pData, 0);
                LogPrintf(DEBUG_BLE, "Start:0x%04X,End:0x%04X", startHandle, endHandle);
                bleDevSetServiceHandle(pMsgEvt->connHandle, startHandle, endHandle);
            }

            if (pMsgEvt->hdr.status == bleProcedureComplete || pMsgEvt->hdr.status == bleTimeout)
            {
                LogPrintf(DEBUG_BLE, "Discover services by UUID done!");
                bleDevDiscoverAllChars(pMsgEvt->connHandle);
            }
            break;
        //查找服务 ALL
        case ATT_READ_BY_GRP_TYPE_RSP:
            infoLen = pMsgEvt->msg.readByGrpTypeRsp.len;
            numOf = pMsgEvt->msg.readByGrpTypeRsp.numGrps;
            pData = pMsgEvt->msg.readByGrpTypeRsp.pDataList;
            dataLen = infoLen * numOf;
            if (infoLen != 0)
            {
                byteArrayInvert(pData, dataLen);
                for (i = 0; i < numOf; i++)
                {
                    uuid = 0;
                    switch (infoLen)
                    {
                        case 6:
                            uuid = pData[6 * i];
                            uuid <<= 8;
                            uuid |= pData[6 * i + 1];

                            endHandle = pData[6 * i + 2];
                            endHandle <<= 8;
                            endHandle |= pData[6 * i + 3];

                            startHandle = pData[6 * i + 4];
                            startHandle <<= 8;
                            startHandle |= pData[6 * i + 5];

                            LogPrintf(DEBUG_BLE, "ServUUID: [%04X],Start:0x%04X,End:0x%04X", uuid, startHandle, endHandle);
                            break;
                        case 20:
                            memcpy(uuid16, pData + (20 * i), 16);
                            endHandle = pData[20 * i + 16];
                            endHandle <<= 8;
                            endHandle |= pData[20 * i + 17];

                            startHandle = pData[20 * i + 18];
                            startHandle <<= 8;
                            startHandle |= pData[20 * i + 19];
                            byteToHexString(uuid16, debug, 16);
                            debug[32] = 0;
                            LogPrintf(DEBUG_BLE, "ServUUID: [%s],Start:0x%04X,End:0x%04X", debug, startHandle, endHandle);
                            break;
                    }
                    if (uuid == SERVICE_UUID)
                    {
                        LogPrintf(DEBUG_BLE, "Find my services uuid [%04X]", uuid);
                        bleDevSetServiceHandle(pMsgEvt->connHandle, startHandle, endHandle);
                    }
                }

            }
            if (pMsgEvt->hdr.status == bleProcedureComplete || pMsgEvt->hdr.status == bleTimeout)
            {
                LogPrintf(DEBUG_BLE, "Discover all services done!");
                bleDevDiscoverAllChars(pMsgEvt->connHandle);
            }
            break;
        //查找特征
        case ATT_READ_BY_TYPE_RSP:
            infoLen = pMsgEvt->msg.readByTypeRsp.len;
            numOf = pMsgEvt->msg.readByTypeRsp.numPairs;
            pData = pMsgEvt->msg.readByTypeRsp.pDataList;
            dataLen = infoLen * numOf;
            if (infoLen != 0)
            {
                //内容反转
                byteArrayInvert(pData, dataLen);
                for (i = 0; i < numOf; i++)
                {
                    switch (infoLen)
                    {
                        case 4:
                            findHandle = pData[4 * i + 2];
                            findHandle <<= 8;
                            findHandle |= pData[4 * i + 3];
                            LogPrintf(DEBUG_BLE, "Find Handle: 0x%04X", findHandle);
                            bleDevSetNotifyHandle(pMsgEvt->connHandle, findHandle);
                            //寻找到notify的句柄，使能notify
                            tmos_set_event(bleCentralTaskId, BLE_TASK_NOTIFYEN_EVENT);
                            break;
                        case 7:
                            uuid = pData[7 * i];
                            uuid <<= 8;
                            uuid |= pData[7 * i + 1];

                            findHandle = pData[7 * i + 2];
                            findHandle <<= 8;
                            findHandle |= pData[7 * i + 3];

                            LogPrintf(DEBUG_BLE, "CharUUID:[%04X],Handle: 0x%04X", uuid, findHandle);

                            if (uuid == CHAR_UUID)
                            {
                                LogPrintf(DEBUG_BLE, "Find my CHARUUID [%04X]", uuid);
                                bleDevSetCharHandle(pMsgEvt->connHandle, findHandle);
                            }
                            break;
                        default:
                            LogPrintf(DEBUG_ALL, "Unprocess infoLen:%d", infoLen);
                            break;
                    }
                }

            }
            if (pMsgEvt->hdr.status == bleProcedureComplete || pMsgEvt->hdr.status == bleTimeout)
            {
                LogPrintf(DEBUG_BLE, "Discover all chars done!");
                //这里需要注意，如果没有notify，可能造成循环找notify
                bleDevDiscoverNotify(pMsgEvt->connHandle);
            }
            break;
        //数据发送回复
        case ATT_WRITE_RSP:
            LogPrintf(DEBUG_BLE, "Handle[%d] send %s!", pMsgEvt->connHandle, pMsgEvt->hdr.status == SUCCESS ? "success" : "fail");
            break;
        //收到notify数据
        case ATT_HANDLE_VALUE_NOTI:
            pData = pMsgEvt->msg.handleValueNoti.pValue;
            dataLen = pMsgEvt->msg.handleValueNoti.len;
            byteToHexString(pData, debug, dataLen);
            debug[dataLen * 2] = 0;
            LogPrintf(DEBUG_BLE, "Handle[%d],Recv:[%s]", pMsgEvt->connHandle, debug);
            bleRelayRecvParser(pMsgEvt->connHandle, pData, dataLen);
            break;
        default:
            LogPrintf(DEBUG_BLE, "It is unprocessed!!!");
            break;
    }
    GATT_bm_free(&pMsgEvt->msg, pMsgEvt->method);
}


/**************************************************
@bref       系统消息事件处理
@param
@return
@note
**************************************************/

static void sysMessageHandler(tmos_event_hdr_t *pMsg)
{
    switch (pMsg->event)
    {
        case GATT_MSG_EVENT:
            gattMessageHandler((gattMsgEvent_t *)pMsg);
            break;
        default:
            LogPrintf(DEBUG_BLE, "Unprocessed Event 0x%02X", pMsg->event);
            break;
    }
}


/**************************************************
@bref       主机任务事件处理
@param
@return
@note
**************************************************/

static tmosEvents bleCentralTaskEventProcess(tmosTaskID taskID, tmosEvents event)
{
    uint8_t *pMsg;
    bStatus_t status;
    if (event & SYS_EVENT_MSG)
    {
        if ((pMsg = tmos_msg_receive(bleCentralTaskId)) != NULL)
        {
            sysMessageHandler((tmos_event_hdr_t *) pMsg);
            tmos_msg_deallocate(pMsg);
        }
        return (event ^ SYS_EVENT_MSG);
    }
    if (event & BLE_TASK_START_EVENT)
    {
        status = GAPRole_CentralStartDevice(bleCentralTaskId, &bleBondCallBack, &bleRoleCallBack);
        if (status == SUCCESS)
        {
			LogMessage(DEBUG_BLE, "master role init..");
        }
        else 
        {
			LogPrintf(DEBUG_BLE, "master role init error, ret:0x%02x", status);
        }
        return event ^ BLE_TASK_START_EVENT;
    }
    if (event & BLE_TASK_SENDTEST_EVENT)
    {
        status = bleDevSendDataTest();
        if (status == blePending)
        {
            LogMessage(DEBUG_BLE, "send pending...");
            tmos_start_task(bleCentralTaskId, BLE_TASK_SENDTEST_EVENT, MS1_TO_SYSTEM_TIME(100));
        }
        return event ^ BLE_TASK_SENDTEST_EVENT;
    }
    if (event & BLE_TASK_NOTIFYEN_EVENT)
    {
        status = bleDevEnableNotify();
        if (status == blePending)
        {
            LogMessage(DEBUG_BLE, "notify pending...");
            tmos_start_task(bleCentralTaskId, BLE_TASK_NOTIFYEN_EVENT, MS1_TO_SYSTEM_TIME(100));
        }
        else
        {
            if (sysinfo.logLevel == 4)
            {
                LogMessage(DEBUG_FACTORY, "+FMPC:BLE CONNECT SUCCESS");
            }
            LogMessage(DEBUG_BLE, "Notify done!");
            bleSchduleChangeFsm(BLE_SCHEDULE_DONE);
        }
        return event ^ BLE_TASK_NOTIFYEN_EVENT;
    }
    if (event & BLE_TASK_CONTIMEOUT_EVENT)
    {
        //终止未成功链接的链路
        LogMessage(DEBUG_BLE, "Connection fail checkout!!!");
        GAPRole_TerminateLink(INVALID_CONNHANDLE);
        return event ^ BLE_TASK_CONTIMEOUT_EVENT;
    }
    if (event & BLE_TASK_SCHEDULE_EVENT)
    {
        bleScheduleTask();
        blePeriodTask();
        bleRelaySendDataTry();
        return event ^ BLE_TASK_SCHEDULE_EVENT;
    }
    return 0;
}
/**-----------------------------------------------------------------**/
/**-----------------------------------------------------------------**/
/**************************************************
@bref       处理扫描时扫到的蓝牙设备
@param
@return
@note
**************************************************/

static void deviceInfoEventHandler(gapDeviceInfoEvent_t *pEvent)
{
    uint8 i, dataLen, cmd;
    char debug[100];
    deviceScanInfo_s scaninfo;

    byteToHexString(pEvent->addr, debug, B_ADDR_LEN);
    debug[B_ADDR_LEN * 2] = 0;
    LogPrintf(DEBUG_BLE, "MAC:[%s],TYPE:0x%02X,RSSI:%d", debug, pEvent->addrType, pEvent->rssi);

    memset(&scaninfo, 0, sizeof(deviceScanInfo_s));
    memcpy(scaninfo.addr, pEvent->addr, B_ADDR_LEN);
    scaninfo.rssi = pEvent->rssi;
    scaninfo.addrType = pEvent->addrType;
    scaninfo.eventType = pEvent->eventType;


    if (pEvent->pEvtData != NULL && pEvent->dataLen != 0)
    {

        byteToHexString(pEvent->pEvtData, debug, pEvent->dataLen);
        debug[pEvent->dataLen * 2] = 0;
        //LogPrintf(DEBUG_ALL, "BroadCast:[%s]", debug);

        for (i = 0; i < pEvent->dataLen; i++)
        {
            dataLen = pEvent->pEvtData[i];
            if ((dataLen + i + 1) > pEvent->dataLen)
            {
                return ;
            }
            cmd = pEvent->pEvtData[i + 1];
            switch (cmd)
            {
                case GAP_ADTYPE_LOCAL_NAME_SHORT:
                case GAP_ADTYPE_LOCAL_NAME_COMPLETE:
                    if (dataLen > 30)
                    {
                        break;
                    }
                    tmos_memcpy(scaninfo.broadcaseName, pEvent->pEvtData + i + 2, dataLen - 1);
                    scaninfo.broadcaseName[dataLen - 1] = 0;
                    LogPrintf(DEBUG_BLE, "<---->BroadName:[%s]", scaninfo.broadcaseName);
                    break;
                default:
                    //LogPrintf(DEBUG_ALL, "UnsupportCmd:0x%02X", cmd);
                    break;
            }

            i += dataLen;
        }
    }

}
/**************************************************
@bref       与从机建立链接成功
@param
@return
@note
**************************************************/

void linkEstablishedEventHandler(gapEstLinkReqEvent_t *pEvent)
{
    char debug[20];

    if (pEvent->hdr.status != SUCCESS)
    {
        LogPrintf(DEBUG_BLE, "Link established error,Status:[0x%X]", pEvent->hdr.status);
        return;
    }
    byteToHexString(pEvent->devAddr, debug, 6);
    debug[12] = 0;
    LogPrintf(DEBUG_BLE, "Device [%s] connect success", debug);
    bleDevConnSuccess(pEvent->devAddr, pEvent->connectionHandle);
    bleDevDiscoverAllServices();
}

/**************************************************
@bref       与从机断开链接
@param
@return
@note
**************************************************/

void linkTerminatedEventHandler(gapTerminateLinkEvent_t *pEvent)
{
    LogPrintf(DEBUG_BLE, "Device disconnect,Handle [%d],Reason [0x%02X]", pEvent->connectionHandle, pEvent->reason);
    bleDevDisconnect(pEvent->connectionHandle);
}
/**-----------------------------------------------------------------**/
/**-----------------------------------------------------------------**/
/**************************************************
@bref       主机底层事件回调
@param
@return
@note
**************************************************/

static void bleCentralEventCallBack(gapRoleEvent_t *pEvent)
{
    LogPrintf(DEBUG_BLE, "bleCentral Event==>[0x%02X]", pEvent->gap.opcode);
    switch (pEvent->gap.opcode)
    {
        case GAP_DEVICE_INIT_DONE_EVENT:
            LogPrintf(DEBUG_BLE, "bleCentral init done!");
            break;
        case GAP_DEVICE_DISCOVERY_EVENT:
            LogPrintf(DEBUG_BLE, "bleCentral discovery done!");
            break;
        case GAP_ADV_DATA_UPDATE_DONE_EVENT:
            break;
        case GAP_MAKE_DISCOVERABLE_DONE_EVENT:
            break;
        case GAP_END_DISCOVERABLE_DONE_EVENT:
            break;
        case GAP_LINK_ESTABLISHED_EVENT:
            linkEstablishedEventHandler(&pEvent->linkCmpl);
            break;
        case GAP_LINK_TERMINATED_EVENT:
            linkTerminatedEventHandler(&pEvent->linkTerminate);
            break;
        case GAP_LINK_PARAM_UPDATE_EVENT:
            break;
        case GAP_RANDOM_ADDR_CHANGED_EVENT:
            break;
        case GAP_SIGNATURE_UPDATED_EVENT:
            break;
        case GAP_AUTHENTICATION_COMPLETE_EVENT:
            break;
        case GAP_PASSKEY_NEEDED_EVENT:
            break;
        case GAP_SLAVE_REQUESTED_SECURITY_EVENT:
            break;
        case GAP_DEVICE_INFO_EVENT:
            deviceInfoEventHandler(&pEvent->deviceInfo);
            break;
        case GAP_BOND_COMPLETE_EVENT:
            break;
        case GAP_PAIRING_REQ_EVENT:
            break;
        case GAP_DIRECT_DEVICE_INFO_EVENT:
            break;
        case GAP_PHY_UPDATE_EVENT:
            break;
        case GAP_EXT_ADV_DEVICE_INFO_EVENT:
            break;
        case GAP_MAKE_PERIODIC_ADV_DONE_EVENT:
            break;
        case GAP_END_PERIODIC_ADV_DONE_EVENT:
            break;
        case GAP_SYNC_ESTABLISHED_EVENT:
            break;
        case GAP_PERIODIC_ADV_DEVICE_INFO_EVENT:
            break;
        case GAP_SYNC_LOST_EVENT:
            break;
        case GAP_SCAN_REQUEST_EVENT:
            break;
    }
}
/**************************************************
@bref       MTU改变时系统回调
@param
@return
@note
**************************************************/

static void bleCentralHciChangeCallBack(uint16_t connHandle, uint16_t maxTxOctets, uint16_t maxRxOctets)
{
    LogPrintf(DEBUG_BLE, "Handle[%d] MTU change ,TX:%d , RX:%d", connHandle, maxTxOctets, maxRxOctets);
}
/**************************************************
@bref       主动读取rssi回调
@param
@return
@note
**************************************************/

static void bleCentralRssiCallBack(uint16_t connHandle, int8_t newRSSI)
{
    LogPrintf(DEBUG_BLE, "Handle[%d] respon rssi %d dB", connHandle, newRSSI);
}
/**************************************************
@bref       主动扫描
@param
@return
@note
**************************************************/

void bleCentralStartDiscover(void)
{
    bStatus_t status;
    status = GAPRole_CentralStartDiscovery(DEVDISC_MODE_ALL, TRUE, FALSE);
    LogPrintf(DEBUG_BLE, "Start discovery,ret=0x%02X", status);
}
/**************************************************
@bref       主动链接从机
@param
    addr        从机地址
    addrType    从机类型
@return
@note
**************************************************/

void bleCentralStartConnect(uint8_t *addr, uint8_t addrType)
{
    char debug[13];
    bStatus_t status;
    byteToHexString(addr, debug, 6);
    debug[12] = 0;
    status = GAPRole_CentralEstablishLink(FALSE, FALSE, addrType, addr);
    LogPrintf(DEBUG_BLE, "Start connect [%s](%d),ret=0x%02X", debug, addrType, status);
    if (status == SUCCESS)
    {
    }
    else
    {
        LogMessage(DEBUG_ALL, "Terminate link");
        GAPRole_TerminateLink(INVALID_CONNHANDLE);
    }
}
/**************************************************
@bref       主动断开与从机的链接
@param
    connHandle  从机句柄
@return
@note
**************************************************/

void bleCentralDisconnect(uint16_t connHandle)
{
    bStatus_t status;
    status = GAPRole_TerminateLink(connHandle);
    LogPrintf(DEBUG_BLE, "ble terminate Handle[%X],ret=0x%02X", connHandle, status);
}

/**************************************************
@bref       主动向从机发送数据
@param
    connHandle  从机句柄
    attrHandle  从机属性句柄
    data        数据
    len         长度
@return
    bStatus_t
@note
**************************************************/

uint8 bleCentralSend(uint16_t connHandle, uint16 attrHandle, uint8 *data, uint8 len)
{
    attWriteReq_t req;
    bStatus_t ret;
    req.handle = attrHandle;
    req.cmd = 0;
    req.sig = 0;
    req.len = len;
    req.pValue = GATT_bm_alloc(connHandle, ATT_WRITE_REQ, req.len, NULL, 0);
    if (req.pValue != NULL)
    {
        tmos_memcpy(req.pValue, data, len);
        ret = GATT_WriteCharValue(connHandle, &req, bleCentralTaskId);
        if (ret != SUCCESS)
        {
            GATT_bm_free((gattMsg_t *)&req, ATT_WRITE_REQ);
        }
    }
    LogPrintf(DEBUG_BLE, "bleCentralSend==>Ret:0x%02X", ret);
    return ret;
}


/*--------------------------------------------------*/


/**************************************************
@bref       链接列表初始化
@param
@return
@note
**************************************************/

static void bleDevConnInit(void)
{
    tmos_memset(&devInfoList, 0, sizeof(devInfoList));
}

/**************************************************
@bref       添加新的链接对象到链接列表中
@param
@return
    >0      添加成功，返回所添加的位置
    <0      添加失败
@note
**************************************************/

int8_t bleDevConnAdd(uint8_t *addr, uint8_t addrType)
{
    uint8_t i;

    for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
    {
        if (devInfoList[i].use == 0)
        {
            tmos_memset(&devInfoList[i], 0, sizeof(deviceConnInfo_s));
            tmos_memcpy(devInfoList[i].addr, addr, 6);
            devInfoList[i].addrType = addrType;
            devInfoList[i].connHandle = INVALID_CONNHANDLE;
            devInfoList[i].charHandle = INVALID_CONNHANDLE;
            devInfoList[i].notifyHandle = INVALID_CONNHANDLE;
            devInfoList[i].use = 1;
            return i;
        }
    }
    return -1;
}

/**************************************************
@bref       删除链接列表中的对象
@param
@return
    >0      删除成功，返回所添加的位置
    <0      删除失败
@note
**************************************************/

int8_t bleDevConnDel(uint8_t *addr)
{
    uint8_t i;
    for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
    {
        if (devInfoList[i].use && tmos_memcmp(addr, devInfoList[i].addr, 6) == TRUE)
        {
            devInfoList[i].use = 0;
            if (devInfoList[i].connHandle != INVALID_CONNHANDLE)
            {
                bleCentralDisconnect(devInfoList[i].connHandle);
            }
            return i;
        }
    }
    return -1;
}

void bleDevConnDelAll(void)
{
    uint8_t i;
    for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
    {
        if (devInfoList[i].use)
        {
            if (devInfoList[i].connHandle != INVALID_CONNHANDLE)
            {
                bleCentralDisconnect(devInfoList[i].connHandle);
            }
            tmos_memset(&devInfoList[i], 0, sizeof(deviceConnInfo_s));
        }
    }

}
/**************************************************
@bref       链接成功，查找对象并赋值句柄
@param
    addr        对象的mac地址
    connHandle  赋值对象的句柄
@return
@note
**************************************************/

static void bleDevConnSuccess(uint8_t *addr, uint16_t connHandle)
{
    uint8_t i;

    for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
    {
        if (devInfoList[i].use && devInfoList[i].connStatus == 0)
        {
            if (tmos_memcmp(devInfoList[i].addr, addr, 6) == TRUE)
            {
                devInfoList[i].connHandle = connHandle;
                devInfoList[i].connStatus = 1;
                devInfoList[i].notifyHandle = INVALID_CONNHANDLE;
                devInfoList[i].charHandle = INVALID_CONNHANDLE;
                devInfoList[i].findServiceDone = 0;
                devInfoList[i].notifyDone = 0;
                LogPrintf(DEBUG_BLE, "Get device conn handle [%d]", connHandle);
                return;
            }
        }
    }
}

/**************************************************
@bref       链接被断开时调用
@param
    connHandle  对象的句柄
@return
@note
**************************************************/

static void bleDevDisconnect(uint16_t connHandle)
{
    uint8_t i;
    char debug[20];
    for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
    {
        if (devInfoList[i].connHandle == connHandle)
        {
            devInfoList[i].connHandle = INVALID_CONNHANDLE;
            devInfoList[i].connStatus = 0;
            devInfoList[i].notifyHandle = INVALID_CONNHANDLE;
            devInfoList[i].charHandle = INVALID_CONNHANDLE;
            devInfoList[i].findServiceDone = 0;
            devInfoList[i].notifyDone = 0;
            byteToHexString(devInfoList[i].addr, debug, 6);
            debug[12] = 0;
            LogPrintf(DEBUG_BLE, "Device [%s] disconnect,Handle[%d]", debug, connHandle);
            return;
        }
    }
}

/**************************************************
@bref       获取到uuid的句柄
@param
    connHandle  对象的句柄
    handle      uuid属性句柄
@return
@note
**************************************************/

static void bleDevSetCharHandle(uint16_t connHandle, uint16_t handle)
{
    uint8_t i;
    for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
    {
        if (devInfoList[i].use &&  devInfoList[i].connHandle == connHandle)
        {
            devInfoList[i].charHandle = handle;
            LogPrintf(DEBUG_BLE, "CharHandle [0x%04X]", handle);
            return;
        }
    }
}

/**************************************************
@bref       获取到notify的句柄
@param
    connHandle  对象的句柄
    handle      notify属性句柄
@return
@note
**************************************************/

static void bleDevSetNotifyHandle(uint16_t connHandle, uint16_t handle)
{
    uint8_t i;
    for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
    {
        if (devInfoList[i].use &&  devInfoList[i].connHandle == connHandle && devInfoList[i].notifyHandle == INVALID_CONNHANDLE)
        {
            devInfoList[i].notifyHandle = handle;
            LogPrintf(DEBUG_BLE, "Notify Handle:[0x%04X]", handle);
            return;
        }
    }
}

/**************************************************
@bref       获取到services的句柄区间
@param
    connHandle  对象的句柄
    findS       起始句柄
    findE       结束句柄
@return
@note
**************************************************/

static void bleDevSetServiceHandle(uint16_t connHandle, uint16_t findS, uint16_t findE)
{
    uint8_t i;
    for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
    {
        if (devInfoList[i].use &&  devInfoList[i].connHandle == connHandle)
        {
            devInfoList[i].startHandle = findS;
            devInfoList[i].endHandle = findE;
            devInfoList[i].findServiceDone = 1;
            LogPrintf(DEBUG_BLE, "Set service handle [0x%04X~0x%04X]", findS, findE);
            return;
        }
    }
}

/**************************************************
@bref       查找所有服务
@param
@return
@note
**************************************************/

static void bleDevDiscoverAllServices(void)
{
    uint8_t i;
    //uint8_t uuid[2];
    bStatus_t status;
    for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
    {
        if (devInfoList[i].use && devInfoList[i].connHandle != INVALID_CONNHANDLE && devInfoList[i].findServiceDone == 0)
        {
            //uuid[0] = LO_UINT16(SERVICE_UUID);
            //uuid[1] = HI_UINT16(SERVICE_UUID);
            //status = GATT_DiscPrimaryServiceByUUID(devInfoList[i].connHandle, uuid, 2, bleCentralTaskId);

            status = GATT_DiscAllPrimaryServices(devInfoList[i].connHandle, bleCentralTaskId);
            LogPrintf(DEBUG_BLE, "Discover Handle[%d] all services,ret=0x%02X", devInfoList[i].connHandle, status);
            return;
        }
    }
}

/**************************************************
@bref       查找所有特征
@param
@return
@note
**************************************************/

static void bleDevDiscoverAllChars(uint16_t connHandle)
{
    uint8_t i;
    bStatus_t status;
    for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
    {
        if (devInfoList[i].use &&  devInfoList[i].connHandle == connHandle && devInfoList[i].findServiceDone == 1)
        {
            status = GATT_DiscAllChars(devInfoList[i].connHandle, devInfoList[i].startHandle, devInfoList[i].endHandle,
                                       bleCentralTaskId);
            LogPrintf(DEBUG_BLE, "Discover handle[%d] all chars,ret=0x%02X", devInfoList[i].connHandle, status);
            return;
        }
    }
}

/**************************************************
@bref       查找所有notify
@param
@return
@note
**************************************************/

static void bleDevDiscoverNotify(uint16_t connHandle)
{
    uint8_t i;
    bStatus_t status;
    attReadByTypeReq_t attReadByTypeReq;
    for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
    {
        if (devInfoList[i].use &&  devInfoList[i].connHandle == connHandle &&
                devInfoList[i].notifyHandle == INVALID_CONNHANDLE && devInfoList[i].findServiceDone == 1)
        {
            memset(&attReadByTypeReq, 0, sizeof(attReadByTypeReq_t));
            attReadByTypeReq.startHandle = devInfoList[i].startHandle;
            attReadByTypeReq.endHandle = devInfoList[i].endHandle;
            attReadByTypeReq.type.len = ATT_BT_UUID_SIZE;
            attReadByTypeReq.type.uuid[0] = LO_UINT16(GATT_CLIENT_CHAR_CFG_UUID);
            attReadByTypeReq.type.uuid[1] = HI_UINT16(GATT_CLIENT_CHAR_CFG_UUID);
            status = GATT_ReadUsingCharUUID(devInfoList[i].connHandle, &attReadByTypeReq, bleCentralTaskId);
            LogPrintf(DEBUG_BLE, "Discover notify,ret=0x%02X", status);
            return;
        }
    }
}

/**************************************************
@bref       使能notify
@param
@return
@note
**************************************************/

static uint8_t bleDevEnableNotify(void)
{
    uint8_t i;
    uint8_t data[2];
    bStatus_t status;
    for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
    {
        if (devInfoList[i].use && devInfoList[i].notifyHandle != INVALID_CONNHANDLE && devInfoList[i].notifyDone == 0)
        {
            data[0] = 0x01;
            data[1] = 0x00;
            LogPrintf(DEBUG_BLE, "Handle(%d)[%d] try to notify", i, devInfoList[i].connHandle);
            status = bleCentralSend(devInfoList[i].connHandle, devInfoList[i].notifyHandle, data, 2);
            if (status == SUCCESS)
            {
                devInfoList[i].notifyDone = 1;
            }
            return status;
        }
    }
    return 0;
}


/**************************************************
@bref       使能notify
@param
@return
@note
**************************************************/

static uint8_t bleDevSendDataTest(void)
{
    uint8_t i;
    uint8_t data[] = {0x0c, 0x01, 0x00, 0x01, 0x0d};
    bStatus_t status;
    for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
    {
        if (devInfoList[i].use && devInfoList[i].charHandle != INVALID_CONNHANDLE)
        {
            status = bleCentralSend(devInfoList[i].connHandle, devInfoList[i].charHandle, data, 5);
            return status;
        }
    }
    return 0;
}


/**************************************************
@bref       终止链路
@param
@return
@note
**************************************************/

void bleDevTerminate(void)
{
    uint8_t i;
    bStatus_t status;
    for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
    {
        if (devInfoList[i].use && devInfoList[i].connHandle != INVALID_CONNHANDLE)
        {
            status = GAPRole_TerminateLink(devInfoList[i].connHandle);
            LogPrintf(DEBUG_BLE, "Terminate Hanle[%d]", devInfoList[i].connHandle);
            return;
        }
    }
}

/**************************************************
@bref       通过地址查找对象
@param
@return
@note
**************************************************/

deviceConnInfo_s *bleDevGetInfo(uint8_t *addr)
{
    uint8_t i;
    for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
    {
        if (devInfoList[i].use && tmos_memcmp(devInfoList[i].addr, addr, 6) == TRUE)
        {
            return &devInfoList[i];
        }
    }
    return NULL;
}

/**************************************************
@bref       通过链接句柄，获取对应mac地址
@param
@return
@note
**************************************************/

uint8_t *bleDevGetAddrByHandle(uint16_t connHandle)
{
    uint8_t i;
    for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
    {
        if (devInfoList[i].use && devInfoList[i].connHandle == connHandle)
        {
            return devInfoList[i].addr;
        }
    }
    return NULL;
}


/**************************************************
@bref       蓝牙链接管理调度器状态切换
@param
@return
@note
**************************************************/

static void bleSchduleChangeFsm(bleFsm nfsm)
{
    bleSchedule.fsm = nfsm;
    bleSchedule.runTick = 0;
    LogPrintf(DEBUG_BLE, "bleSchduleChangeFsm==>%d", nfsm);
}

/**************************************************
@bref       蓝牙链接管理调度器
@param
@return
@note
**************************************************/

static void bleScheduleTask(void)
{
    static uint8_t ind = 0;
    static uint8_t noNetFlag = 0;
    static uint16_t noNetTick = 0;
    static uint8_t linkFlag = 0;
    static uint8_t disConnTick = 0;
    if (primaryServerIsReady() == 0)
    {
		noNetFlag = 1;
		//LogMessage(DEBUG_ALL, "NO NET.............");
    }
    else
    {
		noNetFlag = 0;
    }

    switch (bleSchedule.fsm)
    {
        case BLE_SCHEDULE_IDLE:
        	linkFlag = 0;
            ind = ind % DEVICE_MAX_CONNECT_COUNT;
            //查找是否有未链接的设备，需要进行链接
            for (; ind < DEVICE_MAX_CONNECT_COUNT; ind++)
            {
                if (devInfoList[ind].use && devInfoList[ind].connHandle == INVALID_CONNHANDLE)
                {
					linkFlag = 1;
					break;
                }
            }
			//没有设备需要连接了，维护链路逻辑
            if (noNetFlag)
            {
				//LogPrintf(DEBUG_ALL, "linkFlag:%d", linkFlag);
				//有链路需要连接
				if (linkFlag)
				{
					noNetTick++;
					if (sysparam.bleAutoDisc != 0)
					{
                        if (noNetTick > ((sysparam.bleAutoDisc * 60) / 2))
                        {
                            bleCentralStartConnect(devInfoList[ind].addr, devInfoList[ind].addrType);
                            bleSchduleChangeFsm(BLE_SCHEDULE_WAIT);
                            noNetTick = 0;
                            break;
                        }
					}
					disConnTick = 0;
				}
				//无链路需要连接
				else
				{
					if (disConnTick++ >= 30)
					{
						disConnTick = 0;
						bleDevTerminate();
					}
					noNetTick = 0;
				}
            }
            else
            {
            	if (linkFlag)
            	{
            		LogPrintf(DEBUG_BLE, "Start connect (%d)",ind);
	            	bleCentralStartConnect(devInfoList[ind].addr, devInfoList[ind].addrType);
	                bleSchduleChangeFsm(BLE_SCHEDULE_WAIT);
					noNetTick = 0;
					disConnTick = 0;
					break;
				}
            }
            break;
        case BLE_SCHEDULE_WAIT:
            if (bleSchedule.runTick >= 20)
            {
                //链接超时
                LogPrintf(DEBUG_BLE, "bleSchedule==>timeout!!!");
                bleCentralDisconnect(devInfoList[ind].connHandle);
                bleSchduleChangeFsm(BLE_SCHEDULE_DONE);
                if (sysinfo.logLevel == 4)
                {
                    LogMessage(DEBUG_FACTORY, "+FMPC:BLE CONNECT FAIL");
                }
            }
            else
            {
                break;
            }
        case BLE_SCHEDULE_DONE:
            ind++;
            bleSchduleChangeFsm(BLE_SCHEDULE_IDLE);
            break;
        default:
            bleSchduleChangeFsm(BLE_SCHEDULE_IDLE);
            break;
    }
    bleSchedule.runTick++;
}
