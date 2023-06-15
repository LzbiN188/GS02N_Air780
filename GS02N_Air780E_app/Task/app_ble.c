/*
 * app_ble.c
 *
 *  Created on: Jan 28, 2023
 *      Author: nimo
 */
#include "app_ble.h"

static bleProcess_s bleProcess;


/*写入需要扫描的BLE地址*/
void doWriteCmdInstrtution(uint8_t *msg, uint16_t len)
{
//    ITEM item;
//    uint8_t i, cnt = 0;
//    instructionItemParser(&item, msg, len);
//
//    for(i = 1; i < item.item_cnt; i++)
//    {
//        LogPrintf(DEBUG_ALL, "+DEV:%s", item.item_data[i]);
//        changeHexStringToByteArray(sysparam.scanAddr[i-1], item.item_data[i], 6);
//    }
//
//    sysparam.scanCnt = item.item_cnt - 1;
//    paramSaveAll();
//    LogPrintf(DEBUG_ALL, "+WRITE[%d]: OK\r\n", item.item_cnt - 1);
//
//    appUpdateScanList();
}


/*开锁指令*/


/*
    +DEV: FA8FDAE4C284,0,Bluetooth beacon,1,0,64,0,4,-57, 1
    mac地址，是否丢失，广播名称，类型，解锁请求，电量，锁状态，版本，信号值, 剪锁
       0        1         2      3     4        5      6    7      8      9
*/
void bleUpload(void)
{
    uint8_t i;
    scanList_s *devScanList;

    char debug[20];

    devScanList = getDevScanlist();
    LogPrintf(DEBUG_ALL, "Show List[%d]:", devScanList->devcnt);

    if (devScanList->devcnt == 0)
        return;
    for (i = 0; i < devScanList->devcnt; i++)
    {
        byteToHexString(devScanList->devlist[i].addr, debug, B_ADDR_LEN);
        debug[12] = 0;
        LogPrintf(DEBUG_ALL, "Mac[%d]:%s, rssi:%d, addrtype:%d, type:%d", i, debug,\
        											devScanList->devlist[i].rssi,\
        											devScanList->devlist[i].addrtype,\
        											devScanList->devlist[i].type);
    }
    if (primaryServerIsReady())
    {
        protocolSend(NORMAL_LINK, PROTOCOL_0B, devScanList);
    }

}


/*蓝牙开锁流程*/
void bleChangeFsm(bleProcessFsm_e fsm)
{
    bleProcess.fsm = fsm;
}

static void bleProcessSend(void)
{
    uint8_t cmd[20];
    uint8_t cmdlen;
    if(bleProcess.sendTick++ %3 == 0)
    {
        if(bleProcess.sendTimeout++ > 3)
        {
            bleProcess.sendTimeout = 0;
            bleChangeFsm(BLE_PROCESS_IDLE);
            LogMessage(DEBUG_ALL, "Send data timeout");
            //bleRsponeToNet("Send data timeout");
        }
        else
        {
            if(bleProcess.onoff)
            {
                LogPrintf(DEBUG_ALL, "Send ble cmd:%s", "0C030501090D");
                changeHexStringToByteArray(cmd, "0C030501090D", strlen("0C030501090D"));
                cmdlen = strlen("0C030501090D") / 2;
                appCentralSendData(cmd, 6);

            }
            else
            {
                LogPrintf(DEBUG_ALL, "Send ble cmd:%s", "0C030500080D");
                changeHexStringToByteArray(cmd, "0C030500080D", strlen("0C030500080D"));
                cmdlen = strlen("0C030500080D") / 2;
                appCentralSendData(cmd, 6);
            }
        }
    }
}

static void bleConnTimeout(void)
{
    bleProcess.connTick = 0;
    LogMessage(DEBUG_ALL, "Ble connect timeout");
    //bleRsponeToNet("Connet Timeout");
    bleChangeFsm(BLE_PROCESS_IDLE);
}

static void bleProcessConnect(void)
{
    if(bleProcess.connTick++ >= 20)
    {
        bleProcess.connTick = 0;
        bleConnTimeout();
        return;
    }
    if(sysinfo.linkConn)
    {
        LogMessage(DEBUG_ALL, "Ble connect success");
        bleChangeFsm(BLE_PROCESS_SEND);
        bleProcess.connTick = 0;
    }

}

void bleProcessTask(void)
{
    switch(bleProcess.fsm)
    {
        case BLE_PROCESS_IDLE:
            if(sysinfo.linkConn)
            {
                appCentralTeiminate();
            }
            break;
        case BLE_PROCESS_CONNECT:
            bleProcessConnect();
            break;
        case BLE_PROCESS_SEND:
            bleProcessSend();
            break;
    }
}

void bleChangeToConnect(uint8_t *mac, uint8_t onoff)
{
    char debug[20];
    appCentralEstablish(mac);
    bleChangeFsm(BLE_PROCESS_CONNECT);
    sysinfo.linkRetry = 1;
    bleProcess.connTick = 0;
    bleProcess.onoff = onoff;
    byteToHexString(mac, debug, 6);
    debug[12] = 0;
    LogPrintf(DEBUG_ALL, "Try to connect:%s to %s", debug, onoff ? "Unlock" : "Lock");
    tmos_start_task(appCentralTaskId, APP_LINK_TIMEOUT_EVENT, MS1_TO_SYSTEM_TIME(15000));
}

uint8_t isBleProcessIdle(void)
{
    if(bleProcess.fsm == BLE_PROCESS_IDLE)
        return 1;
    return 0;
}



/*蓝牙锁传回来的数据 字符串*/
static void bleResponeParser(uint8_t cmd, uint8_t *buf, uint8_t len)
{
    int16_t index;
    LogPrintf(DEBUG_ALL, "Cmd:0x%x", cmd);
    switch(cmd)
    {
        case 0x80:
            if(buf[0] == 0x05)
            {
                bleChangeFsm(BLE_PROCESS_IDLE);
                //bleRsponeToNet("Excute OK");
                bleProcess.sendTick = 0;
                bleProcess.sendTimeout = 0;
                LogMessage(DEBUG_ALL, "Excute OK");
            }
            break;
    }
}

uint8_t bleProtocolParser(uint8_t *src, uint8_t len)
{
    uint8_t i, crc;
    //开头匹配
    if(src[0] != 0x0c)
    {
        return 0;
    }
    //长度匹配
    if(src[1] + 3 != len)
    {
        return 0;
    }
    //校验匹配
    crc = 0;
    for(i = 0; i < src[1]; i++)
    {
        crc += src[i + 1];
    }
    if(crc != src[len - 2])
    {
        return 0;
    }
    //校验通过
    bleResponeParser(src[2], src + 3, src[1] - 2);
    return 1;
}



