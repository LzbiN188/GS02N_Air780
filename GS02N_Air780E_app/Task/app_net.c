#include <app_protocol.h>
#include "app_net.h"

#include "app_db.h"
#include "app_gps.h"
#include "app_instructioncmd.h"
#include "app_kernal.h"
#include "app_param.h"
#include "app_sys.h"
#include "app_task.h"
#include "app_server.h"
#include "app_socket.h"
#include "app_jt808.h"

//联网相关结构体

static moduleState_s  moduleState;
static moduleCtrl_s moduleCtrl;
static cmdNode_s *headNode = NULL;


static void gpsUploadChangeFsm(uint8_t fsm);
static void gpsUploadSetSize(uint32_t size);

//指令表
const atCmd_s cmdtable[] =
{
    {AT_CMD, "ATE0"},
    {CPIN_CMD, "AT+CPIN"},
    {CSQ_CMD, "AT+CSQ"},
    {CGREG_CMD, "AT+CGREG"},
    {CEREG_CMD, "AT+CEREG"},
    {QICSGP_CMD, "AT+QICSGP"},
    {QIACT_CMD, "AT+QIACT"},
    {QIDEACT_CMD, "AT+QIDEACT"},
    {QIOPEN_CMD, "AT+QIOPEN"},
    {QISEND_CMD, "AT+QISEND"},
    {CIMI_CMD, "AT+CIMI"},
    {CGSN_CMD, "AT+CGSN"},
    {CCID_CMD, "AT+CCID"},
    {ICCID_CMD, "AT+ICCID"},
    {CMGF_CMD, "AT+CMGF"},
    {CMGR_CMD, "AT+CMGR"},
    {CMGD_CMD, "AT+CMGD"},
    {CMGS_CMD, "AT+CMGS"},
    {CPMS_CMD, "AT+CPMS"},
    {CNMI_CMD, "AT+CNMI"},
    {QSCLK_CMD, "AT+QSCLK"},
    {CFUN_CMD, "AT+CFUN"},
    {QICLOSE_CMD, "AT+QICLOSE"},
    {CGDCONT_CMD, "AT+CGDCONT"},
    {QCFG_CMD, "AT+QCFG"},
    {QICFG_CMD, "AT+QICFG"},
    {QPOWD_CMD, "AT+QPOWD"},
    {QGPS_CMD, "AT+QGPS"},
    {QGPSEND_CMD, "AT+QGPSEND"},
    {QGPSCFG_CMD, "AT+QGPSCFG"},
    {QGPSGNMEA_CMD, "AT+QGPSGNMEA"},
    {QISTATE_CMD, "AT+QISTATE"},
    {QAGPS_CMD, "AT+QAGPS"},
    {QURCCFG_CMD, "AT+QURCCFG"},
    {CGATT_CMD, "AT+CGATT"},
    {QIDNSIP_CMD, "AT+QIDNSIP"},
    {QIMUX_CMD, "AT+QIMUX"},
    {QISACK_CMD, "AT+QISACK"},
    {QINDI_CMD, "AT+QINDI"},
    {QIRD_CMD, "AT+QIRD"},
    {ATA_CMD, "ATA"},
    {CBC_CMD, "AT+CBC"},
    {CIPMUX_CMD, "AT+CIPMUX"},
    {CIPSTART_CMD, "AT+CIPSTART"},
    {CIPCLOSE_CMD, "AT+CIPCLOSE"},
    {CIPSEND_CMD, "AT+CIPSEND"},
    {CIPQSEND_CMD, "AT+CIPQSEND"},
    {CIPRXGET_CMD, "AT+CIPRXGET"},
    {CIPACK_CMD, "AT+CIPACK"},
    {CSCLK_CMD, "AT+CSCLK"},
    {CIPSTATUS_CMD, "AT+CIPSTATUS"},
    {CIPSHUT_CMD, "AT+CIPSHUT"},
    {CFGRI_CMD, "AT+CFGRI"},
    {WIFISCAN_CMD, "AT+WIFISCAN"},
    {CFG_CMD, "AT+CFG"},
    {CIICR_CMD, "AT+CIICR"},
    {CIFSR_CMD, "AT+CIFSR"},
    {CSTT_CMD, "AT+CSTT"},
    {CPNETAPN_CMD, "AT+CPNETAPN"},
    {CGAUTH_CMD, "AT+CGAUTH"},

};

/**************************************************
@bref       创建指令输出队列，用于顺序输出
@param
@return
@note
**************************************************/

uint8_t createNode(char *data, uint16_t datalen, uint8_t currentcmd)
{
    cmdNode_s *nextnode;
    cmdNode_s *currentnode;
    //如果链表头未创建，则创建链表头。
    WAKEMODULE;
    if (currentcmd == WIFISCAN_CMD)
    {
        wakeUpByInt(1, 20);
    }
    else
    {
        wakeUpByInt(1, 15);
    }
    if (headNode == NULL)
    {
        headNode = malloc(sizeof(cmdNode_s));
        if (headNode != NULL)
        {
            headNode->currentcmd = currentcmd;
            headNode->data = NULL;
            headNode->data = malloc(datalen);
            if (headNode->data != NULL)
            {
                memcpy(headNode->data, data, datalen);
                headNode->datalen = datalen;
                headNode->nextnode = NULL;
                return 1;
            }
            else
            {
                free(headNode);
                headNode = NULL;
                return 0;
            }
        }
        else
        {
            return 0;
        }
    }
    currentnode = headNode;
    do
    {
        nextnode = currentnode->nextnode;
        if (nextnode == NULL)
        {
            nextnode = malloc(sizeof(cmdNode_s));
            if (nextnode != NULL)
            {

                nextnode->currentcmd = currentcmd;
                nextnode->data = NULL;
                nextnode->data = malloc(datalen);
                if (nextnode->data != NULL)
                {
                    memcpy(nextnode->data, data, datalen);
                    nextnode->datalen = datalen;
                    nextnode->nextnode = NULL;
                    currentnode->nextnode = nextnode;
                    nextnode = nextnode->nextnode;
                }
                else
                {
                    free(nextnode);
                    nextnode = NULL;
                    return 0;
                }
            }
            else
            {
                return 0;
            }
        }
        currentnode = nextnode;
    }
    while (nextnode != NULL);

    return 1;
}

/**************************************************
@bref       数据队列输出
@param
@return
@note
**************************************************/

void outputNode(void)
{
    static uint8_t lockFlag = 0;
    static uint8_t lockTick = 0;
    static uint8_t sleepTick = 0;
    static uint8_t tickRange = 50;
    cmdNode_s *nextnode;
    cmdNode_s *currentnode;
    if (lockFlag)
    {
        if (lockTick++ >= tickRange)
        {
            lockFlag = 0;
            lockTick = 0;
            LogMessage(DEBUG_ALL, "outputNode==>Unlock");
        }
        return ;
    }
    if (headNode == NULL)
    {
        if (sleepTick > 0)
        {
            sleepTick--;
        }
        else
        {
            SLEEPMODULE;
        }
        return ;
    }
    sleepTick = 2;
    currentnode = headNode;
    if (currentnode != NULL)
    {
        nextnode = currentnode->nextnode;
        moduleState.cmd = currentnode->currentcmd;

        //数据发送
        portUartSend(&usart0_ctl, (uint8_t *)currentnode->data, currentnode->datalen);
        if (currentnode->data[0] != 0X78 && currentnode->data[0] != 0x79 && currentnode->data[0] != 0x7E)
        {
            LogMessageWL(DEBUG_ALL, currentnode->data, currentnode->datalen);
        }
        free(currentnode->data);
        free(currentnode);
        if (currentnode->currentcmd == QICLOSE_CMD || currentnode->currentcmd == CMGS_CMD || 
                currentnode->currentcmd == WIFISCAN_CMD)
        {
            lockFlag = 1;
            if (currentnode->currentcmd == QICLOSE_CMD)
            {
                tickRange = 20;
            }
            else if (currentnode->currentcmd == WIFISCAN_CMD)
            {
                tickRange = 75;
            }
            else
            {
                tickRange = 10;
            }
            LogMessage(DEBUG_ALL, "outputNode==>Lock");
        }
    }
    headNode = nextnode;

}

/**************************************************
@bref       模组指令发送
@param
@return
@note
**************************************************/

uint8_t  sendModuleCmd(uint8_t cmd, char *param)
{
    uint8_t i;
    int16_t cmdtype = -1;
    char sendData[256];
    for (i = 0; i < sizeof(cmdtable) / sizeof(cmdtable[0]); i++)
    {
        if (cmd == cmdtable[i].cmd_type)
        {
            cmdtype = i;
            break;
        }
    }
    if (cmdtype < 0)
    {
        snprintf(sendData, 255, "sendModuleCmd==>No cmd");
        LogMessage(DEBUG_ALL, sendData);
        return 0;
    }
    if (param != NULL && strlen(param) <= 240)
    {
        if (param[0] == '?')
        {
            snprintf(sendData, 255, "%s?\r\n", cmdtable[cmdtype].cmd);

        }
        else
        {
            snprintf(sendData, 255, "%s=%s\r\n", cmdtable[cmdtype].cmd, param);
        }
    }
    else if (param == NULL)
    {
        snprintf(sendData, 255, "%s\r\n", cmdtable[cmdtype].cmd);
    }
    else
    {
        return 0;
    }
    createNode(sendData, strlen(sendData), cmd);
    return 1;
}

void sendModuleDirect(char *param)
{
    createNode(param, strlen(param), 0);
}

/**************************************************
@bref       初始化模块相关使用结构体
@param
@return
@note
**************************************************/

static void moduleInit(void)
{
    memset(&moduleState, 0, sizeof(moduleState_s));
}

/**************************************************
@bref       是否开机按键
@param
@return
@note
**************************************************/
static void modulePressReleaseKey(void)
{
    PWRKEY_HIGH;
    moduleState.powerState = 1;
    LogPrintf(DEBUG_ALL, "PowerOn Done");
}
/**************************************************
@bref       按下开机按键
@param
@return
@note
**************************************************/

static void modulePressPowerKey(void)
{
    PWRKEY_LOW;
    startTimer(12, modulePressReleaseKey, 0);
}
/**************************************************
@bref       模组开机
@param
@return
@note
**************************************************/

void modulePowerOn(void)
{
    LogMessage(DEBUG_ALL, "modulePowerOn");
    moduleInit();
    sysinfo.moduleRstFlag = 1;
    portUartCfg(APPUSART0, 1, 115200, moduleRecvParser);
    POWER_ON;
    PWRKEY_HIGH;
    RSTKEY_HIGH;
    startTimer(6, modulePressPowerKey, 0);
    moduleState.gpsFileHandle = 1;
    moduleCtrl.scanMode = 0;
    
    socketDelAll();
}
/**************************************************
@bref       释放关机按键
@param
@return
@note
**************************************************/
static void modulePowerOffProcess(void)
{
    PWRKEY_HIGH;
    RSTKEY_HIGH;
    LogMessage(DEBUG_ALL, "modulePowerOff Done");
}
/**************************************************
@bref       模组关机
@param
@return
@note
**************************************************/

void modulePowerOff(void)
{
    LogMessage(DEBUG_ALL, "modulePowerOff");
    moduleInit();
    portUartCfg(APPUSART0, 0, 115200, NULL);
    POWER_OFF;
    RSTKEY_LOW;
    PWRKEY_HIGH;
    startTimer(15, modulePowerOffProcess, 0);
    sysinfo.moduleRstFlag = 1;
    socketDelAll();
}

/**************************************************
@bref       释放复位按键
@param
@return
@note
**************************************************/

static void moduleReleaseRstkey(void)
{
    moduleState.powerState = 1;
    RSTKEY_HIGH;
}
/**************************************************
@bref       模组复位
@param
@return
@note
**************************************************/

void moduleReset(void)
{
    LogMessage(DEBUG_ALL, "moduleReset");
    moduleInit();
    RSTKEY_LOW;
    startTimer(10, modulePowerOn, 0);
    socketDelAll();
}

/**************************************************
@bref       切换联网状态机
@param
@return
@note
**************************************************/
static void changeProcess(uint8_t fsm)
{
    moduleState.fsmState = fsm;
    moduleState.fsmtick = 0;
    if (moduleState.fsmState != NORMAL_STATUS)
    {
        ledStatusUpdate(SYSTEM_LED_NETOK, 0);
    }
}

/**************************************************
@bref       创建socket
@param
@return
@note
**************************************************/

void openSocket(uint8_t link, char *server, uint16_t port)
{
    char param[100];
    sprintf(param, "%d,\"TCP\",\"%s\",%d", link, server, port);
    sendModuleCmd(AT_CMD, NULL);
    sendModuleCmd(CIPSTART_CMD, param);
}

/**************************************************
@bref       关闭socket
@param
@return
@note
**************************************************/

void closeSocket(uint8_t link)
{
    char param[10];
    sprintf(param, "%d", link);
    sendModuleCmd(AT_CMD, NULL);
    sendModuleCmd(CIPCLOSE_CMD, param);
}

/**************************************************
@bref       apn配置
@param
@return
@note
**************************************************/

static void netSetCgdcong(char *apn)
{
    char param[100];
    sprintf(param, "1,\"IP\",\"%s\"", apn);
    sendModuleCmd(CGDCONT_CMD, param);
}

/**************************************************
@bref       apn配置
@param
@return
@note
**************************************************/

static void netSetApn(char *apn, char *apnname, char *apnpassword, uint8_t apnauthport)
{
    char param[100];
    sprintf(param, "1,%d,\"%s\",\"%s\"", apnauthport, apnname, apnpassword);
    sendModuleCmd(CGAUTH_CMD, param);
}

/**************************************************
@bref       apn配置
@param
@return
@note
**************************************************/

static void netSetCstt(char *apn, char *apnname, char *apnpassword)
{
    char param[100];
    sprintf(param, "\"%s\",\"%s\",\"%s\"", apn, apnname, apnpassword);
    sendModuleCmd(CSTT_CMD, param);
}



/**************************************************
@bref       模组进入飞行模式
@param
@return
@note
**************************************************/

static void moduleEnterFly(void)
{
    sendModuleCmd(CFUN_CMD, "0");
}

/**************************************************
@bref       模组进入正常模式
@param
@return
@note
**************************************************/

static void moduleExitFly(void)
{
    sendModuleCmd(CFUN_CMD, "1");
}

/**************************************************
@bref       发送socket读取缓存指令
@param
@return
@note
**************************************************/

static void qirdCmdSend(uint8_t link)
{
    char param[10];
    sprintf(param, "2,%d,500", link);
    moduleState.curQirdId = link;
    sendModuleCmd(CIPRXGET_CMD, param);
}

/**************************************************
@bref       读取缓存
@param
@return
@note
**************************************************/

static void queryRecvBuffer(void)
{
    char param[10];
    if (moduleState.normalLinkQird)
    {
        qirdCmdSend(NORMAL_LINK);
    }
    else if (moduleState.agpsLinkQird)
    {
        qirdCmdSend(AGPS_LINK);
    }
    else if (moduleState.bleLinkQird)
    {
        qirdCmdSend(BLE_LINK);
    }
    else if (moduleState.jt808LinkQird)
    {
        qirdCmdSend(JT808_LINK);
    }
    else if (moduleState.hideLinkQird)
    {
        qirdCmdSend(HIDDEN_LINK);
    }
}

/**************************************************
@bref       联网准备任务
@param
@return
@note
**************************************************/

void netConnectTask(void)
{
    if (moduleState.powerState == 0)
    {
        return;
    }

    moduleState.powerOnTick++;
    switch (moduleState.fsmState)
    {
        case AT_STATUS:
            if (moduleState.atResponOK)
            {
                moduleCtrl.atCount = 0;
                moduleState.atResponOK = 0;
                moduleState.cpinResponOk = 0;
                changeProcess(CPIN_STATUS);
            }
            else
            {
                if (moduleState.fsmtick % 2 == 0)
                {
                    moduleState.powerOnTick = 0;
                    sendModuleCmd(AT_CMD, NULL);
                }
                if (moduleState.fsmtick >= 30)
                {
                    moduleCtrl.atCount++;
                    if (moduleCtrl.atCount >= 2)
                    {
                        moduleCtrl.atCount = 0;
                        modulePowerOn();
                    }
                    else
                    {
                        moduleReset();
                    }
                }
                break;
            }
        case CPIN_STATUS:
            if (moduleState.cpinResponOk)
            {
                moduleState.cpinResponOk = 0;
                moduleState.csqOk = 0;
                netSetCgdcong((char *)sysparam.apn);
                netSetApn((char *)sysparam.apn, (char *)sysparam.apnuser, (char *)sysparam.apnpassword, sysparam.apnAuthPort);
                changeProcess(CSQ_STATUS);

            }
            else
            {
                if (moduleState.fsmtick % 2 == 0)
                {
                    sendModuleCmd(CPIN_CMD, "?");
                }
                if (moduleState.fsmtick >= 30)
                {
                    moduleReset();
                }
                break;
            }
        case CSQ_STATUS:
            if (moduleState.csqOk)
            {
                moduleState.csqOk = 0;
                moduleState.cgregOK = 0;
                moduleCtrl.csqCount = 0;
                sendModuleCmd(CGREG_CMD, "2");
                sendModuleCmd(CEREG_CMD, "2");
                changeProcess(CGREG_STATUS);
                netResetCsqSearch();
            }
            else
            {
                sendModuleCmd(CSQ_CMD, NULL);
                if (moduleState.fsmtick >= 90)
                {
                    moduleCtrl.csqCount++;
                    if (moduleCtrl.csqCount >= 3)
                    {
                        moduleCtrl.csqCount = 0;
                        //3次搜索不到网络时，如果没有gps请求，则关机
                        if (sysinfo.gpsRequest != 0)
                        {
                            moduleReset();
                        }
                        else
                        {
                            modeTryToStop();
                        }
                    }
                    else
                    {
                        moduleEnterFly();
                        startTimer(80, moduleExitFly, 0);
                    }
                    changeProcess(AT_STATUS);
                }
                break;
            }
        case CGREG_STATUS:
            if (moduleState.cgregOK)
            {
                moduleCtrl.cgregCount = 0;
                moduleState.cgregOK = 0;
                sendModuleCmd(CIPSHUT_CMD, NULL);
                changeProcess(CONFIG_STATUS);
            }
            else
            {
                sendModuleCmd(CGREG_CMD, "?");
                sendModuleCmd(CEREG_CMD, "?");
                if (moduleState.fsmtick >= 90)
                {
                    moduleCtrl.cgregCount++;
                    if (moduleCtrl.cgregCount >= 2)
                    {
                        moduleCtrl.cgregCount = 0;
                        //2次注册不上基站时，如果没有gps请求，则关机
                        if (sysinfo.gpsRequest == 0)
                        {
                            modeTryToStop();
                        }
                        else
                        {
                            moduleReset();
                        }

                        LogMessage(DEBUG_ALL, "Register timeout,try to skip");
                    }
                    else
                    {
                        sendModuleCmd(CGATT_CMD, "1");
                        changeProcess(AT_STATUS);
                    }
                }
                break;
            }
        case CONFIG_STATUS:
            sendModuleCmd(CPMS_CMD, "\"ME\",\"ME\",\"ME\"");    /*修改短信存储位置*/
            sendModuleCmd(CNMI_CMD, "2,2");                     /*第二个参数表示缓存在ME中, 不立即上报*/
            sendModuleCmd(CMGF_CMD, "1");                       /*TEXT模式*/
            sendModuleCmd(CFGRI_CMD, "1,50,50,3");
            sendModuleCmd(CIPMUX_CMD, "1");
            sendModuleCmd(CIPQSEND_CMD, "1");
            sendModuleCmd(CIPRXGET_CMD, "5");
            sendModuleCmd(CFG_CMD, "\"urcdelay\",100");
            sendModuleCmd(CIMI_CMD, NULL);
            sendModuleCmd(CGSN_CMD, NULL);
            sendModuleCmd(ICCID_CMD, NULL);
            netSetCstt((char *)sysparam.apn, (char *)sysparam.apnuser, (char *)sysparam.apnpassword);
            sendModuleCmd(CIICR_CMD, NULL);
            changeProcess(QIACT_STATUS);
            break;
        case QIACT_STATUS:
            if (moduleState.qipactOk)
            {
                moduleState.qipactOk = 0;
                moduleCtrl.qipactCount = 0;
                sendModuleCmd(CIFSR_CMD, NULL);
                changeProcess(NORMAL_STATUS);
            }
            else
            {
                if (moduleState.qipactSet == 0)
                {
                    moduleState.qipactSet = 1;
                    sendModuleCmd(CGATT_CMD, "1");
                }
                else
                {
                    sendModuleCmd(CGATT_CMD, "?");
                }
                if (moduleState.fsmtick >= 45)
                {
                    LogMessage(DEBUG_ALL, "try QIPACT again");
                    moduleState.qipactSet = 0;
                    moduleState.fsmtick = 0;
                    moduleCtrl.qipactCount++;
                    if (moduleCtrl.qipactCount >= 3)
                    {
                        moduleCtrl.qipactCount = 0;
                        moduleReset();
                    }
                    else
                    {
                        changeProcess(CPIN_STATUS);
                    }
                }
                break;
            }
        case NORMAL_STATUS:
            socketSchedule();
            queryRecvBuffer();
            break;
        default:
            changeProcess(AT_STATUS);
            break;
    }
    moduleState.fsmtick++;
}


/**************************************************
@bref       AT+CSQ  指令解析
@param
@return
@note
**************************************************/

static void csqParser(uint8_t *buf, uint16_t len)
{
    int index, indexa, datalen;
    uint8_t *rebuf;
    uint16_t  relen;
    char restore[5];
    index = my_getstrindex((char *)buf, "+CSQ:", len);
    if (index >= 0)
    {
        rebuf = buf + index;
        relen = len - index;
        indexa = getCharIndex(rebuf, relen, ',');
        if (indexa > 6)
        {
            datalen = indexa - 6;
            if (datalen > 5)
                return;
            memset(restore, 0, 5);
            strncpy(restore, (char *)rebuf + 6, datalen);
            moduleState.rssi = atoi(restore);
            if (moduleState.rssi >= 6 && moduleState.rssi <= 31)
                moduleState.csqOk = 1;
        }
    }
}

/**************************************************
@bref       AT+CREG 指令解析
@param
@return
@note
**************************************************/

static void cgregParser(uint8_t *buf, uint16_t len)
{
    int index, datalen;
    uint8_t *rebuf;
    uint16_t  relen, i;
    char restore[50];
    uint8_t cnt;
    uint8_t type = 0;
    index = my_getstrindex((char *)buf, "+CGREG:", len);
    if (index < 0)
    {
        type = 1;
        index = my_getstrindex((char *)buf, "+CEREG:", len);
    }
    if (index >= 0)
    {
        rebuf = buf + index;
        relen = len - index;
        datalen = 0;
        cnt = 0;
        restore[0] = 0;
        for (i = 0; i < relen; i++)
        {
            if (rebuf[i] == ',' || rebuf[i] == '\r' || rebuf[i] == '\n')
            {
                if (restore[0] != 0)
                {
                    restore[datalen] = 0;
                    cnt++;
                    datalen = 0;
                    switch (cnt)
                    {
                        case 2:
                            if (restore[0] == '1' || restore[0] == '5')
                            {
                                if (type)
                                {
                                    moduleState.ceregOK = 1;
                                }
                                else
                                {
                                    moduleState.cgregOK = 1;
                                }
                            }
                            else
                            {
                                return ;
                            }
                            break;
                        case 3:

                            moduleState.lac = strtoul(restore + 1, NULL, 16);
                            LogPrintf(DEBUG_ALL, "LAC=%s,0x%X", restore, moduleState.lac);
                            break;
                        case 4:
                            moduleState.cid = strtoul(restore + 1, NULL, 16);
                            LogPrintf(DEBUG_ALL, "CID=%s,0x%X", restore, moduleState.cid);
                            break;
                    }
                    restore[0] = 0;
                }
            }
            else
            {
                restore[datalen] = rebuf[i];
                datalen++;
                if (datalen >= 50)
                {
                    return ;
                }

            }
        }
    }
}

/**************************************************
@bref       AT+CIMI 指令解析
@param
@return
@note
**************************************************/

static void cimiParser(uint8_t *buf, uint16_t len)
{
    int16_t index;
    uint8_t *rebuf;
    uint16_t  relen;
    uint8_t i;
    rebuf = buf;
    relen = len;
    index = getCharIndex(rebuf, relen, '\n');
    if (index < 0)
    {
        return;
    }
    rebuf = rebuf + index + 1;
    relen = relen - index - 1;
    index = getCharIndex(rebuf, relen, '\r');
    if (index == 15)
    {
        for (i = 0; i < index; i++)
        {
            moduleState.IMSI[i] = rebuf[i];
        }
        moduleState.IMSI[index] = 0;
        moduleState.mcc = (moduleState.IMSI[0] - '0') * 100 + (moduleState.IMSI[1] - '0') * 10 + moduleState.IMSI[2] - '0';
        moduleState.mnc = (moduleState.IMSI[3] - '0') * 10 + moduleState.IMSI[4] - '0';
        LogPrintf(DEBUG_ALL, "IMSI:%s,MCC=%d,MNC=%d", moduleState.IMSI, moduleState.mcc, moduleState.mnc);
    }
}


/**************************************************
@bref       AT+ICCID    指令解析
@param
@return
@note
**************************************************/

static void iccidParser(uint8_t *buf, uint16_t len)
{
    int16_t index, indexa;
    uint8_t *rebuf;
    uint16_t  relen;
    uint8_t snlen, i;
    char debug[70];
    index = my_getstrindex((char *)buf, "+ICCID:", len);
    if (index >= 0)
    {
        rebuf = buf + index;
        relen = len - index;
        indexa = getCharIndex(rebuf, relen, '\r');
        if (indexa > 8)
        {
            snlen = indexa - 8;
            if (snlen == 20)
            {
                for (i = 0; i < snlen; i++)
                {
                    moduleState.ICCID[i] = rebuf[i + 8];
                }
                moduleState.ICCID[snlen] = 0;
                sprintf(debug, "ICCID:%s", moduleState.ICCID);
                LogMessage(DEBUG_ALL, debug);
            }
        }
    }
}


/**************************************************
@bref       AT+QIACT    指令解析
@param
@return
@note
**************************************************/

static void qipactParser(uint8_t *buf, uint16_t len)
{
    int index, datalen;
    uint8_t *rebuf;
    uint16_t  relen, i;
    char restore[50];
    uint8_t   cnt;

    index = my_getstrindex((char *)buf, "+QIACT:", len);
    if (index >= 0)
    {
        rebuf = buf + index;
        relen = len - index;
        datalen = 0;
        cnt = 0;
        restore[0] = 0;
        for (i = 0; i < relen; i++)
        {
            if (rebuf[i] == ',' || rebuf[i] == '\r' || rebuf[i] == '\n')
            {
                if (restore[0] != 0)
                {
                    restore[datalen] = 0;
                    cnt++;
                    datalen = 0;
                    switch (cnt)
                    {
                        case 4:
                            if (my_strpach(restore, "\"0.0.0.0\"") == 1)
                            {
                                moduleState.qipactOk = 0;

                            }
                            else if (restore[0] == '"')
                            {
                                moduleState.qipactOk = 1;
                            }
                            else
                            {
                                moduleState.qipactOk = 0;

                            }
                            break;
                    }
                    restore[0] = 0;

                }
            }
            else
            {
                restore[datalen] = rebuf[i];
                datalen++;
                if (datalen >= 50)
                {
                    return ;
                }

            }
        }
    }
}

/**************************************************
@bref       socket数据接收解析
@param
@return
@note
+QIURC: "closed",0
+QIURC: "pdpdeact",1
+QIURC: "recv",1,10
xx\0\0萓

**************************************************/

static void qiurcParser(uint8_t *buf, uint16_t len)
{
    char *rebuf;
    char resbuf[513];
    int index, relen, recvLen;
    int sockId, debugLen;
    rebuf = buf;
    relen = len;
    index = my_getstrindex((char *)rebuf, "+QIURC:", relen);
    if (index < 0)
        return;
    while (index >= 0)
    {
        index += 9;
        rebuf += index;
        relen -= index;
        if (my_strpach(rebuf, "closed"))
        {
            rebuf += 8;
            relen -= 8;
            index = getCharIndex(rebuf, relen, '\r');
            if (index >= 1 && index <= 2)
            {
                memcpy(resbuf, rebuf, index);
                resbuf[index] = 0;
                sockId = atoi(resbuf);
                LogPrintf(DEBUG_ALL, "Socket[%d] Close", sockId);
                socketSetConnState(sockId, SOCKET_CONN_IDLE);
            }
            rebuf += index;
            relen -= index;
        }
        else if (my_strpach(rebuf, "pdpdeact"))
        {
            rebuf += 8;
            relen -= 8;
            LogMessage(DEBUG_ALL, "Socket all closed");
            moduleSleepCtl(0);
            socketResetConnState();
            changeProcess(AT_STATUS);
        }
        else if (my_strpach(rebuf, "recv"))
        {
            rebuf += 6;
            relen -= 6;
            index = getCharIndex(rebuf, relen, ',');
            if (index >= 1 && index <= 2)
            {
                memcpy(resbuf, rebuf, index);
                resbuf[index] = 0;
                sockId = atoi(resbuf);
                LogPrintf(DEBUG_ALL, "Socket[%d] recv data", sockId);
                switch (sockId)
                {
                    case NORMAL_LINK:
                        moduleState.normalLinkQird = 1;
                        break;
                    case BLE_LINK:
                        moduleState.bleLinkQird = 1;
                        break;
                    case JT808_LINK:
                        moduleState.jt808LinkQird = 1;
                        break;
                    case HIDDEN_LINK:
                        moduleState.hideLinkQird = 1;
                        break;
                    case AGPS_LINK:
                        moduleState.agpsLinkQird = 1;
                        break;
                }
            }
        }

        index = my_getstrindex((char *)rebuf, "+QIURC:", relen);
    }
}




/**************************************************
@bref       短信接收
@param
@return
@note
**************************************************/


static void cmtiParser(uint8_t *buf, uint16_t len)
{
    uint8_t i;
    int16_t index;
    uint8_t *rebuf;
    char restore[5];
    index = my_getstrindex((char *)buf, "+CMTI:", len);
    if (index >= 0)
    {
        rebuf = buf + index;
        index = getCharIndex(rebuf, len, ',');
        if (index < 0)
            return;
        rebuf = rebuf + index + 1;
        index = getCharIndex(rebuf, len, '\r');
        if (index > 5 || index < 0)
            return ;
        for (i = 0; i < index; i++)
        {
            restore[i] = rebuf[i];
        }
        restore[index] = 0;
        LogPrintf(DEBUG_ALL, "Message index=%d", atoi(restore));
        sendModuleCmd(CMGR_CMD, restore);
    }
}

/**************************************************
@bref       CMGR    指令解析
@param
@return
@note
**************************************************/

static void cmgrParser(uint8_t *buf, uint16_t len)
{
    int index;
    uint8_t *rebuf;
    uint8_t *numbuf;
    uint16_t  relen, i, renumlen;
    char restore[100];
    insParam_s insparam;
    //找到特定字符串在buf的位置
    index = my_getstrindex((char *)buf, "+CMGR:", len);
    if (index >= 0)
    {
        //得到特定字符串的开始位置和剩余长度
        rebuf = buf + index;
        relen = len - index;
        //识别手机号码
        index = getCharIndexWithNum(rebuf, relen, '"', 3);
        if (index < 0)
            return;
        numbuf = rebuf + index + 1;
        renumlen = relen - index - 1;
        index = getCharIndex(numbuf, renumlen, '"');
        if (index > 100 || index < 0)
            return ;
        for (i = 0; i < index; i++)
        {
            restore[i] = numbuf[i];
        }
        restore[index] = 0;

        if (index > sizeof(moduleState.messagePhone))
            return ;
        strcpy((char *)moduleState.messagePhone, restore);
        LogPrintf(DEBUG_ALL, "Tel:%s", moduleState.messagePhone);
        //得到第一个\n的位置
        index = getCharIndex(rebuf, len, '\n');
        if (index < 0)
            return;
        //偏移到内容处
        rebuf = rebuf + index + 2;
        //得到从内容处开始的第一个\n，测试index就是内容长度
        index = getCharIndex(rebuf, len, '"');
        if (index > 100 || index < 0)
            return ;
        for (i = 0; i < index; i++)
        {
            restore[i] = rebuf[i];
        }
        restore[index] = 0;
        LogPrintf(DEBUG_ALL, "Message:%s", restore);
        insparam.telNum = moduleState.messagePhone;
        //lastparam.telNum = moduleState.messagePhone;
        instructionParser((uint8_t *)restore, index, SMS_MODE, &insparam);
    }
}

/**************************************************
@bref       AT+QISEND   指令解析
@param
@return
@note
+QISEND: 212,212,0

**************************************************/

static void qisendParser(uint8_t *buf, uint16_t len)
{
    int index;
    uint8_t *rebuf;
    uint8_t i, datalen, cnt, sockId;
    uint16_t relen;
    char restore[51];

    index = my_getstrindex((char *)buf, "ERROR", len);
    if (index >= 0)
    {
        //不能很好区分到底是哪条链路出现错误
        socketResetConnState();
        moduleSleepCtl(0);
        changeProcess(CGREG_STATUS);
        return ;
    }
    index = my_getstrindex((char *)buf, "+QISEND:", len);
    if (index < 0)
    {
        return ;
    }

    rebuf = buf + index + 9;
    relen = len - index - 9;
    index = getCharIndex(rebuf, relen, '\n');
    datalen = 0;
    cnt = 0;
    if (index > 0 && index < 50)
    {
        restore[0] = 0;
        for (i = 0; i < index; i++)
        {
            if (rebuf[i] == ',' || rebuf[i] == '\r' || rebuf[i] == '\n')
            {
                if (restore[0] != 0)
                {
                    restore[datalen] = 0;
                    cnt++;
                    datalen = 0;
                    switch (cnt)
                    {
                        case 1:
                            moduleState.tcpTotal = atoi(restore);
                            break;
                        case 2:
                            moduleState.tcpAck = atoi(restore);
                            break;
                        case 3:
                            moduleState.tcpNack = atoi(restore);
                            break;
                    }
                    restore[0] = 0;
                }
            }
            else
            {
                restore[datalen] = rebuf[i];
                datalen++;
                if (datalen >= 50)
                {
                    return ;
                }

            }
        }
    }
}





/**************************************************
@bref       QWIFISCAN   指令解析
@param
@return
@note
+WIFISCAN: "50:0f:f5:51:1a:eb",-31,3
+WIFISCAN: "dc:9f:db:1c:1d:76",-50,11
+WIFISCAN: "ec:41:18:0c:82:09",-63,4
+WIFISCAN: "40:22:30:1a:f8:01",-67,1
+WIFISCAN: "08:6b:d1:0b:50:60",-68,11
+WIFISCAN: "14:09:b4:95:94:6d",-72,6
+WIFISCAN: "f8:8c:21:a2:c6:e9",-74,11
+WIFISCAN: "70:3a:73:05:79:1c",-90,13
+WIFISCAN: "2c:9c:6e:28:f5:0e",-91,1
+WIFISCAN: "7c:a7:b0:61:db:14",-92,11

OK


**************************************************/
static void wifiscanParser(uint8_t *buf, uint16_t len)
{
    int index;
    uint8_t *rebuf, i;
    int16_t relen;
    char restore[20];
    WIFIINFO wifiList;
    rebuf = buf;
    relen = len;
    index = my_getstrindex((char *)rebuf, "+WIFISCAN:", relen);
    wifiList.apcount = 0;
    while (index >= 0)
    {
        rebuf += index + 12;
        relen -= index + 12;

        index = getCharIndex(rebuf, relen, '"');
        if (index == 17 && wifiList.apcount < WIFIINFOMAX)
        {
            memcpy(restore, rebuf, index);
            restore[17] = 0;
            LogPrintf(DEBUG_ALL, "WIFI:[%s]", restore);

            wifiList.ap[wifiList.apcount].signal = 0;
            for (i = 0; i < 6; i++)
            {
                changeHexStringToByteArray(wifiList.ap[wifiList.apcount].ssid + i, restore + (3 * i), 1);
            }
            wifiList.apcount++;
        }
        index = getCharIndex(rebuf, relen, '\n');
        rebuf += index;
        relen -= index;

        index = my_getstrindex((char *)rebuf, "+WIFISCAN:", relen);
    }
    if (wifiList.apcount != 0)
    {
        gpsinfo_s *gpsinfo;
        gpsinfo = getCurrentGPSInfo();
        //当前已经定到位置则不发送lbs
        if (gpsinfo->fixstatus)
        {
            sysinfo.wifiExtendEvt = 0;
            return;
        }
        if (sysinfo.wifiExtendEvt & DEV_EXTEND_OF_MY)
        {
            protocolSend(NORMAL_LINK, PROTOCOL_F3, &wifiList);
        }
        if (sysinfo.wifiExtendEvt & DEV_EXTEND_OF_BLE)
        {
            protocolSend(BLE_LINK, PROTOCOL_F3, &wifiList);
        }
        sysinfo.wifiExtendEvt = 0;
    }

}

static void cgsnParser(uint8_t *buf, uint16_t len)
{
    int16_t index;
    uint8_t *rebuf;
    uint16_t  relen;
    uint8_t i;
    rebuf = buf;
    relen = len;
    index = getCharIndex(rebuf, relen, '\n');
    rebuf = rebuf + index + 1;
    relen = relen - index - 1;
    index = getCharIndex(rebuf, relen, '\r');
    if (index >= 0 && index < 20)
    {
        for (i = 0; i < index; i++)
        {
            moduleState.IMEI[i] = rebuf[i];
        }
        moduleState.IMEI[index] = 0;
        LogPrintf(DEBUG_ALL, "module IMEI [%s]", moduleState.IMEI);
        if (tmos_memcmp(moduleState.IMEI, dynamicParam.SN, 15) == FALSE)
        {
            tmos_memset(dynamicParam.SN, 0, sizeof(dynamicParam.SN));
            strncpy(dynamicParam.SN, moduleState.IMEI, 15);
            jt808CreateSn(dynamicParam.jt808sn, dynamicParam.SN + 3, 12);
            dynamicParam.jt808isRegister = 0;
            dynamicParam.jt808AuthLen = 0;
            dynamicParamSaveAll();
        }
    }

}

/**************************************************
@bref       QIOPEN  指令解析
@param
@return
@note
    0, CONNECT OK
**************************************************/

static void qiopenParser(uint8_t *buf, uint16_t len)
{
    int index;
    uint8_t *rebuf;
    uint8_t sockId;
    int16_t relen;
    uint16_t result;
    char restore[10];
    rebuf = buf;
    relen = len;
    index = my_getstrindex((char *)rebuf, "+QIOPEN:", relen);
    while (index >= 0)
    {
        rebuf += index + 9;
        relen -= index + 9;

        index = getCharIndex(rebuf, relen, ',');
        if (index >= 1 && index <= 2)
        {
            memcpy(restore, rebuf, index);
            restore[index] = 0;
            sockId = atoi(restore);

            rebuf += index + 1;
            relen -= index + 1;

            index = getCharIndex(rebuf, relen, '\r');
            if (index > 0 && index < 9)
            {
                memcpy(restore, rebuf, index);
                restore[index] = 0;
                result = atoi(restore);
                if (result == 0)
                {
                    socketSetConnState(sockId, SOCKET_CONN_SUCCESS);
                }
                else
                {
                    socketSetConnState(sockId, SOCKET_CONN_ERR);
                }
            }
        }
        index = my_getstrindex((char *)rebuf, "+QIOPEN:", relen);
    }
}

/**************************************************
@bref       +CGATT  指令解析
@param
@return
@note
    +CGATT: 1

    OK
**************************************************/

static void cgattParser(uint8_t *buf, uint16_t len)
{
    uint8_t ret;
    int index;
    uint8_t *rebuf;
    int16_t relen;
    rebuf = buf;
    relen = len;
    index = my_getstrindex((char *)rebuf, "+CGATT:", relen);
    if (index < 0)
    {
        return;
    }
    rebuf += index;
    relen -= index;
    ret = rebuf[8] - '0';
    if (ret == 1)
    {
        moduleState.qipactOk = 1;
    }
    else
    {
        moduleState.qipactOk = 0;
    }
}


/**************************************************
@bref       +RECEIVE    指令解析
@param
@return
@note
+RECEIVE: 0, 16
xx
?      \0U?

**************************************************/

uint8_t receiveParser(uint8_t *buf, uint16_t len)
{
    char *rebuf;
    char resbuf[513];
    int index, relen, recvLen;
    int sockId, debugLen;
    rebuf = buf;
    relen = len;
    index = my_getstrindex((char *)rebuf, "+RECEIVE:", relen);
    if (index < 0)
        return 0;
    while (index >= 0)
    {
        rebuf += index + 10;
        relen -= index + 10;
        index = getCharIndex(rebuf, relen, ',');
        if (index >= 1 && index <= 2)
        {
            memcpy(resbuf, rebuf, index);
            resbuf[index] = 0;
            sockId = atoi(resbuf);
            rebuf += index + 2;
            relen -= index + 2;
            index = getCharIndex(rebuf, relen, '\r');
            if (index >= 0 && index <= 5)
            {
                memcpy(resbuf, rebuf, index);
                resbuf[index] = 0;
                recvLen = atoi(resbuf);
                rebuf += index + 2;
                relen -= index + 2;
                if (relen >= recvLen)
                {
                    debugLen = recvLen > 256 ? 256 : recvLen;
                    byteToHexString(rebuf, resbuf, debugLen);
                    resbuf[debugLen * 2] = 0;
                    LogPrintf(DEBUG_ALL, "TCP RECV (%d)[%d]:%s", sockId, recvLen, resbuf);
                    socketRecv(sockId, rebuf, recvLen);
                    rebuf += recvLen;
                    relen -= recvLen;
                }
                else
                {
                    LogMessage(DEBUG_ALL, "TCP data lost");
                    return 1;
                }

            }
        }
        index = my_getstrindex((char *)rebuf, "+RECEIVE:", relen);
    }
    return 0;
}


/**************************************************
@bref       QISACK  指令解析
@param
@return
@note
    +QISACK: 0, 0, 0
**************************************************/

void qisackParser(uint8_t *buf, uint16_t len)
{
    char *rebuf;
    int index, relen;
    ITEM item;
    rebuf = buf;
    relen = len;
    index = my_getstrindex((char *)rebuf, "+QISACK:", relen);
    if (index < 0)
    {
        return;
    }
    rebuf += index + 9;
    relen -= index + 9;
    stringToItem(&item, rebuf, relen);
    if (item.item_cnt >= 3)
    {
        moduleState.tcpTotal = atoi(item.item_data[0]);
        moduleState.tcpAck = atoi(item.item_data[1]);
        moduleState.tcpNack = atoi(item.item_data[2]);
        LogPrintf(DEBUG_ALL, "Total:%d,Ack:%d,NAck:%d", moduleState.tcpTotal, moduleState.tcpAck, moduleState.tcpNack);
    }
}

/**************************************************
@bref       CBC 指令解析
@param
@return
@note
    +CBC: 0,80,3909

    OK
**************************************************/

void cbcParser(uint8_t *buf, uint16_t len)
{
    char *rebuf;
    int index, relen;
    ITEM item;
    rebuf = buf;
    relen = len;
    index = my_getstrindex((char *)rebuf, "+CBC:", relen);
    if (index < 0)
    {
        return;
    }
    rebuf += index + 6;
    relen -= index + 6;
    stringToItem(&item, rebuf, relen);
    if (item.item_cnt >= 3)
    {
        sysinfo.insidevoltage = atoi(item.item_data[2]) / 1000.0;
        LogPrintf(DEBUG_ALL, "batttery voltage %.2f", sysinfo.insidevoltage);
    }
}


/**************************************************
@bref       QIRD    指令解析
@param
@return
@note
+QIRD: 47.107.25.39:9998,TCP,10
xx馶0
?

OK

**************************************************/

static uint8_t qirdParser(uint8_t *buf, uint16_t len)
{
    char *rebuf;
    char resbuf[513];
    int index, relen, recvLen;
    int debugLen;
    uint8_t ret = 0;
    rebuf = buf;
    relen = len;
    index = my_getstrindex(rebuf, "+QIRD:", relen);
    if (index < 0)
    {
        return ret;
    }
    while (index >= 0)
    {
        rebuf += index + 7;
        relen -= index + 7;
        index = getCharIndex(rebuf, relen, '\r');
        if (index >= 0 && index <= 5)
        {
            memcpy(resbuf, rebuf, index);
            resbuf[index] = 0;
            recvLen = atoi(resbuf);
            rebuf += index + 2;
            relen -= index + 2;
            if (relen >= recvLen)
            {
                if (recvLen == 0)
                {
                    switch (moduleState.curQirdId)
                    {
                        case NORMAL_LINK:
                            moduleState.normalLinkQird = 0;
                            break;
                        case BLE_LINK:
                            moduleState.bleLinkQird = 0;
                            break;
                        case JT808_LINK:
                            moduleState.jt808LinkQird = 0;
                            break;
                        case HIDDEN_LINK:
                            moduleState.hideLinkQird = 0;
                            break;
                        case AGPS_LINK:
                            moduleState.agpsLinkQird = 0;
                            break;
                    }
                    LogPrintf(DEBUG_ALL, "Link[%d] recv Done", moduleState.curQirdId);
                }
                else
                {
                    debugLen = recvLen > 256 ? 256 : recvLen;
                    byteToHexString(rebuf, resbuf, debugLen);
                    resbuf[debugLen * 2] = 0;
                    LogPrintf(DEBUG_ALL, "TCP RECV (%d)[%d]:%s", moduleState.curQirdId, recvLen, resbuf);
                    ret = 0;
                    socketRecv(moduleState.curQirdId, rebuf, recvLen);
                }

                rebuf += recvLen;
                relen -= recvLen;
            }
            else
            {
                ret = 1;
                LogMessage(DEBUG_ALL, "TCP data lost");
            }

        }

        index = my_getstrindex((char *)rebuf, "+QIRD:", relen);
    }
    return ret;
}


/**************************************************
@bref       PDP 指令解析
@param
@return
@note
+PDP DEACT


**************************************************/
void deactParser(uint8_t *buf, uint16_t len)
{
    int index;
    index = my_getstrindex(buf, "+PDP DEACT", len);
    if (index < 0)
    {
        return;
    }
    socketDelAll();
    changeProcess(CPIN_STATUS);
}


/**************************************************
@bref       CMT 指令解析
@param
@return
@note
+CMT: "1064899195049",,"2022/09/05 17:46:53+32"
PARAM

**************************************************/

void cmtParser(uint8_t *buf, uint16_t len)
{
    uint8_t *rebuf;
    char restore[130];
    int relen;
    int index;
    insParam_s insparam;
    rebuf = buf;
    relen = len;
    index = my_getstrindex(rebuf, "+CMT:", relen);
    if (index < 0)
    {
        return;
    }
    rebuf += index;
    relen -= index;
    while (index >= 0)
    {
        rebuf += 7;
        relen -= 7;
        index = getCharIndex(rebuf, relen, '\"');
        if (index < 0 || index >= 20)
        {
            return;
        }
        memcpy(moduleState.messagePhone, rebuf, index);
        moduleState.messagePhone[index] = 0;
        LogPrintf(DEBUG_ALL, "TEL:%s", moduleState.messagePhone);
        index = getCharIndex(rebuf, relen, '\n');
        if (index < 0)
        {
            return;
        }
        rebuf += index + 1;
        relen -= index - 1;
        index = getCharIndex(rebuf, relen, '\r');
        if (index < 0 || index >= 128)
        {
            return ;
        }
        memcpy(restore, rebuf, index);
        restore[index] = 0;
        LogPrintf(DEBUG_ALL, "Content:[%s]", restore);
        insparam.telNum = moduleState.messagePhone;
        instructionParser((uint8_t *)restore, index, SMS_MODE, &insparam);
    }
}

void ringParser(uint8_t *buf, uint16_t len)
{
    int index;
    index = my_getstrindex(buf, "RING", len);
    if (index < 0)
    {
        return;
    }
    sendModuleCmd(ATA_CMD, NULL);
}

void cipstartRspParser(uint8_t *buf, uint16_t len)
{
    int index;
    int relen;
    uint8_t *rebuf;
    rebuf = buf;
    relen = len;

    index = my_getstrindex(rebuf, "CONNECT", relen);
    if (index < 0)
    {
        return;
    }

    index = my_getstrindex(rebuf, "0, CONNECT OK", relen);
    if (index >= 0)
    {
        socketSetConnState(0, SOCKET_CONN_SUCCESS);
    }
    index = my_getstrindex(rebuf, "1, CONNECT OK", relen);
    if (index >= 0)
    {
        socketSetConnState(1, SOCKET_CONN_SUCCESS);
    }
    index = my_getstrindex(rebuf, "2, CONNECT OK", relen);
    if (index >= 0)
    {
        socketSetConnState(2, SOCKET_CONN_SUCCESS);
    }
    index = my_getstrindex(rebuf, "3, CONNECT OK", relen);
    if (index >= 0)
    {
        socketSetConnState(3, SOCKET_CONN_SUCCESS);
    }
    index = my_getstrindex(rebuf, "4, CONNECT OK", relen);
    if (index >= 0)
    {
        socketSetConnState(4, SOCKET_CONN_SUCCESS);
    }
    index = my_getstrindex(rebuf, "5, CONNECT OK", relen);
    if (index >= 0)
    {
        socketSetConnState(5, SOCKET_CONN_SUCCESS);
    }

    index = my_getstrindex(rebuf, "0, CONNECT FAIL", relen);
    if (index >= 0)
    {
        socketSetConnState(0, SOCKET_CONN_ERR);
    }
    index = my_getstrindex(rebuf, "1, CONNECT FAIL", relen);
    if (index >= 0)
    {
        socketSetConnState(1, SOCKET_CONN_ERR);
    }
    index = my_getstrindex(rebuf, "2, CONNECT FAIL", relen);
    if (index >= 0)
    {
        socketSetConnState(2, SOCKET_CONN_ERR);
    }
    index = my_getstrindex(rebuf, "3, CONNECT FAIL", relen);
    if (index >= 0)
    {
        socketSetConnState(3, SOCKET_CONN_ERR);
    }
    index = my_getstrindex(rebuf, "4, CONNECT FAIL", relen);
    if (index >= 0)
    {
        socketSetConnState(4, SOCKET_CONN_ERR);
    }
    index = my_getstrindex(rebuf, "5, CONNECT FAIL", relen);
    if (index >= 0)
    {
        socketSetConnState(5, SOCKET_CONN_ERR);
    }
}

void cipstartParser(uint8_t *buf, uint16_t len)
{
    int index;
    index = my_getstrindex(buf, "+CME ERROR:", len);
    if (index < 0)
    {
        return;
    }
    LogMessage(DEBUG_ALL, "socket open err");
    socketDelAll();
    changeProcess(CPIN_STATUS);
    sendModuleCmd(CIPSHUT_CMD, NULL);
}

uint8_t isAgpsDataRecvComplete(void)
{
    return moduleState.agpsLinkQird;
}

/**************************************************
@bref       模组端数据接收解析器
@param
@return
@note
+CIPRXGET: 1,0
+CIPRXGET: 2,0,20,0
xx\0
2
xx\0
2


OK
**************************************************/

uint8_t ciprxgetParser(uint8_t *buf, uint16_t len)
{
    int index;
    char restore[513];
    uint8_t *rebuf, type, link, ret = 0;
    int16_t relen;
    uint16_t readLen, unreadLen, debugLen;
    ITEM item;
    rebuf = buf;
    relen = len;

    index = my_getstrindex(rebuf, "+CIPRXGET:", relen);
    while (index >= 0)
    {
        rebuf += index + 11;
        relen -= index + 11;

        index = getCharIndex(rebuf, relen, '\r');
        if (index >= 0 && index < 20)
        {
            tmos_memcpy(restore, rebuf, index);
            restore[index] = 0;
            stringToItem(&item, restore, index);
            type = atoi(item.item_data[0]);
            link = atoi(item.item_data[1]);
            if (type == 1)
            {
                LogPrintf(DEBUG_ALL, "Socket[%d] recv data", link);
                switch (link)
                {
                    case NORMAL_LINK:
                        moduleState.normalLinkQird = 1;
                        break;
                    case BLE_LINK:
                        moduleState.bleLinkQird = 1;
                        break;
                    case JT808_LINK:
                        moduleState.jt808LinkQird = 1;
                        break;
                    case HIDDEN_LINK:
                        moduleState.hideLinkQird = 1;
                        break;
                    case AGPS_LINK:
                        moduleState.agpsLinkQird = 1;
                        break;
                }
            }
            else if (type == 2)
            {
                readLen = atoi(item.item_data[2]);
                unreadLen = atoi(item.item_data[3]);
                rebuf += index + 2;
                relen -= index + 2;
                if (relen >= readLen)
                {
                    if (readLen != 0)
                    {
                        debugLen = readLen > 256 ? 256 : readLen;
                        byteToHexString(rebuf, restore, debugLen);
//                        if (link == 4)
//                        {
//                          LogMessageWL(DEBUG_ALL, rebuf, relen);
//                        }
                        restore[debugLen * 2] = 0;
                        LogPrintf(DEBUG_ALL, "TCP RECV (%d)[%d]:%s", link, readLen, restore);
                        if (link == 4)
                        {
                            socketRecv(link, rebuf, readLen);
                        }
                        else 
                        {
                            socketRecv(link, rebuf, relen);//relen替代readLen
                        }
                    }
                }
                else
                {
                    ret = 1;
                    break;
                }

                if (unreadLen == 0)
                {
                    switch (link)
                    {
                        case NORMAL_LINK:
                            moduleState.normalLinkQird = 0;
                            break;
                        case BLE_LINK:
                            moduleState.bleLinkQird = 0;
                            break;
                        case JT808_LINK:
                            moduleState.jt808LinkQird = 0;
                            break;
                        case HIDDEN_LINK:
                            moduleState.hideLinkQird = 0;
                            break;
                        case AGPS_LINK:
                            moduleState.agpsLinkQird = 0;
                            break;
                    }
                }
            }
        }

        index = my_getstrindex(rebuf, "+CIPRXGET:", relen);
    }

    return ret;
}

/**************************************************
@bref       模组端数据接收解析器
@param
@return
@note
+CIPACK: 42,42,0
**************************************************/

void cipackParser(uint8_t *buf, uint16_t len)
{
    int index;
    ITEM item;
    uint8_t *rebuf;
    int16_t relen;
    rebuf = buf;
    relen = len;
    static uint8_t cnt;

    index = my_getstrindex(rebuf, "ERROR", relen);
    if (index >= 0)
    {
        cnt++;
        if (cnt >= 5)
        {
            moduleReset();
            cnt = 0;
        }
    }

    
    index = my_getstrindex(rebuf, "+CIPACK:", relen);
    if (index < 0)
    {
        return;
    }

    rebuf += 8;
    relen -= 8;

    if (relen < 0)
    {
        return;
    }
    cnt = 0;
    stringToItem(&item, rebuf, relen);
    moduleState.tcpTotal = atoi(item.item_data[0]);
    moduleState.tcpAck = atoi(item.item_data[1]);
    moduleState.tcpNack = atoi(item.item_data[2]);
    LogPrintf(DEBUG_ALL, "Total:%d,Ack:%d,NAck:%d", moduleState.tcpTotal, moduleState.tcpAck, moduleState.tcpNack);

}



void cipsendParser(uint8_t *buf, uint16_t len)
{
    int index;
    index = my_getstrindex(buf, "+CME ERROR:", len);
    if (index < 0)
    {
        return;
    }
    LogMessage(DEBUG_ALL, "socket send error");
    sendModuleCmd(CIPSTATUS_CMD, NULL);
}


/**************************************************
@bref       模组端数据接收解析器
@param
@return
@note
C: 0,0,"TCP","47.107.25.39","9998","CONNECTED"
C: 1,,"","","","INITIAL"
C: 2,,"","","","INITIAL"
C: 3,,"","","","INITIAL"
C: 4,0,"TCP","47.106.96.28","10188","CLOSED"
C: 5,,"","","","INITIAL"


STATE: IP INITIAL
**************************************************/
void cipstatusParser(uint8_t *buf, uint16_t len)
{
    int index;
    ITEM item;
    uint8_t *rebuf, link;
    int16_t relen;
    rebuf = buf;
    relen = len;

    index = my_getstrindex(rebuf, "IP INITIAL", relen);
    if (index > 0)
    {
        LogMessage(DEBUG_ALL, "All socket lost");
        socketDelAll();
        return;
    }

    index = my_getstrindex(rebuf, "C:", relen);
    while (index >= 0)
    {
        rebuf += index + 3;
        relen -= index + 3;

        index = getCharIndex(rebuf, relen, '\r');
        if (index >= 0)
        {
            stringToItem(&item, rebuf, index);
            link = atoi(item.item_data[0]);
            if (my_strstr(item.item_data[5], "CONNECTED", strlen(item.item_data[5])))
            {
                socketSetConnState(link, SOCKET_CONN_SUCCESS);
            }
            else// if (my_strstr(item.item_data[5], "CLOSED", strlen(item.item_data[5])))
            {
                socketSetConnState(link, SOCKET_CONN_ERR);
            }



            rebuf += index;
            relen -= index;
        }

        index = my_getstrindex(rebuf, "C:", relen);
    }

}

/**************************************************
@bref       模组异常复位检测
@param
@return
@note
**************************************************/
static void moduleRstDetector(uint8_t * buf, uint16_t len)
{
    int index;
    if (moduleState.powerState != 1)
    {
        return;
    }

    index = my_getstrindex((char *)buf, "RDY", len);
    if (index >= 0)
    {
        if (sysinfo.moduleRstFlag == 1)
        {
            sysinfo.moduleRstFlag = 0;
            LogMessage(DEBUG_ALL, "ignore module abnormal reset");
            return;
        }

        sysinfo.moduleRstCnt++;
        LogPrintf(DEBUG_ALL, "module abnormal reset %d", sysinfo.moduleRstCnt);

    }
}


/**************************************************
@bref       模组端数据接收解析器
@param
@return
@note
**************************************************/

void moduleRecvParser(uint8_t *buf, uint16_t bufsize)
{
    static uint16_t len = 0;
    static uint8_t dataRestore[MODULE_RX_BUFF_SIZE + 1];
    if (bufsize == 0)
        return;
    if (len + bufsize > MODULE_RX_BUFF_SIZE)
    {
        len = 0;
        bufsize %= MODULE_RX_BUFF_SIZE;
        LogMessage(DEBUG_ALL, "UartRecv Full!!!");
    }
    memcpy(dataRestore + len, buf, bufsize);
    len += bufsize;
    dataRestore[len] = 0;
    if (dataRestore[len - 1] != '\n')
    {
        if (dataRestore[2] != '>')
        {
            return;
        }
    }
    LogPrintf(DEBUG_ALL, "--->>>---0x%X [%d]", dataRestore, len);
    LogMessageWL(DEBUG_ALL, (char *)dataRestore, len);
    LogMessage(DEBUG_ALL, "---<<<---");
    /*****************************************/

    moduleRstDetector(dataRestore, len);
    moduleRspSuccess();
    cmtiParser(dataRestore, len);
    cmtParser(dataRestore, len);
    cmgrParser(dataRestore, len);
    nmeaParser(dataRestore, len);
    cipstartRspParser(dataRestore, len);
    wifiscanParser(dataRestore, len);
    if (ciprxgetParser(dataRestore, len))
    {
        if (moduleState.cmd == CIPRXGET_CMD)
        {
            return;
        }
    }

    /*****************************************/
    switch (moduleState.cmd)
    {
        case AT_CMD:
            if (distinguishOK((char *)dataRestore))
            {
                moduleState.atResponOK = 1;
            }
            break;
        case CPIN_CMD:
            if (my_strstr((char *)dataRestore, "+CPIN: READY", len))
            {
                moduleState.cpinResponOk = 1;
            }
            break;
        case CSQ_CMD:
            csqParser(dataRestore, len);
            break;
        case CGREG_CMD:
        case CEREG_CMD:
            cgregParser(dataRestore, len);
            break;
        case CIMI_CMD:
            cimiParser(dataRestore, len);
            break;
        case ICCID_CMD:
            iccidParser(dataRestore, len);
            break;
        case CGSN_CMD:
            cgsnParser(dataRestore, len);
            break;
        case CBC_CMD:
            cbcParser(dataRestore, len);
            break;
        case CGATT_CMD:
            cgattParser(dataRestore, len);
            break;
        case CIPACK_CMD:
            cipackParser(dataRestore, len);
            break;
        case CIPSTART_CMD:
            cipstartParser(dataRestore, len);
            break;
        case CIPSTATUS_CMD:
            cipstatusParser(dataRestore, len);
            break;
        case CIPSEND_CMD:
            cipsendParser(dataRestore, len);
            break;
        case CIPRXGET_CMD:
            if (my_strstr((char *)dataRestore, "+CME ERROR: 3", len))
            {
                switch (moduleState.curQirdId)
                {
                    case NORMAL_LINK:
                        moduleState.normalLinkQird = 0;
                        break;
                    case BLE_LINK:
                        moduleState.bleLinkQird = 0;
                        break;
                    case JT808_LINK:
                        moduleState.jt808LinkQird = 0;
                        break;
                    case HIDDEN_LINK:
                        moduleState.hideLinkQird = 0;
                        break;
                    case AGPS_LINK:
                        moduleState.agpsLinkQird = 0;
                        break;
                }
                LogPrintf(DEBUG_ALL, "Link[%d] recv err", moduleState.curQirdId);
            }
            break;
        default:
            break;
    }
    moduleState.cmd = 0;
    len = 0;
}

/*--------------------------------------------------------------*/

/**************************************************
@bref       重置信号搜索时长
@param
@return
@note
**************************************************/

void netResetCsqSearch(void)
{
    moduleCtrl.csqTime = 90;
}

/**************************************************
@bref       socket发送数据
@param
@return
@note
**************************************************/

int socketSendData(uint8_t link, uint8_t *data, uint16_t len)
{
    int ret = 0;
    char param[10];

    if (socketGetConnStatus(link) == 0)
    {
        //链路未链接
        return 0;
    }

    sprintf(param, "%d,%d", link, len);
    sendModuleCmd(AT_CMD, NULL);
    sendModuleCmd(CIPSEND_CMD, param);
    createNode((char *)data, len, CIPSEND_CMD);
    if (link == NORMAL_LINK || link == JT808_LINK)
    {
        moduleState.tcpNack = len;
    }
    return len;
}
/**************************************************
@bref       模组睡眠控制
@param
@return
@note
**************************************************/
void moduleSleepCtl(uint8_t onoff)
{
    char param[5];
    if (onoff == 0)
    {
        return;
    }
    sprintf(param, "%d", onoff);
    sendModuleCmd(AT_CMD, NULL);
    sendModuleCmd(CSCLK_CMD, param);
}

/**************************************************
@bref       获取CSQ
@param
@return
@note
**************************************************/

void moduleGetCsq(void)
{
    sendModuleCmd(CSQ_CMD, NULL);
}

/**************************************************
@bref       获取基站
@param
@return
@note
**************************************************/

void moduleGetLbs(void)
{
    sendModuleCmd(CGREG_CMD, "?");
    sendModuleCmd(CEREG_CMD, "?");
}
/**************************************************
@bref       获取WIFIscan
@param
@return
@note
**************************************************/

void moduleGetWifiScan(void)
{
    sendModuleCmd(AT_CMD, NULL);
    sendModuleCmd(WIFISCAN_CMD, NULL);
}

/**************************************************
@bref       发送短消息
@param
@return
@note
**************************************************/

void sendMessage(uint8_t *buf, uint16_t len, char *telnum)
{
    char param[60];
    sprintf(param, "\"%s\"", telnum);
    sendModuleCmd(CMGF_CMD, "1");
    sendModuleCmd(CMGS_CMD, param);
    LogPrintf(DEBUG_ALL, "len:%d", len);
    buf[len] = 0x1A;
    createNode((char *)buf, len + 1, CMGS_CMD);
}
/**************************************************
@bref       删除所有短消息
@param
@return
@note
**************************************************/


void deleteMessage(void)
{
    sendModuleCmd(CMGD_CMD, "0,4");
}

/**************************************************
@bref       查询数据是否发送完毕
@param
@return
@note
**************************************************/

void querySendData(uint8_t link)
{
    char param[5];
    sprintf(param, "%d", link);
    sendModuleCmd(CIPACK_CMD, param);
}


/**************************************************
@bref       查询模组电池电压
@param
@return
@note
**************************************************/

void queryBatVoltage(void)
{
        sendModuleCmd(AT_CMD, NULL);
        sendModuleCmd(CBC_CMD, NULL);
}

/**************************************************
@bref       读取信号值
@param
@return
@note
**************************************************/

uint8_t getModuleRssi(void)
{
    return moduleState.rssi;
}

/**************************************************
@bref       读取IMSI
@param
@return
@note
**************************************************/

uint8_t *getModuleIMSI(void)
{
    return moduleState.IMSI;
}
/**************************************************
@bref       读取IMEI
@param
@return
@note
**************************************************/

uint8_t *getModuleIMEI(void)
{
    return moduleState.IMEI;
}



/**************************************************
@bref       读取ICCID
@param
@return
@note
**************************************************/

uint8_t *getModuleICCID(void)
{
    return moduleState.ICCID;
}

/**************************************************
@bref       读取MCC
@param
@return
@note
**************************************************/

uint16_t getMCC(void)
{
    return moduleState.mcc;
}

/**************************************************
@bref       读取MNC
@param
@return
@note
**************************************************/

uint8_t getMNC(void)
{
    return moduleState.mnc;
}

/**************************************************
@bref       读取LAC
@param
@return
@note
**************************************************/

uint16_t getLac(void)
{
    return moduleState.lac;
}

/**************************************************
@bref       读取CID
@param
@return
@note
**************************************************/

uint32_t getCid(void)
{
    return moduleState.cid;
}

/**************************************************
@bref       读取未发送字节数，判断是否发送成功
@param
@return
@note
**************************************************/

uint32_t getTcpNack(void)
{
    return moduleState.tcpNack;
}

/**************************************************
@bref       查询模组版本
@param
@return
@note
**************************************************/

char *getQgmr(void)
{
    return moduleState.qgmr;
}


/**************************************************
@bref       模组是否达到联网状态
@param
@return
@note
**************************************************/

uint8_t isModuleRunNormal(void)
{
    if (moduleState.fsmState == NORMAL_STATUS)
        return 1;
    return 0;
}

/**************************************************
@bref       模组达到正常开机
@param
@return
@note
**************************************************/

uint8_t isModulePowerOnOk(void)
{
    if (moduleState.powerOnTick > 10)
        return 1;
    return 0;
}

/**************************************************
@bref       挂断电话
@param
@return
@note
**************************************************/

void stopCall(void)
{
    sendModuleDirect("ATH\r\n");
}
/**************************************************
@bref       拨打电话
@param
@return
@note
**************************************************/

void callPhone(char *tel)
{
    char param[50];
    sprintf(param, "ATD%s;\r\n", tel);
    LogPrintf(DEBUG_ALL, "Call %s", tel);
    sendModuleDirect(param);

}


