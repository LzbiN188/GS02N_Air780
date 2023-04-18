/*
 * app_bleRelay.c
 *
 *  Created on: Oct 27, 2022
 *      Author: idea
 */
#include "app_bleRelay.h"
#include "app_central.h"
#include "app_param.h"
#include "app_task.h"
#include "aes.h"
#include "app_port.h"
#include "app_encrypt.h"
#include "app_instructioncmd.h"

static bleRelayDev_s bleRelayList[BLE_CONNECT_LIST_SIZE];


static void bleRelaySendProtocol(uint16_t connHandle, uint16_t charHandle, uint8_t cmd, uint8_t *data,
                                 uint8_t data_len);


/**************************************************
@bref       为对应蓝牙设备设置数据请求标志
@param
@return
@note
**************************************************/

void bleRelaySetReq(uint8_t ind, uint32_t req)
{
    if (ind >= BLE_CONNECT_LIST_SIZE)
    {
        return;
    }
    bleRelayList[ind].dataReq |= req;
    LogPrintf(DEBUG_ALL, "bleRelaySetReq[%d]:(%04X)", ind, req);
}

/**************************************************
@bref       为所有已使用蓝牙设备设置请求标志
@param
@return
@note
**************************************************/

void bleRelaySetAllReq(uint32_t req)
{
    uint8_t i;
    for (i = 0; i < BLE_CONNECT_LIST_SIZE; i++)
    {
        if (bleRelayList[i].used)
        {
            bleRelaySetReq(i, req);
        }
    }
}

/**************************************************
@bref       为对应蓝牙设备清除数据请求标志
@param
@return
@note
**************************************************/

void bleRelayClearReq(uint8_t ind, uint32_t req)
{
    if (ind >= BLE_CONNECT_LIST_SIZE)
    {
        return;
    }
    bleRelayList[ind].dataReq &= ~req;
    LogPrintf(DEBUG_ALL, "bleRelayClearReq(%d):[%04X]", ind, req);
}

/**************************************************
@bref       为所有已使用蓝牙设备清除请求标志
@param
@return
@note
**************************************************/

void bleRelayClearAllReq(uint32_t req)
{
    uint8_t i;
    for (i = 0; i < BLE_CONNECT_LIST_SIZE; i++)
    {
        if (bleRelayList[i].used)
        {
            bleRelayList[i].dataReq &= ~req;
            bleRelayClearReq(i, req);
        }
    }
}

/**************************************************
@bref       添加新的蓝牙设备到链接列表中
@param
@return
@note
**************************************************/

int8_t bleRelayInsert(uint8_t *addr, uint8_t addrType)
{
    int8_t ret = -1;
    uint8_t i;
    for (i = 0; i < BLE_CONNECT_LIST_SIZE; i++)
    {
        if (bleRelayList[i].used == 0)
        {
            tmos_memset(&bleRelayList[i], 0, sizeof(bleRelayDev_s));
            bleRelayList[i].used = 1;
            tmos_memcpy(bleRelayList[i].addr, addr, 6);
            bleRelayList[i].addrType = addrType;
            ret = bleDevConnAdd(addr, addrType);
            bleRelaySetReq(i, BLE_EVENT_SET_RF_THRE | BLE_EVENT_SET_OUTV_THRE | BLE_EVENT_SET_AD_THRE | BLE_EVENT_GET_AD_THRE |
                           BLE_EVENT_GET_RF_THRE | BLE_EVENT_GET_OUT_THRE | BLE_EVENT_GET_OUTV | BLE_EVENT_GET_RFV |
                           BLE_EVENT_GET_PRE_PARAM | BLE_EVENT_SET_PRE_PARAM | BLE_EVENT_GET_PRE_PARAM);
            return ret;

        }
    }
    return ret;
}

/**************************************************
@bref       初始化
@param
@return
@note
**************************************************/

void bleRelayInit(void)
{
    uint8_t i;
    uint8_t mac[6];
    tmos_memset(mac, 0, 6);
    tmos_memset(&bleRelayList, 0, sizeof(bleRelayList));
    for (i = 0; i < BLE_CONNECT_LIST_SIZE; i++)
    {
        if (tmos_memcmp(mac, sysparam.bleConnMac[i], 6) == TRUE)
        {
            continue;
        }
        bleRelayInsert(sysparam.bleConnMac[i], 0);
    }
    if (sysparam.relayCtl)
    {
        relayAutoRequest();
    }
}

/**************************************************
@bref       删除所有设备
@param
@return
@note
**************************************************/

void bleRelayDeleteAll(void)
{
    bleDevConnDelAll();
    tmos_memset(bleRelayList, 0, sizeof(bleRelayList));
}

/**************************************************
@bref       获取蓝牙信息
@param
@return
@note
**************************************************/

bleRelayInfo_s *bleRelayGeInfo(uint8_t i)
{
    if (i >= BLE_CONNECT_LIST_SIZE)
    {
        return NULL;
    }
    if (bleRelayList[i].used == 0)
    {
        return NULL;
    }
    return &bleRelayList[i].bleInfo;
}

/**************************************************
@bref       蓝牙断连侦测
@param
@return
@note
**************************************************/

static void bleDiscDetector(void)
{
    uint8_t ind;
    uint32_t tick;
    char debug[20];
    if (sysparam.bleAutoDisc == 0)
    {
        return;
    }
    for (ind = 0; ind < BLE_CONNECT_LIST_SIZE; ind++)
    {
        if (bleRelayList[ind].used == 0)
        {
            return;
        }
        if (bleRelayList[ind].bleInfo.bleLost == 1)
        {
            return;
        }
        tick = sysinfo.sysTick - bleRelayList[ind].bleInfo.updateTick;
        if (tick >= (sysparam.bleAutoDisc * 60))
        {
            bleRelayList[ind].bleInfo.bleLost = 1;
            alarmRequestSet(ALARM_BLE_LOST_REQUEST);
            byteToHexString(bleRelayList[ind].addr, debug, 6);
            debug[12] = 0;
            LogPrintf(DEBUG_ALL, "oh ,BLE [%s] lost", debug);
        }
    }
}

/**************************************************
@bref       周期任务
@param
@return
@note
**************************************************/

void blePeriodTask(void)
{
    static uint8_t runTick = 0;
    if (runTick++ >= 25)
    {
        runTick = 0;
        bleRelaySetAllReq(BLE_EVENT_GET_OUTV | BLE_EVENT_GET_RFV);
    }
    bleDiscDetector();
}

/**************************************************
@bref       数据发送控制
@param
@return
@note
**************************************************/

void bleRelaySendDataTry(void)
{
    static uint8_t ind = 0;
    uint8_t param[20], paramLen;
    uint16_t year;
    uint8_t  month;
    uint8_t date;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint16_t value16;
    uint32_t event;
    deviceConnInfo_s *devInfo;
    ind = ind % BLE_CONNECT_LIST_SIZE;
    for (; ind  < BLE_CONNECT_LIST_SIZE; ind++)
    {
        event = bleRelayList[ind].dataReq;
        if (event != 0 && bleRelayList[ind].used)
        {
            devInfo = bleDevGetInfo(bleRelayList[ind].addr);
            if (devInfo == NULL)
            {
                LogMessage(DEBUG_ALL, "Not find devinfo");
                continue;
            }
            if (devInfo->notifyDone && devInfo->connHandle != INVALID_CONNHANDLE)
            {
                LogPrintf(DEBUG_ALL, "bleRelaySendDataTry[%d],Handle:(%d)", ind, devInfo->connHandle);

                if (event & BLE_EVENT_SET_RTC)
                {
                    portGetRtcDateTime(&year, &month, &date, &hour, &minute, &second);
                    param[0] = year % 100;
                    param[1] = month;
                    param[2] = date;
                    param[3] = hour;
                    param[4] = minute;
                    param[5] = second;
                    LogMessage(DEBUG_ALL, "try to update RTC");
                    bleRelaySendProtocol(devInfo->connHandle, devInfo->charHandle, CMD_SET_RTC, param, 6);
                    break;
                }

                //固定发送组
                if (event & BLE_EVENT_GET_OUTV)
                {
                    LogMessage(DEBUG_ALL, "try to get outside voltage");
                    bleRelaySendProtocol(devInfo->connHandle, devInfo->charHandle, CMD_GET_OUTV, param, 0);
                    break;
                }
                if (event & BLE_EVENT_GET_RFV)
                {
                    LogMessage(DEBUG_ALL, "try to get rf voltage");
                    bleRelaySendProtocol(devInfo->connHandle, devInfo->charHandle, CMD_GET_ADCV, param, 0);
                    break;
                }

                //非固定发送组
                if (event & BLE_EVENT_SET_DEVON)
                {
                    LogMessage(DEBUG_ALL, "try to set relay on");
					createEncrypt(bleRelayList[ind].addr, param, &paramLen);
                    bleRelaySendProtocol(devInfo->connHandle, devInfo->charHandle, CMD_DEV_ON, param, paramLen);
                    break;
                }
                if (event & BLE_EVENT_SET_DEVOFF)
                {
                    LogMessage(DEBUG_ALL, "try to set relay off");
					createEncrypt(bleRelayList[ind].addr, param, &paramLen);
                    bleRelaySendProtocol(devInfo->connHandle, devInfo->charHandle, CMD_DEV_OFF, param, paramLen);
                    break;
                }
                if (event & BLE_EVENT_CLR_CNT)
                {
                    LogMessage(DEBUG_ALL, "try to clear shield");
                    bleRelaySendProtocol(devInfo->connHandle, devInfo->charHandle, CMD_CLEAR_SHIELD_CNT, param, 0);
                    break;
                }
                if (event & BLE_EVENT_SET_RF_THRE)
                {
                    LogMessage(DEBUG_ALL, "try to set shield voltage");
                    param[0] = sysparam.bleRfThreshold;
                    bleRelaySendProtocol(devInfo->connHandle, devInfo->charHandle, CMD_SET_VOLTAGE, param, 1);
                    break;
                }
                if (event & BLE_EVENT_SET_OUTV_THRE)
                {
                    LogMessage(DEBUG_ALL, "try to set accoff voltage");
                    value16 = sysparam.bleOutThreshold;
                    param[0] = value16 >> 8 & 0xFF;
                    param[1] = value16 & 0xFF;
                    bleRelaySendProtocol(devInfo->connHandle, devInfo->charHandle, CMD_SET_OUTVOLTAGE, param, 2);
                    break;
                }
                if (event & BLE_EVENT_SET_AD_THRE)
                {
                    LogMessage(DEBUG_ALL, "try to set auto disconnect param");
                    param[0] = sysparam.bleAutoDisc;
                    bleRelaySendProtocol(devInfo->connHandle, devInfo->charHandle, CMD_AUTODIS, param, 1);
                    break;
                }
                if (event & BLE_EVENT_CLR_ALARM)
                {
                    LogMessage(DEBUG_ALL, "try to clear alarm");
                    bleRelaySendProtocol(devInfo->connHandle, devInfo->charHandle, CMD_CLEAR_ALARM, param, 1);
                    break;
                }

                if (event & BLE_EVENT_GET_RF_THRE)
                {
                    LogMessage(DEBUG_ALL, "try to get rf threshold");
                    bleRelaySendProtocol(devInfo->connHandle, devInfo->charHandle, CMD_GET_VOLTAGE, param, 0);
                    break;
                }

                if (event & BLE_EVENT_GET_OUT_THRE)
                {
                    LogMessage(DEBUG_ALL, "try to get out threshold");
                    bleRelaySendProtocol(devInfo->connHandle, devInfo->charHandle, CMD_GET_OUTVOLTAGE, param, 0);
                    break;
                }
                if (event & BLE_EVENT_CLR_PREALARM)
                {
                    LogMessage(DEBUG_ALL, "try to clear prealarm");
                    bleRelaySendProtocol(devInfo->connHandle, devInfo->charHandle, CMD_CLEAR_PREALARM, param, 1);
                    break;
                }
                if (event & BLE_EVENT_SET_PRE_PARAM)
                {
                    LogMessage(DEBUG_ALL, "try to set preAlarm param");
                    param[0] = sysparam.blePreShieldVoltage;
                    param[1] = sysparam.blePreShieldDetCnt;
                    param[2] = sysparam.blePreShieldHoldTime;
                    bleRelaySendProtocol(devInfo->connHandle, devInfo->charHandle, CMD_SET_PRE_ALARM_PARAM, param, 3);
                    break;
                }
                if (event & BLE_EVENT_GET_PRE_PARAM)
                {
                    LogMessage(DEBUG_ALL, "try to get preAlarm param");
                    bleRelaySendProtocol(devInfo->connHandle, devInfo->charHandle, CMD_GET_PRE_ALARM_PARAM, param, 0);
                    break;
                }
                if (event & BLE_EVENT_GET_AD_THRE)
                {
                    LogMessage(DEBUG_ALL, "try to get auto disc param");
                    bleRelaySendProtocol(devInfo->connHandle, devInfo->charHandle, CMD_GET_DISCONNECT_PARAM, param, 0);
                    break;
                }
            }

        }
    }
    if (ind != BLE_CONNECT_LIST_SIZE)
    {
        ind++;
    }
}
/**************************************************
@bref       蓝牙发送协议
@param
    cmd     指令类型
    data    数据
    data_len数据长度
@return
@note
**************************************************/

static void bleRelaySendProtocol(uint16_t connHandle, uint16_t charHandle, uint8_t cmd, uint8_t *data, uint8_t data_len)
{
    unsigned char i, size_len, lrc;
    //char message[50];
    uint8_t ret;
    char mcu_data[32];
    size_len = 0;
    mcu_data[size_len++] = 0x0c;
    mcu_data[size_len++] = data_len + 1;
    mcu_data[size_len++] = cmd;
    i = 3;
    if (data_len > 0 && data == NULL)
    {
        return;
    }
    while (data_len)
    {
        mcu_data[size_len++] = *data++;
        i++;
        data_len--;
    }
    lrc = 0;
    for (i = 1; i < size_len; i++)
    {
        lrc += mcu_data[i];
    }
    mcu_data[size_len++] = lrc;
    mcu_data[size_len++] = 0x0d;
    //changeByteArrayToHexString((uint8_t *)mcu_data, (uint8_t *)message, size_len);
    //message[size_len * 2] = 0;
    //LogPrintf(DEBUG_ALL, "ble send :%s", message);
    //bleClientSendData(0, 0, (uint8_t *) mcu_data, size_len);
    ret = bleCentralSend(connHandle, charHandle, mcu_data, size_len);

    switch (ret)
    {
        case bleTimeout:
            LogMessage(DEBUG_ALL, "bleTimeout");
            bleCentralDisconnect(connHandle);
            break;
    }
}



/**************************************************
@bref       协议解析
@param
    data
    len
@return
@note       0C 04 80 09 04 E7 78 0D
**************************************************/

void bleRelayRecvParser(uint8_t connHandle, uint8_t *data, uint8_t len)
{
    uint8_t readInd, size, crc, i, ind = BLE_CONNECT_LIST_SIZE;
    uint8_t *addr;
    char debug[20];
    uint16_t value16;
    float valuef;
    if (len <= 5)
    {
        return;
    }
    addr = bleDevGetAddrByHandle(connHandle);

    if (addr == NULL)
    {
        return;
    }

    for (i = 0; i < BLE_CONNECT_LIST_SIZE; i++)
    {
        if (tmos_memcmp(addr, bleRelayList[i].addr, 6) == TRUE)
        {
            ind = i;
            break;
        }
    }
    if (ind == BLE_CONNECT_LIST_SIZE)
    {
        return;
    }


    for (readInd = 0; readInd < len; readInd++)
    {
        if (data[readInd] != 0x0C)
        {
            continue;
        }
        if (readInd + 4 >= len)
        {
            //内容超长了
            break;
        }
        size = data[readInd + 1];
        if (readInd + 3 + size >= len)
        {
            continue;
        }
        if (data[readInd + 3 + size] != 0x0D)
        {
            continue;
        }
        crc = 0;
        for (i = 0; i < (size + 1); i++)
        {
            crc += data[readInd + 1 + i];
        }
        if (crc != data[readInd + size + 2])
        {
            continue;
        }
        //LogPrintf(DEBUG_ALL, "CMD[0x%02X]", data[readInd + 3]);
        /*状态更新*/
        bleRelayList[ind].bleInfo.updateTick = sysinfo.sysTick;
        if (bleRelayList[ind].bleInfo.bleLost == 1)
        {
            alarmRequestSet(ALARM_BLE_RESTORE_REQUEST);
            byteToHexString(bleRelayList[ind].addr, debug, 6);
            debug[12] = 0;
            LogPrintf(DEBUG_ALL, "BLE %s restore", debug);
        }
        bleRelayList[ind].bleInfo.bleLost = 0;
        switch (data[readInd + 3])
        {
            case CMD_GET_SHIELD_CNT:
                value16 = data[readInd + 4];
                value16 = value16 << 8 | data[readInd + 5];
                LogPrintf(DEBUG_ALL, "BLE==>shield occur cnt %d", value16);
                break;
            case CMD_CLEAR_SHIELD_CNT:
                LogMessage(DEBUG_ALL, "BLE==>clear success");
                bleRelayClearReq(ind, BLE_EVENT_CLR_CNT);
                break;
            case CMD_DEV_ON:
                if (data[readInd + 4] == 1)
                {
                    LogMessage(DEBUG_ALL, "BLE==>relayon success");
                    alarmRequestSet(ALARM_OIL_CUTDOWN_REQUEST);
                    instructionRespone("relayon success");
                }
                else
                {
                    LogMessage(DEBUG_ALL, "BLE==>relayon fail");
                    instructionRespone("relayon fail");
                }
                bleRelayClearReq(ind, BLE_EVENT_SET_DEVON);
                break;
            case CMD_DEV_OFF:
                if (data[readInd + 4] == 1)
                {
                    LogMessage(DEBUG_ALL, "BLE==>relayoff success");
                    alarmRequestSet(ALARM_OIL_RESTORE_REQUEST);
                    instructionRespone("relayoff success");
                }
                else
                {
                    LogMessage(DEBUG_ALL, "BLE==>relayoff fail");
                    instructionRespone("relayoff fail");
                }
                bleRelayClearReq(ind, BLE_EVENT_SET_DEVOFF);
                break;
            case CMD_SET_VOLTAGE:
                LogMessage(DEBUG_ALL, "BLE==>set shiled voltage success");
                bleRelayClearReq(ind, BLE_EVENT_SET_RF_THRE);
                break;
            case CMD_GET_VOLTAGE:

                valuef = data[readInd + 4] / 100.0;
                bleRelayList[ind].bleInfo.rf_threshold = valuef;
                LogPrintf(DEBUG_ALL, "BLE==>get shield voltage range %.2fV", valuef);
                bleRelayClearReq(ind, BLE_EVENT_GET_RF_THRE);
                break;
            case CMD_GET_ADCV:
                value16 = data[readInd + 4];
                value16 = value16 << 8 | data[readInd + 5];
                valuef = value16 / 100.0;
                bleRelayList[ind].bleInfo.rfV = valuef;
                LogPrintf(DEBUG_ALL, "BLE==>shield voltage %.2fV", valuef);
                bleRelayClearReq(ind, BLE_EVENT_GET_RFV);
                break;
            case CMD_SET_OUTVOLTAGE:
                LogMessage(DEBUG_ALL, "BLE==>set acc voltage success");
                bleRelayClearReq(ind, BLE_EVENT_SET_OUTV_THRE);
                break;
            case CMD_GET_OUTVOLTAGE:
                value16 = data[readInd + 4];
                value16 = value16 << 8 | data[readInd + 5];
                valuef = value16 / 100.0;
                bleRelayList[ind].bleInfo.out_threshold = valuef;
                LogPrintf(DEBUG_ALL, "BLE==>get outside voltage range %.2fV", valuef);
                bleRelayClearReq(ind, BLE_EVENT_GET_OUT_THRE);
                break;
            case CMD_GET_OUTV:
                value16 = data[readInd + 4];
                value16 = value16 << 8 | data[readInd + 5];
                valuef = value16 / 100.0;
                bleRelayList[ind].bleInfo.outV = valuef;
                LogPrintf(DEBUG_ALL, "BLE==>outside voltage %.2fV", valuef);
                bleRelayClearReq(ind, BLE_EVENT_GET_OUTV);
                break;
            case CMD_GET_ONOFF:
                LogPrintf(DEBUG_ALL, "BLE==>relay state %d", data[readInd + 4]);
                break;
            case CMD_ALARM:
                LogMessage(DEBUG_ALL, "BLE==>shield alarm occur");
                LogMessage(DEBUG_ALL, "oh, 蓝牙屏蔽报警...");
                alarmRequestSet(ALARM_SHIELD_REQUEST);
                bleRelaySetReq(ind, BLE_EVENT_CLR_ALARM);
                break;
            case CMD_AUTODIS:
                LogMessage(DEBUG_ALL, "BLE==>set auto disconnect success");
                bleRelayClearReq(ind, BLE_EVENT_SET_AD_THRE);
                break;
            case CMD_CLEAR_ALARM:
                LogMessage(DEBUG_ALL, "BLE==>clear alarm success");
                bleRelayClearReq(ind, BLE_EVENT_CLR_ALARM);
                break;
            case CMD_PREALARM:
                LogMessage(DEBUG_ALL, "BLE==>preshield alarm occur");
                LogMessage(DEBUG_ALL, "oh, 蓝牙预警...");
                alarmRequestSet(ALARM_PREWARN_REQUEST);
                bleRelaySetReq(ind, BLE_EVENT_CLR_PREALARM);
                break;
            case CMD_CLEAR_PREALARM:
                LogMessage(DEBUG_ALL, "BLE==>clear prealarm success");
                bleRelayClearReq(ind, BLE_EVENT_CLR_PREALARM);
                break;
            case CMD_SET_PRE_ALARM_PARAM:
                LogMessage(DEBUG_ALL, "BLE==>set prealarm param success");
                bleRelayClearReq(ind, BLE_EVENT_SET_PRE_PARAM);
                break;
            case CMD_GET_PRE_ALARM_PARAM:
                LogPrintf(DEBUG_ALL, "BLE==>get prealarm param [%d,%d,%d] success", data[readInd + 4], data[readInd + 5],
                          data[readInd + 6]);
                bleRelayList[ind].bleInfo.preV_threshold = data[readInd + 4];
                bleRelayList[ind].bleInfo.preDetCnt_threshold = data[readInd + 5];
                bleRelayList[ind].bleInfo.preHold_threshold = data[readInd + 6];
                bleRelayClearReq(ind, BLE_EVENT_GET_PRE_PARAM);
                break;
            case CMD_GET_DISCONNECT_PARAM:
                LogPrintf(DEBUG_ALL, "BLE==>auto disc %d minutes", data[readInd + 4]);
                bleRelayList[ind].bleInfo.disc_threshold = data[readInd + 4];
                bleRelayClearReq(ind, BLE_EVENT_GET_AD_THRE);
                break;
            case CMD_SET_RTC:
                LogMessage(DEBUG_ALL, "BLE==>RTC update success");
                bleRelayClearReq(ind, BLE_EVENT_SET_RTC);
                break;
        }
        readInd += size + 3;
    }
}

