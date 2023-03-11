/*
 * app_update.c
 *
 *  Created on: 2021年10月16日
 *      Author: idea
 */
#include "app_update.h"
#include "app_param.h"
#include "app_sys.h"
#include "app_kernal.h"
#include "app_net.h"
#include "app_port.h"
#include "app_flash.h"
#include "app_socket.h"


static netConnect_s netconnect;
static upgradeInfo_s uis;
static int8_t timeOutId = -1;

static void upgradeFsmChange(networkFsm_e state);

/**************************************************
@bref		创建协议头部
@param
@return
@note
**************************************************/

static int createHead7878(char *dest, unsigned char Protocol_type)
{
    int pdu_len;
    pdu_len = 0;
    if (dest == NULL)
    {
        return 0;
    }
    dest[pdu_len++] = 0x78;
    dest[pdu_len++] = 0x78;
    dest[pdu_len++] = 0;
    dest[pdu_len++] = Protocol_type;
    return pdu_len;
}

/**************************************************
@bref		创建协议头部
@param
@return
@note
**************************************************/

static int createHead7979(char *dest, unsigned char Protocol_type)
{
    int pdu_len;

    pdu_len = 0;

    if (dest == NULL)
    {
        return 0;
    }

    dest[pdu_len++] = 0x79;
    dest[pdu_len++] = 0x79;
    /* len = 0 First */
    dest[pdu_len++] = 0;
    dest[pdu_len++] = 0;

    dest[pdu_len++] = Protocol_type;
    return pdu_len;
}

/**************************************************
@bref		创建协议尾部
@param
@return
@note
**************************************************/

static int createTail7878(char *dest, int h_b_len, int serial_no)
{
    int pdu_len;
    unsigned short crc_16;

    if (dest == NULL)
    {
        return 0;
    }
    pdu_len = h_b_len;
    dest[pdu_len++] = (serial_no >> 8) & 0xff;
    dest[pdu_len++] = serial_no & 0xff;
    dest[2] = pdu_len - 1;
    /* Caculate Crc */
    crc_16 = GetCrc16(dest + 2, pdu_len - 2);
    dest[pdu_len++] = (crc_16 >> 8) & 0xff;
    dest[pdu_len++] = (crc_16 & 0xff);
    dest[pdu_len++] = 0x0d;
    dest[pdu_len++] = 0x0a;

    return pdu_len;
}

/**************************************************
@bref		创建协议尾部
@param
@return
@note
**************************************************/

static int createTail7979(char *dest, int h_b_len, int serial_no)
{
    int pdu_len;
    unsigned short crc_16;

    if (dest == NULL)
    {
        return -1;
    }
    pdu_len = h_b_len;
    dest[pdu_len++] = (serial_no >> 8) & 0xff;
    dest[pdu_len++] = serial_no & 0xff;
    dest[2] = ((pdu_len - 2) >> 8) & 0xff;
    dest[3] = ((pdu_len - 2) & 0xff);
    crc_16 = GetCrc16(dest + 2, pdu_len - 2);
    dest[pdu_len++] = (crc_16 >> 8) & 0xff;
    dest[pdu_len++] = (crc_16 & 0xff);
    dest[pdu_len++] = 0x0d;
    dest[pdu_len++] = 0x0a;
    return pdu_len;
}


/**************************************************
@bref		打包IMEI
@param
@return
@note
**************************************************/

static int packIMEI(char *IMEI, char *dest)
{
    int imei_len;
    int i;
    if (IMEI == NULL || dest == NULL)
    {
        return -1;
    }
    imei_len = strlen(IMEI);
    if (imei_len == 0)
    {
        return -2;
    }
    if (imei_len % 2 == 0)
    {
        for (i = 0; i < imei_len / 2; i++)
        {
            dest[i] = ((IMEI[i * 2] - '0') << 4) | (IMEI[i * 2 + 1] - '0');
        }
    }
    else
    {
        for (i = 0; i < imei_len / 2; i++)
        {
            dest[i + 1] = ((IMEI[i * 2 + 1] - '0') << 4) | (IMEI[i * 2 + 2] - '0');
        }
        dest[0] = (IMEI[0] - '0');
    }
    return (imei_len + 1) / 2;
}

/**************************************************
@bref		创建登录协议
@param
@return
@note
**************************************************/

static int createProtocol_01(char *IMEI, unsigned short Serial, char *DestBuf)
{
    int pdu_len;
    int ret;
    pdu_len = createHead7878(DestBuf, 0x01);
    ret = packIMEI(IMEI, DestBuf + pdu_len);
    if (ret < 0)
    {
        return -1;
    }
    pdu_len += ret;
    ret = createTail7878(DestBuf, pdu_len,  Serial);
    if (ret < 0)
    {
        return -2;
    }
    pdu_len = ret;
    return pdu_len;
}

/**************************************************
@bref		基本升级协议
@param
@return
@note
**************************************************/

static int createProtocol_F3(char *IMEI, unsigned short Serial, char *DestBuf, uint8_t cmd)
{
    int pdu_len;
    uint32_t readfilelen;
    pdu_len = createHead7979(DestBuf, 0xF3);
    //命令
    DestBuf[pdu_len++] = cmd;
    if (cmd == 0x01)
    {
        //SN号长度
        DestBuf[pdu_len++] = strlen(IMEI);
        //拷贝SN号
        memcpy(DestBuf + pdu_len, IMEI, strlen(IMEI));
        pdu_len += strlen(IMEI);
        //版本号长度
        DestBuf[pdu_len++] = strlen(uis.curCODEVERSION);
        //拷贝SN号
        memcpy(DestBuf + pdu_len, uis.curCODEVERSION, strlen(uis.curCODEVERSION));
        pdu_len += strlen(uis.curCODEVERSION);
    }
    else if (cmd == 0x02)
    {
        DestBuf[pdu_len++] = (uis.file_id >> 24) & 0xFF;
        DestBuf[pdu_len++] = (uis.file_id >> 16) & 0xFF;
        DestBuf[pdu_len++] = (uis.file_id >> 8) & 0xFF;
        DestBuf[pdu_len++] = (uis.file_id) & 0xFF;

        //文件偏移位置
        DestBuf[pdu_len++] = (uis.rxfileOffset >> 24) & 0xFF;
        DestBuf[pdu_len++] = (uis.rxfileOffset >> 16) & 0xFF;
        DestBuf[pdu_len++] = (uis.rxfileOffset >> 8) & 0xFF;
        DestBuf[pdu_len++] = (uis.rxfileOffset) & 0xFF;

        readfilelen = uis.file_totalsize - uis.rxfileOffset; //得到剩余未接收大小
        if (readfilelen > uis.file_len)
        {
            readfilelen = uis.file_len;
        }
        //文件读取长度
        DestBuf[pdu_len++] = (readfilelen >> 24) & 0xFF;
        DestBuf[pdu_len++] = (readfilelen >> 16) & 0xFF;
        DestBuf[pdu_len++] = (readfilelen >> 8) & 0xFF;
        DestBuf[pdu_len++] = (readfilelen) & 0xFF;
    }
    else if (cmd == 0x03)
    {
        DestBuf[pdu_len++] = uis.updateOK;
        //SN号长度
        DestBuf[pdu_len++] = strlen(IMEI);
        //拷贝SN号
        memcpy(DestBuf + pdu_len, IMEI, strlen(IMEI));
        pdu_len += strlen(IMEI);

        DestBuf[pdu_len++] = (uis.file_id >> 24) & 0xFF;
        DestBuf[pdu_len++] = (uis.file_id >> 16) & 0xFF;
        DestBuf[pdu_len++] = (uis.file_id >> 8) & 0xFF;
        DestBuf[pdu_len++] = (uis.file_id) & 0xFF;

        //版本号长度
        DestBuf[pdu_len++] = strlen(uis.newCODEVERSION);
        //拷贝SN号
        memcpy(DestBuf + pdu_len, uis.newCODEVERSION, strlen(uis.newCODEVERSION));
        pdu_len += strlen(uis.newCODEVERSION);
    }
    else
    {
        return 0;
    }
    pdu_len = createTail7979(DestBuf, pdu_len, Serial);
    return pdu_len;
}

/**************************************************
@bref		登录回复
@param
@return
@note
**************************************************/
static void protoclparase01(char *protocol, int size)
{
    netconnect.loginCount = 0;
    upgradeFsmChange(NETWORK_LOGIN_READY);
    LogMessage(DEBUG_ALL, "登录成功");
}

/**************************************************
@bref		升级包内容回复
@param
@return
@note
F3 协议解析

命令01回复：79790042F3
01 命令
01 是否需要更新
00000174  文件ID
0004EB34  文件总大小
0F        SN长度
363930323137303931323030303237   SN号
0B        当前版本长度
5632303230373134303031
16        新版本号长度
4137575F353030333030303330385F3131355F34374B
000D   序列号
AD3B   校验
0D0A   结束

命令02回复：79790073F3
02 命令
01 升级有效标志
00000000   文件偏移起始
00000064   文件大小
474D3930312D3031303030344542314333374330304331457F454C4601010100000000000000000002002800010000006C090300E4E9040004EA04000200000534002000010028000700060038009FE510402DE924108FE2090080E002A300EB24109FE5
0003  序列号
27C6  校验
0D0A  借宿

**************************************************/

static void protoclparaseF3(uint8_t *protocol, int size)
{
    int8_t ret;
    uint8_t cmd, snlen, myversionlen, newversionlen;
    uint16_t index, filecrc, calculatecrc;
    uint32_t rxfileoffset, rxfilelen;
    uint8_t *codedata;
    cmd = protocol[5];
    if (cmd == 0x01)
    {
        //判断是否有更新文件
        if (protocol[6] == 0x01)
        {
            uis.file_id = (protocol[7] << 24 | protocol[8] << 16 | protocol[9] << 8 | protocol[10]);
            uis.file_totalsize = (protocol[11] << 24 | protocol[12] << 16 | protocol[13] << 8 | protocol[14]);

            snlen = protocol[15];
            index = 16;
            if (snlen > (sizeof(uis.rxsn) - 1))
            {
                LogPrintf(DEBUG_ALL, "Sn too long %d", snlen);
                return ;
            }
            strncpy(uis.rxsn, (char *)&protocol[index], snlen);
            uis.rxsn[snlen] = 0;
            index = 16 + snlen;
            myversionlen = protocol[index];
            index += 1;
            if (myversionlen > (sizeof(uis.rxcurCODEVERSION) - 1))
            {
                LogPrintf(DEBUG_ALL, "myversion too long %d", myversionlen);
                return ;
            }
            strncpy(uis.rxcurCODEVERSION, (char *)&protocol[index], myversionlen);
            uis.rxcurCODEVERSION[myversionlen] = 0;
            index += myversionlen;
            newversionlen = protocol[index];
            index += 1;
            if (newversionlen > (sizeof(uis.newCODEVERSION) - 1))
            {
                LogPrintf(DEBUG_ALL, "newversion too long %d", newversionlen);
                return ;
            }
            strncpy(uis.newCODEVERSION, (char *)&protocol[index], newversionlen);
            uis.newCODEVERSION[newversionlen] = 0;
            LogPrintf(DEBUG_ALL, "File %08X,Total size=%d Bytes", uis.file_id, uis.file_totalsize);
            LogPrintf(DEBUG_ALL, "My SN:[%s]", uis.rxsn);
            LogPrintf(DEBUG_ALL, "My Ver:[%s]", uis.rxcurCODEVERSION);
            LogPrintf(DEBUG_ALL, "New Ver:[%s]", uis.newCODEVERSION);
            upgradeFsmChange(NETWORK_DOWNLOAD_DOING);
            if (uis.rxfileOffset == 0)
            {
                /*
                 * 代码擦除
                 */
                flashEarseByFileSize(APPLICATION_ADDRESS, uis.file_totalsize);
            }
            else
            {
                LogMessage(DEBUG_ALL, "Update firmware continute");
            }
        }
        else
        {
            LogMessage(DEBUG_ALL, "No update file");
            paramSaveUpdateStatus(0);
            startTimer(1000, startJumpToApp, 1);
        }
    }
    else if (cmd == 0x02)
    {
        if (protocol[6] == 1)
        {
            rxfileoffset = (protocol[7] << 24 | protocol[8] << 16 | protocol[9] << 8 | protocol[10]); //文件偏移
            rxfilelen = (protocol[11] << 24 | protocol[12] << 16 | protocol[13] << 8 | protocol[14]); //文件大小
            calculatecrc = GetCrc16(protocol + 2, size - 6); //文件校验
            filecrc = (*(protocol + 15 + rxfilelen + 2) << 8) | (*(protocol + 15 + rxfilelen + 2 + 1));
            if (rxfileoffset < uis.rxfileOffset)
            {
                LogMessage(DEBUG_ALL, "Receive the same firmware");
                upgradeFsmChange(NETWORK_DOWNLOAD_DOING);
                return ;
            }
            if (calculatecrc == filecrc)
            {
                LogMessage(DEBUG_ALL, "Data validation OK,Writting...");
                codedata = protocol + 15;
                /*
                 * 代码写入
                 */
                ret = flashWriteCode(APPLICATION_ADDRESS + rxfileoffset, (uint8_t *)codedata, rxfilelen);
                if (ret == 1)
                {
                    uis.rxfileOffset = rxfileoffset + rxfilelen;
                    LogPrintf(DEBUG_ALL, ">>>>>>>>>> Completed progress %.2f%% <<<<<<<<<",
                              (uis.rxfileOffset * 100.0 / uis.file_totalsize));


                    if (uis.rxfileOffset == uis.file_totalsize)
                    {
                        uis.updateOK = 1;
                        upgradeFsmChange(NETWORK_DOWNLOAD_DONE);
                    }
                    else if (uis.rxfileOffset > uis.file_totalsize)
                    {
                        LogMessage(DEBUG_ALL, "Recevie complete ,but total size is different,retry again");
                        uis.rxfileOffset = 0;
                        upgradeFsmChange(NETWORK_LOGIN);
                    }
                    else
                    {
                        upgradeFsmChange(NETWORK_DOWNLOAD_DOING);

                    }
                }
                else
                {
                    LogPrintf(DEBUG_ALL, "Writing firmware error at 0x%X", APPLICATION_ADDRESS + uis.rxfileOffset);
                    upgradeFsmChange(NETWORK_DOWNLOAD_ERROR);
                }

            }
            else
            {
                LogMessage(DEBUG_ALL, "Data validation Fail");
                upgradeFsmChange(NETWORK_DOWNLOAD_DOING);
            }
        }
        else
        {
            LogMessage(DEBUG_ALL, "未知\n");

        }
    }
}

/**************************************************
@bref		协议解析
@param
@return
@note
**************************************************/

void protocolRecvParser(char *protocol, int size)
{
    if (protocol[0] == 0X78 && protocol[1] == 0X78)
    {
        switch (protocol[3])
        {
            case (uint8_t)0x01:
                protoclparase01(protocol, size);
                break;
        }
    }
    else if (protocol[0] == 0X79 && protocol[1] == 0X79)
    {
        switch ((uint8_t)protocol[4])
        {
            case 0xF3:
                protoclparaseF3(protocol, size);
                break;
        }
    }
    else
    {
        LogMessage(DEBUG_ALL, "protocolRecvParser:Error");
    }
}


/**************************************************
@bref		创建序列号
@param
@return
@note
**************************************************/

static unsigned short createProtocolSerial(void)
{
    return netconnect.serial++;
}


/**************************************************
@bref		协议发送
@param
@return
@note
**************************************************/

void protocolSend(protocolType_e protocol, void *param)
{
    char txdata[384];
    char senddata[768];
    int txlen;
    switch (protocol)
    {
        case PROTOCOL_01:
            txlen = createProtocol_01((char *)sysparam.SN, createProtocolSerial(), txdata);
            break;
        case PROTOCOL_F3:
            txlen = createProtocol_F3((char *)sysparam.SN, createProtocolSerial(), txdata, *(uint8_t *)param);
            break;
        default:
            break;
    }
    switch (protocol)
    {
        default:
            socketSendData(NORMAL_LINK, (uint8_t *)txdata, txlen);
            memset(senddata, 0, sizeof(senddata));
            byteToHexString((uint8_t *)txdata, (uint8_t *)senddata, txlen);
            LogMessage(DEBUG_ALL, "TCP Send:");
            LogMessage(DEBUG_ALL, senddata);
            break;
    }

}


/**************************************************
@bref		升级状态机切换
@param
@return
@note
**************************************************/

static void upgradeFsmChange(networkFsm_e state)
{
    netconnect.fsmstate = state;
    netconnect.runtick = 0;
}
/**************************************************
@bref		数据接收
@param
@return
@note
**************************************************/

static void upgradeServerRecv(char *data, uint16_t len)
{
    protocolRecvParser(data, len);
}

/**************************************************
@bref		断开连接
@param
@return
@note
**************************************************/

void upgradeReconnect(void)
{
    socketDel(NORMAL_LINK);
}


/**************************************************
@bref       模组回复，停止定时器，防止模组死机用
@param
@return
@note
**************************************************/

void moduleRspSuccess(void)
{
    if (timeOutId != -1)
    {
        stopTimer(timeOutId);
        timeOutId = -1;
    }
}

/**************************************************
@bref       执行复位模组
@param
@return
@note
**************************************************/

void moduleRspTimeout(void)
{
    timeOutId = -1;
    moduleReset();
}

static void timeOutCheck(void)
{
    if (timeOutId == -1)
    {
        timeOutId = startTimer(10000, moduleRspTimeout, 0);
    }

}

/**************************************************
@bref		升级任务
@param
@return
@note
**************************************************/

void upgradeRunTask(void)
{
    uint8_t cmd;
    int8_t ret;
    if (isModuleRunNormal() == 0)
    {
        return ;
    }

    if (socketGetUsedFlag(NORMAL_LINK) != 1)
    {
        upgradeFsmChange(NETWORK_LOGIN);
        ret=socketAdd(NORMAL_LINK, sysparam.updateServer, sysparam.updatePort, upgradeServerRecv);
        if (ret != 1)
        {
            sysparam.updateStatus = 0;
            paramSaveAll();
            portSysReset();
        }
        return;
    }
    if (socketGetConnStatus(NORMAL_LINK) != SOCKET_CONN_SUCCESS)
    {
        upgradeFsmChange(NETWORK_LOGIN);
        return;
    }

    switch (netconnect.fsmstate)
    {
        case NETWORK_LOGIN:
            //发送登录,并进入等待状态
            protocolSend(PROTOCOL_01, NULL);
            timeOutCheck();
            upgradeFsmChange(NETWORK_LOGIN_WAIT);
            break;
        case NETWORK_LOGIN_WAIT:
            //等待登录返回，30秒内任未连上，则继续重连
            if (netconnect.runtick > 30)
            {
                netconnect.loginCount++;
                upgradeFsmChange(NETWORK_LOGIN);
                if (netconnect.loginCount >= 3)
                {
                    netconnect.loginCount = 0;
                    upgradeReconnect();
                }
            }
            break;
        case NETWORK_LOGIN_READY:
            //登录后获取新软件版本，未获取，每隔30秒重新获取
            if (netconnect.runtick % 30 == 0)
            {
                cmd = 1;
                protocolSend(PROTOCOL_F3, &cmd);
            timeOutCheck();
                netconnect.getVerCount++;
                if (netconnect.getVerCount >= 3)
                {
                    netconnect.getVerCount = 0;
                    upgradeReconnect();
                }
            }
            break;
        case NETWORK_DOWNLOAD_DOING:
            //发送下载协议,并进入等待状态
            cmd = 2;
            protocolSend(PROTOCOL_F3, &cmd);
            timeOutCheck();
            upgradeFsmChange(NETWORK_DOWNLOAD_WAIT);
            break;
        case NETWORK_DOWNLOAD_WAIT:
            //等下固件下载，超过30秒未收到数据，重新发送下载协议
            LogMessage(DEBUG_ALL, "Waitting firmware data...");
            if (netconnect.runtick > 45)
            {
                upgradeFsmChange(NETWORK_DOWNLOAD_DOING);
            }
            break;
        case NETWORK_DOWNLOAD_DONE:
            //下载写入完成
            cmd = 3;
            protocolSend(PROTOCOL_F3, &cmd);
            LogMessage(DEBUG_ALL, "Download firmware complete!");
            upgradeFsmChange(NETWORK_WAIT_JUMP);
            paramSaveUpdateStatus(0);
            startTimer(1000, startJumpToApp, 1);
            break;
        case NETWORK_DOWNLOAD_ERROR://下载错误，需要处理
            LogMessage(DEBUG_ALL, "Writting firmware error,retry...");
            flashEarseByFileSize(APPLICATION_ADDRESS + uis.rxfileOffset, uis.file_len);
            upgradeFsmChange(NETWORK_DOWNLOAD_DOING);
            break;
        case NETWORK_WAIT_JUMP:
            //18秒任未跳转，则重新下载
            if (netconnect.runtick > 10)
            {
                upgradeFsmChange(NETWORK_LOGIN);

            }
            break;
    }
    netconnect.runtick++;
}


/**************************************************
@bref		准备跳转
@param
@return
@note
**************************************************/

void startJumpToApp(void)
{
    static uint8_t tick = 4;
    LogPrintf(DEBUG_ALL, "startJumpToApp %d ...", tick);
    --tick;
    if (tick == 0)
    {
        modulePowerOff();
        if (JumpToApp(APPLICATION_ADDRESS) == 0)
        {
            LogMessage(DEBUG_ALL, "Jump to app fail");
            paramSaveUpdateStatus(1);
        }
    }
}


/**************************************************
@bref		跳转APP
@param
@return
@note
**************************************************/

uint8_t JumpToApp(uint32_t appaddress)
{
    uint8_t result = 0;
    PFIC_DisableIRQ(TMR0_IRQn);
    jumpApp();
    return result;
}

/**************************************************
@bref		初始化升级信息
@param
@return
@note
**************************************************/

void upgradeInfoInit(void)
{
    char CODEVERSION[50];
    memset(&uis, 0, sizeof(upgradeInfo_s));
    paramGetCodeVersion(CODEVERSION);
    strcpy(uis.curCODEVERSION, CODEVERSION);
    uis.file_len = 1024; //务必保证该值能够被4整除
    LogPrintf(DEBUG_ALL, "Current Version:%s", uis.curCODEVERSION);
}
/**************************************************
@bref       升级服务器链接失败
@param
@return
@note
**************************************************/

void upgradeSocketErr(void)
{
    netconnect.socketErrCount++;
    LogMessage(DEBUG_ALL, "upgrade server connect fail");
    if (netconnect.socketErrCount >= 2)
    {
        if (uis.rxfileOffset == 0)
        {
            LogMessage(DEBUG_ALL, "upgrade abort");
            sysparam.updateStatus = 0;
            paramSaveAll();
            portSysReset();
        }
    }
}
/**************************************************
@bref       升级服务器链接成功
@param
@return
@note
**************************************************/

void upgradeSocketSuccess(void)
{
    netconnect.socketErrCount = 0;
}
