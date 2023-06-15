/*
 * app_central.c
 *
 *  Created on: Jun 27, 2022
 *      Author: idea
 */
#include "app_central.h"
#include "app_sys.h"
#include "app_param.h"
#include "app_ble.h"


#define DISCOVER_SERVER_UUID		0xFFE0
#define DISCOVER_CHAR_UUID			0xFFE1




static scanList_s devScanList;

tmosTaskID appCentralTaskId = INVALID_TASK_ID;
static gapBondCBs_t appCentralGapbondCallBack;
static gapCentralRoleCB_t appCentralRoleCallBack;
static centralConnInfo_s  appCentralConnInfo;

static tmosEvents appCentralTaskEventProcess(tmosTaskID taskID, tmosEvents event);

/**
 * @brief   信号读取回调
 */
static void appCentralRssiReadCallBack(uint16_t connHandle, int8_t newRSSI)
{
    LogPrintf(DEBUG_ALL, "Handle(%d),RSSI:%d", connHandle, newRSSI);
}
/**
 * @brief   数据包长度更改回调
 */
static void appCentralHciDataLenChangeCallBack(uint16_t connHandle, uint16_t maxTxOctets, uint16_t maxRxOctets)
{
    LogPrintf(DEBUG_ALL, "Handle(%d),maxTxOctets:%d,maxRxOctets:%d", connHandle, maxTxOctets,
              maxRxOctets);
}


/**
 * @brief   添加设备信息
 */

static void addScanDevList(scanDevInfo_s *devInfo)
{
	uint8_t i;
	if (devScanList.devcnt >= SCAN_LIST_SIZE)
	{
		return;
	}
	for (i = 0; i < SCAN_LIST_SIZE; i++)
	{
		if (tmos_memcmp(devScanList.devlist[i].addr, devInfo->addr, 6) == TRUE)
		{
			return;
		}
	}
	memcpy(&devScanList.devlist[devScanList.devcnt++], devInfo, sizeof(scanDevInfo_s));
}

/**
 * @brief   扫描回调
 */
static void doDeviceInfoEvent(gapDeviceInfoEvent_t *pEvent)
{
    uint8 i, dataLen, cmd;
    char debug[30];
    scanDevInfo_s scaninfo;

    tmos_memset(&scaninfo, 0, sizeof(scanDevInfo_s));
    tmos_memcpy(&scaninfo.addr, pEvent->addr, 6);
    byteToHexString(scaninfo.addr, debug, 6);
    debug[12] = 0;
//    LogPrintf(DEBUG_ALL, "Mac:[%s],AddrType:0x%02X,EventType:0x%02x,Rssi:%d", debug, pEvent->addrType,
//              pEvent->eventType, pEvent->rssi);
	scaninfo.addrtype = pEvent->addrType;
	scaninfo.rssi = pEvent->rssi;
  	addScanDevList(&scaninfo);
	if (pEvent->pEvtData != NULL && pEvent->dataLen != 0)
    {
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
                    LogPrintf(DEBUG_ALL, "broadcastname:%s", scaninfo.broadcaseName);
                    //devScanInfoUpdate(&scaninfo);
                    break;
                default:
                    break;
            }

            i += dataLen;
        }
    }
   
//    if(pEvent->dataLen == 0)
//        return;

//    for (i = 0; i < pEvent->dataLen; i++)
//    {
//        dataLen = pEvent->pEvtData[i];
//        if ((dataLen + i + 1) > pEvent->dataLen)
//        {
//            return ;
//        }
//        cmd = pEvent->pEvtData[i + 1];
//        switch (cmd)
//        {
//            case GAP_ADTYPE_LOCAL_NAME_SHORT:
//            case GAP_ADTYPE_LOCAL_NAME_COMPLETE:
//                if (dataLen > 30)
//                {
//                    break;
//                }
//                tmos_memcpy(scaninfo.broadcaseName, pEvent->pEvtData + i + 2, dataLen - 1);
//                scaninfo.broadcaseName[dataLen - 1] = 0;
//                break;
//            case 0xF1:
//                if (dataLen == 2)
//                {
//                    scaninfo.request = pEvent->pEvtData[i + 2] ? 1 : 0;
//                }
//                break;
//            case 0xF2:
//                if (dataLen == 2)
//                {
//                    scaninfo.elec = pEvent->pEvtData[i + 2];
//                }
//                break;
//            case 0xF3:
//                if (dataLen == 4)
//                {
//                    scaninfo.userId = pEvent->pEvtData[i + 2];
//                    scaninfo.type = pEvent->pEvtData[i + 3];
//                    scaninfo.version = pEvent->pEvtData[i + 4];
//                }
//                break;
//            case 0xF4:
//                if (dataLen == 2)
//                {
//                    scaninfo.lockStatus = pEvent->pEvtData[i + 2] ? 1 : 0;
//                }
//                break;
//            case 0xF5:
//                if (dataLen == 2)
//                {
//                    scaninfo.scanMinute = pEvent->pEvtData[i + 2];
//                }
//                break;
//            case 0xF6:
//                if (dataLen == 2)
//                {
//                    scaninfo.chainStatus = pEvent->pEvtData[i + 2] ? 1 : 0;
//                }
//                break;
//            default:
//                //LogPrintf(DEBUG_ALL, "UnsupportCmd:0x%02X", cmd);
//                break;
//        }
//
//        i += dataLen;
//    }
//
//    if (/*scaninfo.userId == 0xEF*/1)
//    {
//        for(i = 0; i < devScanList.devcnt; i++)
//        {
//            if(tmos_memcmp(scaninfo.addr, devScanList.devlist[i].addr, B_ADDR_LEN) == TRUE)
//            {
//                byteToHexString(scaninfo.addr, debug, 6);
//                debug[12] = 0;
//                scaninfo.rssi = pEvent->rssi;
//                devScanInfoUpdate(&devScanList.devlist[i], &scaninfo);
//                devScanList.devlist[i].hadscan = 1;
//                LogPrintf(DEBUG_ALL, "Find Dev[%s] ==>rssi:%d, type:%d, elec:%d", debug, scaninfo.rssi, scaninfo.type, scaninfo.elec);
//                if(tmos_memcmp(scaninfo.addr, devScanList.targetDev, B_ADDR_LEN) == TRUE)
//                {
//                    tmos_memset(devScanList.targetDev, 0, 6);
//                    appCentralStopScan();
//                }
//                return ;
//            }
//
//        }
//    }
}


static void gapDeviceDiscoveryEvent(gapDevDiscEvent_t *pEvent)
{
    LogPrintf(DEBUG_ALL, "搜索完成");
    sysinfo.scanNow = 0;
}

static void doEstablishEvent(gapEstLinkReqEvent_t *pEvent)
{
    char debug[50];
    byteToHexString( pEvent->devAddr, debug,B_ADDR_LEN);
    debug[B_ADDR_LEN * 2] = 0;
    if (pEvent->hdr.status == SUCCESS)
    {
        LogPrintf(DEBUG_ALL, "---------------------------------------");
        LogPrintf(DEBUG_ALL, "********Device Connected********");
        LogPrintf(DEBUG_ALL, "Mac: [%s] , AddrType: %d", debug, pEvent->devAddrType);
        LogPrintf(DEBUG_ALL, "connectionHandle: [%d]", pEvent->connectionHandle);
        LogPrintf(DEBUG_ALL, "connRole: [%d]", pEvent->connRole);
        LogPrintf(DEBUG_ALL, "connInterval: [%d]", pEvent->connInterval);
        LogPrintf(DEBUG_ALL, "connLatency: [%d]", pEvent->connLatency);
        LogPrintf(DEBUG_ALL, "connTimeout: [%d]", pEvent->connTimeout);
        LogPrintf(DEBUG_ALL, "clockAccuracy: [%d]", pEvent->clockAccuracy);
        LogPrintf(DEBUG_ALL, "---------------------------------------");
        memcpy(appCentralConnInfo.devAddr, pEvent->devAddr, B_ADDR_LEN);
        appCentralConnInfo.devAddrType = pEvent->devAddrType;
        appCentralConnInfo.connectionHandle = pEvent->connectionHandle;
        appCentralConnInfo.connRole = pEvent->connRole;
        appCentralConnInfo.connInterval = pEvent->connInterval;
        appCentralConnInfo.connLatency = pEvent->connLatency;
        appCentralConnInfo.connTimeout = pEvent->connTimeout;
        appCentralConnInfo.clockAccuracy = pEvent->clockAccuracy;
        appCentralConnInfo.findUUID = DISCOVER_SERVER_UUID;
        tmos_set_event(appCentralTaskId, APP_DISCOVER_SERVICES_EVENT);
    }
    else
    {
        sysinfo.linkConn = 0;
        sysinfo.linking = 0;
        appCentralConnInfo.connectionHandle = INVALID_CONNHANDLE;
        LogPrintf(DEBUG_ALL, "+CONNECT: FAIL, 0x%x", pEvent->hdr.status);
        tmos_stop_task(appCentralTaskId, APP_LINK_TIMEOUT_EVENT);
    }
}

static void doDirectDeviceInfoEvent(gapDirectDeviceInfoEvent_t *pEvent)
{
    char debug[100];
    byteToHexString( pEvent->addr, debug, B_ADDR_LEN);
    debug[B_ADDR_LEN * 2] = 0;
    LogPrintf(DEBUG_ALL, "Mac:[%s],AddrType:0x%02X,EventType:0x%02X,Rssi:%d", debug, pEvent->addrType,
              pEvent->eventType, pEvent->rssi);
}

static void doTerminatedEvent(gapTerminateLinkEvent_t *pEvent)
{
    LogPrintf(DEBUG_ALL, "---------------------------------------");
    LogPrintf(DEBUG_ALL, "********Device Terminate********");
    LogPrintf(DEBUG_ALL, "connectionHandle: [%d]", pEvent->connectionHandle);
    LogPrintf(DEBUG_ALL, "connRole: [%d]", pEvent->connRole);
    LogPrintf(DEBUG_ALL, "reason: [0x%02X]", pEvent->reason);
    LogPrintf(DEBUG_ALL, "---------------------------------------");
    appCentralConnInfo.connectionHandle = INVALID_CONNHANDLE;
    tmos_stop_task(appCentralTaskId, APP_DISCOVER_CHAR_EVENT);
    sysinfo.linkConn = 0;
    sysinfo.linking = 0;

    if(sysinfo.linkRetry)
    {
        LogMessage(DEBUG_ALL, "重新连接");
        appCentralEstablish(sysinfo.linkAddr);
    }
    else
    {
        LogPrintf(DEBUG_ALL, "+URC:Disconnected");
        tmos_stop_task(appCentralTaskId, APP_LINK_TIMEOUT_EVENT);
    }

}
/**
 * @brief   主机事件回调
 */
static void appCentralGapRoleEventCallBack(gapRoleEvent_t *pEvent)
{
    //LogPrintf(DEBUG_ALL, "gapRoleEvent==>0x%02X", pEvent->gap.opcode);
    switch (pEvent->gap.opcode)
    {
        case GAP_DEVICE_INIT_DONE_EVENT:
            LogPrintf(DEBUG_ALL, "device init done");


            break;
        case GAP_DEVICE_DISCOVERY_EVENT:

            gapDeviceDiscoveryEvent(&pEvent->discCmpl);
            break;
        case GAP_ADV_DATA_UPDATE_DONE_EVENT:
            break;
        case GAP_MAKE_DISCOVERABLE_DONE_EVENT:
            break;
        case GAP_END_DISCOVERABLE_DONE_EVENT:
            break;
        case GAP_LINK_ESTABLISHED_EVENT:
            doEstablishEvent(&pEvent->linkCmpl);
            break;
        case GAP_LINK_TERMINATED_EVENT:
            doTerminatedEvent(&pEvent->linkTerminate);
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
            doDeviceInfoEvent(&pEvent->deviceInfo);
            break;
        case GAP_BOND_COMPLETE_EVENT:
            break;
        case GAP_PAIRING_REQ_EVENT:
            break;
        case GAP_DIRECT_DEVICE_INFO_EVENT:
            doDirectDeviceInfoEvent(&pEvent->deviceDirectInfo);
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
/**
 * @brief   主机初始化
 * @note
 * @param
 * @return
 */
void appCentralInit(void)
{
    GAPRole_CentralInit();
    appCentralTaskId = TMOS_ProcessEventRegister(appCentralTaskEventProcess);
    appCentralConnInfo.connectionHandle = INVALID_CONNHANDLE;
    appCentralGapbondCallBack.pairStateCB = NULL;
    appCentralGapbondCallBack.passcodeCB = NULL;

    appCentralRoleCallBack.ChangCB = appCentralHciDataLenChangeCallBack;
    appCentralRoleCallBack.eventCB = appCentralGapRoleEventCallBack;
    appCentralRoleCallBack.rssiCB = appCentralRssiReadCallBack;

    GAP_SetParamValue(TGAP_DISC_SCAN, 12800);
    GAP_SetParamValue(TGAP_CONN_EST_INT_MIN, 20);
    GAP_SetParamValue(TGAP_CONN_EST_INT_MAX, 100);
    GAP_SetParamValue(TGAP_CONN_EST_SUPERV_TIMEOUT, 100);


    GATT_InitClient();
    GATT_RegisterForInd(appCentralTaskId);

    //appUpdateScanList();

    tmos_set_event(appCentralTaskId, APP_START_EVENT);
}

/*主机接收数据*/
void attValueHandleNotiEvent(attHandleValueNoti_t *notify)
{
    char debug[80];
    LogPrintf(DEBUG_ALL, "Notify Handle: 0x%02x", notify->handle);
    byteToHexString(notify->pValue, debug, notify->len);
    debug[notify->len * 2] = 0;
    LogMessage(DEBUG_ALL, "- + - - + - - + - - + - ^");
    LogPrintf(DEBUG_ALL, "Notify :%s", debug);
    LogMessage(DEBUG_ALL, "- + - - + - - + - - + - -");

    bleProtocolParser(notify->pValue, notify->len);
}

static void doGattMsgEvent(gattMsgEvent_t *pMsgEvt)
{
    char debug[100];
    uint8 dataLen, infoLen, numOf, i;
    uint8 *pData;
    uint8_t uuid16[16];
    uint16 uuid, start, end, handle;
    LogPrintf(DEBUG_ALL, "gattMsgEvent==>Handle:[%d],Method:[0x%02X]", pMsgEvt->connHandle, pMsgEvt->method);
    switch (pMsgEvt->method)
    {
        //!< ATT Error Response
        case ATT_ERROR_RSP:
            LogPrintf(DEBUG_ALL, "Error Req:%#02X , Error Code:%#02X , Status:%#02X", pMsgEvt->msg.errorRsp.reqOpcode,
                      pMsgEvt->msg.errorRsp.errCode, pMsgEvt->hdr.status);
            break;
        //!< ATT Exchange MTU Request
        case ATT_EXCHANGE_MTU_REQ:
            break;
        //!< ATT Exchange MTU Response
        case ATT_EXCHANGE_MTU_RSP:
            break;
        //!< ATT Find Information Request
        case ATT_FIND_INFO_REQ:
            break;
        //!< ATT Find Information Response
        case ATT_FIND_INFO_RSP:
            break;
        //!< ATT Find By Type Value Request
        case ATT_FIND_BY_TYPE_VALUE_REQ:
            break;
        //!< ATT Find By Type Value Response
        case ATT_FIND_BY_TYPE_VALUE_RSP:
            break;
        //!< ATT Read By Type Request
        case ATT_READ_BY_TYPE_REQ:
            break;
        //!< ATT Read By Type Response
        case ATT_READ_BY_TYPE_RSP:
            infoLen = pMsgEvt->msg.readByTypeRsp.len;
            numOf = pMsgEvt->msg.readByTypeRsp.numPairs;
            pData = pMsgEvt->msg.readByTypeRsp.pDataList;
            dataLen = infoLen * numOf;
            LogPrintf(DEBUG_ALL, "numGrps==>%d,Status:%02X", numOf, pMsgEvt->hdr.status);
            if (infoLen != 0)
            {
                //原始数据
                byteToHexString( pData,debug,dataLen);
                debug[dataLen * 2] = 0;
                LogPrintf(DEBUG_ALL, "Data:(%d)[%s]", dataLen, debug);
                //内容反转
                byteArrayInvert(pData, dataLen);
                byteToHexString( pData,debug, dataLen);
                debug[dataLen * 2] = 0;
                LogPrintf(DEBUG_ALL, "Data:(%d)[%s]", dataLen, debug);
                //180AFFFF0024 180F00230020 FEE7001F0017
                for (i = 0; i < numOf; i++)
                {
                    switch (infoLen)
                    {
                        /*
                        GATT_ReadUsingCharUUID
                        [00:00:00] Data:(4)[04000100]
                        [00:00:00] Data:(4)[00010004]
                        */
                        case 4:
                            handle = pData[4 * i + 2];
                            handle <<= 8;
                            handle |= pData[4 * i + 3];
                            LogPrintf(DEBUG_ALL, "findUUID[%04X]==>handle:0x%04X", appCentralConnInfo.findUUID, handle);
                            appCentralConnInfo.findIt = 1;
                            appCentralConnInfo.notifyHandle = handle;
                            break;
                        /*
                        [00:00:00] Data:(7)[02001A0300E1FF]
                        [00:00:00] Data:(7)[FFE100031A0002]
                        */
                        case 7:
                            uuid = pData[7 * i];
                            uuid <<= 8;
                            uuid |= pData[7 * i + 1];

                            handle = pData[7 * i + 2];
                            handle <<= 8;
                            handle |= pData[7 * i + 3];

                            LogPrintf(DEBUG_ALL, "CharUUID: [%04X],handle:0x%04X", uuid, handle);
                            if (uuid == appCentralConnInfo.findUUID)
                            {
                                appCentralConnInfo.findIt = 1;
                                appCentralConnInfo.writeHandle = handle;
                            }
                            break;
                    }
                }

            }
            if (pMsgEvt->hdr.status == bleProcedureComplete)
            {
                LogPrintf(DEBUG_ALL, "discover char done");
                if (appCentralConnInfo.findIt)
                {
                    appCentralConnInfo.findIt = 0;
                    switch (appCentralConnInfo.findUUID)
                    {
                        case GATT_CLIENT_CHAR_CFG_UUID:
                            LogPrintf(DEBUG_ALL, "find my notify UUID");
                            tmos_set_event(appCentralTaskId, APP_NOTIFY_ENABLE_EVENT);
                            sysinfo.linkConn = 1;
                            sysinfo.linking = 0;
                            tmos_stop_task(appCentralTaskId, APP_LINK_TIMEOUT_EVENT);
                            break;
                        case DISCOVER_CHAR_UUID:
                            LogPrintf(DEBUG_ALL, "find my char UUID");
                            tmos_set_event(appCentralTaskId, APP_DISCOVER_NOTIFY_EVENT);
                            break;
                        default:
                            LogPrintf(DEBUG_ALL, "hei,chekcout hear");
                            break;
                    }
                }
            }
            break;
        //!< ATT Read Request
        case ATT_READ_REQ:
            break;
        //!< ATT Read Response
        case ATT_READ_RSP:
            break;
        //!< ATT Read Blob Request
        case ATT_READ_BLOB_REQ:
            break;
        //!< ATT Read Blob Response
        case ATT_READ_BLOB_RSP:
            break;
        //!< ATT Read Multiple Request
        case ATT_READ_MULTI_REQ:
            break;
        //!< ATT Read Multiple Response
        case ATT_READ_MULTI_RSP:
            break;
        //!< ATT Read By Group Type Request
        case ATT_READ_BY_GRP_TYPE_REQ:
            break;
        //!< ATT Read By Group Type Response
        case ATT_READ_BY_GRP_TYPE_RSP:
            infoLen = pMsgEvt->msg.readByGrpTypeRsp.len;
            numOf = pMsgEvt->msg.readByGrpTypeRsp.numGrps;
            pData = pMsgEvt->msg.readByGrpTypeRsp.pDataList;
            dataLen = infoLen * numOf;
            LogPrintf(DEBUG_ALL, "numGrps==>%d,Status:%02X", numOf, pMsgEvt->hdr.status);
            if (infoLen != 0)
            {
                //原始数据
                byteToHexString( pData, debug,dataLen);
                debug[dataLen * 2] = 0;
                LogPrintf(DEBUG_ALL, "Data:(%d)[%s]", dataLen, debug);
                //内容反转
                byteArrayInvert(pData, dataLen);
                byteToHexString( pData,debug, dataLen);
                debug[dataLen * 2] = 0;
                LogPrintf(DEBUG_ALL, "Data:(%d)[%s]", dataLen, debug);
                //180AFFFF0024 180F00230020 FEE7001F0017
                for (i = 0; i < numOf; i++)
                {
                    switch (infoLen)
                    {
                        case 6:
                            uuid = pData[6 * i];
                            uuid <<= 8;
                            uuid |= pData[6 * i + 1];

                            end = pData[6 * i + 2];
                            end <<= 8;
                            end |= pData[6 * i + 3];

                            start = pData[6 * i + 4];
                            start <<= 8;
                            start |= pData[6 * i + 5];

                            LogPrintf(DEBUG_ALL, "ServUUID: [%04X],start:0x%04X,end:0x%04X", uuid, start, end);

                            //test
                            if (uuid == appCentralConnInfo.findUUID)
                            {
                                appCentralConnInfo.findStart = start;
                                appCentralConnInfo.findEnd = end;
                                appCentralConnInfo.findIt = 1;
                            }
                            break;
                        case 20:
                            memcpy(uuid16, pData + (20 * i), 16);
                            end = pData[20 * i + 16];
                            end <<= 8;
                            end |= pData[20 * i + 17];

                            start = pData[20 * i + 18];
                            start <<= 8;
                            start |= pData[20 * i + 19];
                            byteToHexString( uuid16,debug, 16);
                            debug[32] = 0;
                            LogPrintf(DEBUG_ALL, "ServUUID: [%s],start:0x%04X,end:0x%04X", debug, start, end);
                            break;
                    }
                }

            }
            if (pMsgEvt->hdr.status == bleProcedureComplete)
            {
                LogPrintf(DEBUG_ALL, "discover server done");
                if (appCentralConnInfo.findIt)
                {
                    appCentralConnInfo.findIt = 0;
                    tmos_set_event(appCentralTaskId, APP_DISCOVER_CHAR_EVENT);

                    LogPrintf(DEBUG_ALL, "find my server UUID");
                }
            }
            break;
        //!< ATT Write Request
        case ATT_WRITE_REQ:
            break;
        //!< ATT Write Response
        case ATT_WRITE_RSP:
            if (pMsgEvt->hdr.status == SUCCESS)
            {
                LogPrintf(DEBUG_ALL, "write success");
            }
            else
            {
                LogPrintf(DEBUG_ALL, "write fail , status 0x%X", pMsgEvt->hdr.status);
            }
            break;
        //!< ATT Prepare Write Request
        case ATT_PREPARE_WRITE_REQ:
            break;
        //!< ATT Prepare Write Response
        case ATT_PREPARE_WRITE_RSP:
            break;
        //!< ATT Execute Write Request
        case ATT_EXECUTE_WRITE_REQ:
            break;
        //!< ATT Execute Write Response
        case ATT_EXECUTE_WRITE_RSP:
            break;
        //!< ATT Handle Value Notification
        case ATT_HANDLE_VALUE_NOTI:
            attValueHandleNotiEvent(&pMsgEvt->msg.handleValueNoti);
            break;
        //!< ATT Handle Value Indication
        case ATT_HANDLE_VALUE_IND:
            break;
        //!< ATT Handle Value Confirmation
        case ATT_HANDLE_VALUE_CFM:
            break;
    }
    GATT_bm_free(&pMsgEvt->msg, pMsgEvt->method);
}

/**
 * @brief   处理系统消息事件
 * @note
 * @param
 * @return
 */
 
static void doSysEventMsg(tmos_event_hdr_t *pMsg)
{
    switch (pMsg->event)
    {
        case GATT_MSG_EVENT:
            doGattMsgEvent((gattMsgEvent_t *)pMsg);
            break;
        case GAP_MSG_EVENT:
            break;
        default:
            LogPrintf(DEBUG_ALL, "Unprocessed Event 0x%02X", pMsg->event);
            break;
    }
}

/**
 * @brief   主机事件处理
 * @note
 * @param
 * @return
 */
static tmosEvents appCentralTaskEventProcess(tmosTaskID taskID, tmosEvents event)
{
    uint8_t *pMsg;
    uint8 data[10];
    bStatus_t status;
    attReadByTypeReq_t attReadByTypeReq;
    if (event & SYS_EVENT_MSG)
    {
        if ((pMsg = tmos_msg_receive(appCentralTaskId)) != NULL)
        {
            doSysEventMsg((tmos_event_hdr_t *) pMsg);
            tmos_msg_deallocate(pMsg);
        }
        return (event ^ SYS_EVENT_MSG);
    }
    if (event & APP_START_EVENT)
    {
        GAPRole_CentralStartDevice(appCentralTaskId, &appCentralGapbondCallBack, &appCentralRoleCallBack);
        return event ^ APP_START_EVENT;
    }
    if (event & APP_DISCOVER_SERVICES_EVENT)
    {
        status = GATT_DiscAllPrimaryServices(appCentralConnInfo.connectionHandle, appCentralTaskId);
        LogPrintf(DEBUG_ALL, "GATT_DiscAllPrimaryServices==>ret:0x%02X", status);
        return event ^ APP_DISCOVER_SERVICES_EVENT;
    }
    if (event & APP_DISCOVER_CHAR_EVENT)
    {
        appCentralConnInfo.findUUID = DISCOVER_CHAR_UUID;
        status = GATT_DiscAllChars(appCentralConnInfo.connectionHandle, appCentralConnInfo.findStart,
                                   appCentralConnInfo.findEnd,
                                   appCentralTaskId);
        LogPrintf(DEBUG_ALL, "GATT_DiscAllChars==>ret:0x%02X", status);
        return event ^ APP_DISCOVER_CHAR_EVENT;
    }
    if (event & APP_DISCOVER_NOTIFY_EVENT)
    {
        memset(&attReadByTypeReq, 0, sizeof(attReadByTypeReq_t));
        attReadByTypeReq.startHandle = appCentralConnInfo.findStart;
        attReadByTypeReq.endHandle = appCentralConnInfo.findEnd;
        attReadByTypeReq.type.len = ATT_BT_UUID_SIZE;
        attReadByTypeReq.type.uuid[0] = LO_UINT16(GATT_CLIENT_CHAR_CFG_UUID);
        attReadByTypeReq.type.uuid[1] = HI_UINT16(GATT_CLIENT_CHAR_CFG_UUID);
        appCentralConnInfo.findUUID = GATT_CLIENT_CHAR_CFG_UUID;
        status = GATT_ReadUsingCharUUID(appCentralConnInfo.connectionHandle, &attReadByTypeReq, appCentralTaskId);
        LogPrintf(DEBUG_ALL, "GATT_ReadUsingCharUUID==>ret:0x%02X", status);
        return event ^ APP_DISCOVER_NOTIFY_EVENT;
    }
    if (event & APP_NOTIFY_ENABLE_EVENT)
    {
        data[0] = 0x01;
        data[1] = 0x00;

        status = appCentralWriteData(appCentralConnInfo.notifyHandle, data, 2);
        if (status == blePending)
        {
            LogPrintf(DEBUG_ALL, "blePending...");
            tmos_start_task(appCentralTaskId, APP_NOTIFY_ENABLE_EVENT, MS1_TO_SYSTEM_TIME(100));
        }
        if (status == SUCCESS)
        {
            LogMessage(DEBUG_ALL, "Enable notify success");
        }
        return event ^ APP_NOTIFY_ENABLE_EVENT;
    }
    if (event & APP_WRITEDATA_EVENT)
    {
        uint8 sendData[] = {0x0C, 0x01, 0x03, 0x04, 0x0D};
        LogPrintf(DEBUG_ALL, "发送断油电测试");
        status = appCentralWriteData(appCentralConnInfo.writeHandle, sendData, 5);
        if (status == blePending)
        {
            LogPrintf(DEBUG_ALL, "blePending...");
            tmos_start_task(appCentralTaskId, APP_WRITEDATA_EVENT, MS1_TO_SYSTEM_TIME(100));
        }
        return event ^ APP_WRITEDATA_EVENT;
    }
    if (event & APP_LINK_TIMEOUT_EVENT)
    {
        sysinfo.linkRetry = 0;
        appCentralTeiminate();
        LogPrintf(DEBUG_ALL, "+CONNECT: TIMEOUT");
        return event ^ APP_LINK_TIMEOUT_EVENT;
    }

    return 0;
}

/**
 * @brief   启动扫描
 * @note
 * @param
 * @return
 */

void appCentralStartScan(void)
{
    if(sysinfo.linking)
    {
        LogMessage(DEBUG_ALL, "linking now, stop scan");
        return;
    }
    sysinfo.scanNow = 1;
    GAPRole_CentralStartDiscovery(DEVDISC_MODE_ALL, TRUE, FALSE);
    LogPrintf(DEBUG_ALL, "start scan");
}

/**
 * @brief   停止扫描
 * @param
 * @return
 */

void appCentralStopScan(void)
{
    LogPrintf(DEBUG_ALL, "stop scan");
    GAPRole_CentralCancelDiscovery();
    sysinfo.scanNow = 0;
}

/**
 * @brief   建立连接
 * @param
 * @return
 */

void appCentralEstablish(uint8_t *mac)
{
    bStatus_t status;
    if(sysinfo.scanNow)
    {
        appCentralStopScan();
    }
    if(sysinfo.linking == 1 || sysinfo.linkConn == 1)
    {
        LogPrintf(DEBUG_ALL, "not allow to establish");
        return ;
    }
    status = GAPRole_CentralEstablishLink(TRUE, FALSE, 0, mac);
    sysinfo.linking = 1;
    if(status == SUCCESS)
    {
        LogPrintf(DEBUG_ALL, "+CONNECT:ING");
    }
    else
    {
        LogPrintf(DEBUG_ALL, "+CONNNECT:FAIL");
        appCentralTeiminate();
    }

    LogPrintf(DEBUG_ALL, "GAPRole_CentralEstablishLink==>Ret:0x%02X", status);
}

/**
 * @brief   终止链接
 * @param
 * @return
 */

void appCentralTeiminate(void)
{
    sysinfo.linkRetry = 0;
    LogPrintf(DEBUG_ALL, "terminate connection");

    GAPRole_TerminateLink(appCentralConnInfo.connectionHandle);

}

/**
 * @brief   主动写数据
 * @note
 * @param
 * @return
 */

uint8 appCentralWriteData(uint16 handle, uint8 *data, uint8 len)
{
    attWriteReq_t req;
    uint8 ret = 0x01;
    req.handle = handle;
    req.cmd = 0;
    req.sig = 0;
    req.len = len;
    req.pValue = GATT_bm_alloc(appCentralConnInfo.connectionHandle, ATT_WRITE_REQ, req.len, NULL, 0);
    if (req.pValue != NULL)
    {
        tmos_memcpy(req.pValue, data, len);
        ret = GATT_WriteCharValue(appCentralConnInfo.connectionHandle, &req, appCentralTaskId);
        if (ret != SUCCESS)
        {
            GATT_bm_free((gattMsg_t *)&req, ATT_WRITE_REQ);
        }
    }
    LogPrintf(DEBUG_ALL, "appCentralWriteData[%04X]==>Ret:0x%02X", handle, ret);
    return ret;
}

/**
 * @brief   显示扫描队列
 * @param
 * @return
 */

void appShowScanList(char *message)
{
    char mac[15];
    uint8 i;
	uint16_t len;
    sprintf(message, "+LIST: %d", devScanList.devcnt);
    
    for (i = 0; i < devScanList.devcnt; i++)
    {
        byteToHexString(devScanList.devlist[i].addr, mac, 6);
        mac[12] = 0;
        sprintf(message + strlen(message), "+DEV: %s,%d,%s,%d,%d\r\n", mac, devScanList.devlist[i].addrtype,\
	    												  					devScanList.devlist[i].broadcaseName,\
	    												  					devScanList.devlist[i].type,\
	   		 											  					devScanList.devlist[i].rssi);
    }
}


/**
 * @brief   查询扫描状态
 * @param
 * @return  1：扫描完成
 *          0：等待
 */
int DevStatusCheck(void)
{
    uint8_t i;
    char debug[20];
    if(sysinfo.scanNow)
    {
        return 0;
    }
    if(sysinfo.linking)
    {
        return 0;
    }

  	return 1;
}

/**
 * @brief   开启扫描请求
 * @param
 * @return
 */
void centralScanRequestSet(void)
{
    uint8_t i;
    sysinfo.masterScanReq = 1;
	tmos_memset(&devScanList, 0, sizeof(scanList_s));
    LogMessage(DEBUG_ALL, "centralScanRequestSet==>OK");
}

/**
 * @brief   关闭扫描请求
 * @param
 * @return
 */
void centralScanRequestClear(void)
{
    sysinfo.masterScanReq = 0;
    LogMessage(DEBUG_ALL, "centralScanRequestClear==>OK");
}

/**
 * @brief   蓝牙主机扫描任务
 * @note
 * @param
 * @return
 */
void appCentralScanTask(void)
{
    static uint8_t scanTime = 2;
    if (sysinfo.masterScanReq == 0)
    {
    	scanTime = 2;
        return;
    }
    if (DevStatusCheck() == 0)
    {
		return;
    }
	scanTime--;
    appCentralStartScan();
	if (scanTime == 0)
	{
		centralScanRequestClear();
	}
}


/**
 * @brief   获取扫描结果
 * @param
 * @return
 */
scanList_s *getDevScanlist(void)
{
    return &devScanList;
}


/**
 * @brief   发送数据
 * @param
 * @return
 */
uint8_t appCentralSendData(uint8_t *data, uint8_t len)
{

    return appCentralWriteData(appCentralConnInfo.writeHandle, data, len);
}


