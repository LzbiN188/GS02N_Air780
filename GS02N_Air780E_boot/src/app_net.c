#include "app_net.h"
#include "stdlib.h"
#include "app_kernal.h"
#include "app_param.h"
#include "app_sys.h"
#include "app_socket.h"
#include "app_update.h"
//联网相关结构体

static moduleState_s  moduleState;
static moduleCtrl_s moduleCtrl;
static cmdNode_s *headNode = NULL;


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
    {QIOPEN_CMD, "AT+QIOPEN"},
    {QISEND_CMD, "AT+QISEND"},
    {QSCLK_CMD, "AT+QSCLK"},
    {CFUN_CMD, "AT+CFUN"},
    {QICLOSE_CMD, "AT+QICLOSE"},
    {CGDCONT_CMD, "AT+CGDCONT"},
    //合宙
     {CSTT_CMD, "AT+CSTT"},
     {CFGRI_CMD, "AT+CFGRI"},
     {CIPMUX_CMD, "AT+CIPMUX"},
     {CIPSHUT_CMD, "AT+CIPSHUT"},
     {CIICR_CMD, "AT+CIICR"},
     {CIFSR_CMD, "AT+CIFSR"},
     {CIPSTART_CMD, "AT+CIPSTART"},
     {CIPCLOSE_CMD, "AT+CIPCLOSE"},
     {CIPSEND_CMD, "AT+CIPSEND"},
     {CIPRXF_CMD, "AT+CIPRXF"},
     {CIPRXGET_CMD, "AT+CIPRXGET"},
     {CSCLK_CMD, "AT+CSCLK"},
     {CIPACK_CMD, "AT+CIPACK"},
     {COPS_CMD, "AT+COPS"},
     {WIFISCAN_CMD, "AT+WIFISCAN"},
};

/**************************************************
@bref		创建指令输出队列，用于顺序输出
@param
@return
@note
**************************************************/

static uint8_t createNode(char *data, uint16_t datalen, uint8_t currentcmd)
{
    cmdNode_s *nextnode;
    cmdNode_s *currentnode;
    //如果链表头未创建，则创建链表头。
    WAKEMODULE;
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
@bref		数据队列输出
@param
@return
@note
**************************************************/

void outputNode(void)
{
    cmdNode_s *nextnode;
    cmdNode_s *currentnode;
    if (headNode == NULL)
    {
        return ;
    }

    currentnode = headNode;
    if (currentnode != NULL)
    {
        nextnode = currentnode->nextnode;
        moduleState.cmd = currentnode->currentcmd;
        //数据发送
        portUartSend(&usart0_ctl, (uint8_t *)currentnode->data, currentnode->datalen);
        if (currentnode->data[0] != 0X78 && currentnode->data[0] != 0x79 && currentnode->data[0] != 0x7E)
        {
            LogMessageWL(DEBUG_NET, currentnode->data, currentnode->datalen);
        }
        free(currentnode->data);
        free(currentnode);
    }
    headNode = nextnode;
    if (headNode == NULL)
    {
        SLEEPMODULE;
    }
}

/**************************************************
@bref		模组指令发送
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

/**************************************************
@bref		初始化模块相关使用结构体
@param
@return
@note
**************************************************/

static void moduleInit(void)
{
    memset(&moduleState, 0, sizeof(moduleState_s));
    netResetCsqSearch();
}

/**************************************************
@bref		是否开机按键
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
@bref		按下开机按键
@param
@return
@note
**************************************************/

static void modulePressPowerKey(void)
{
    PWRKEY_LOW;
    startTimer(1000, modulePressReleaseKey, 0);
}
/**************************************************
@bref		模组开机
@param
@return
@note
**************************************************/

void modulePowerOn(void)
{
    LogMessage(DEBUG_ALL, "modulePowerOn");
    moduleInit();
    portUartCfg(APPUSART0, 1, 115200, moduleRecvParser);
    POWER_ON;
    PWRKEY_HIGH;
    RSTKEY_HIGH;
    startTimer(600, modulePressPowerKey, 0);
    socketDelAll();
}

/**************************************************
@bref		模组关机
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
    RSTKEY_HIGH;
    PWRKEY_HIGH;
    socketDelAll();
}

/**************************************************
@bref       释放复位按键
@param
@return
@note
**************************************************/

 void moduleReleaseRstkey(void)
{
    moduleState.powerState = 1;
    RSTKEY_HIGH;
}
/**************************************************
@bref		模组复位
@param
@return
@note
**************************************************/

void moduleReset(void)
{
    LogMessage(DEBUG_ALL, "moduleReset");
    moduleInit();
    POWER_OFF;
    PWRKEY_HIGH;
    RSTKEY_HIGH;
    startTimer(1000, modulePowerOn, 0);
    socketDelAll();
}

/**************************************************
@bref		切换联网状态机
@param
@return
@note
**************************************************/
static void changeProcess(uint8_t fsm)
{
    moduleState.fsmState = fsm;
    moduleState.fsmtick = 0;
}

/**************************************************
@bref		创建socket
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
@bref		关闭socket
@param
@return
@note
**************************************************/

void closeSocket(uint8_t link)
{
    char param[10];
    sprintf(param, "%d,0", link);
    sendModuleCmd(CIPCLOSE_CMD, param);

}
/**************************************************
@bref		apn配置
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
@bref		apn配置
@param
@return
@note
**************************************************/

static void netSetApn(char *apn, char *apnname, char *apnpassword)
{
    char param[100];
    sprintf(param, "\"%s\",\"%s\",\"%s\"", apn, apnname, apnpassword);
    sendModuleCmd(CSTT_CMD, param);
}

/**************************************************
@bref		模组进入飞行模式
@param
@return
@note
**************************************************/

static void moduleEnterFly(void)
{
    sendModuleCmd(CFUN_CMD, "0");
}

/**************************************************
@bref		模组进入正常模式
@param
@return
@note
**************************************************/

static void moduleExitFly(void)
{
    sendModuleCmd(CFUN_CMD, "1");
}

/**************************************************
@bref		联网准备任务
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
                sendModuleCmd(AT_CMD, NULL);

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
                moduleCtrl.csqCount = 0;
                sendModuleCmd(CGREG_CMD, "2");
                sendModuleCmd(CEREG_CMD, "2");
                changeProcess(CGREG_STATUS);
                netResetCsqSearch();
            }
            else
            {
                sendModuleCmd(CSQ_CMD, NULL);
                if (moduleState.fsmtick >= moduleCtrl.csqTime)
                {
                    moduleCtrl.csqTime *= 2;
                    moduleCtrl.csqTime=moduleCtrl.csqTime>3600?3600:moduleCtrl.csqTime;
                    moduleCtrl.csqCount++;
                    if (moduleCtrl.csqCount >= 3)
                    {
                        moduleCtrl.csqCount = 0;
                        moduleReset();
                    }
                    else
                    {
                        moduleEnterFly();
                        startTimer(8000, moduleExitFly, 0);
                    }
                }
                break;
            }
        case CGREG_STATUS:
            if (moduleState.cgregOK || moduleState.ceregOK)
            {
                moduleCtrl.cgregCount = 0;
                moduleState.cgregOK = 0;
                changeProcess(CONFIG_STATUS);
            }
            else
            {
                sendModuleCmd(CGREG_CMD, "?");
                sendModuleCmd(CEREG_CMD, "?");
                if (moduleState.fsmtick >= 180)
                {
                    moduleCtrl.cgregCount++;
                    if (moduleCtrl.cgregCount >= 2)
                    {
                        moduleCtrl.cgregCount = 0;
                        moduleState.cgregOK = 1;
                        LogMessage(DEBUG_ALL, "Register timeout,try to skip\r\n");
                    }
                    else
                    {
                        changeProcess(AT_STATUS);
                    }
                }
                break;
            }
        case CONFIG_STATUS:
            sendModuleCmd(CIPMUX_CMD, "1");
            netSetApn((char *)sysparam.apn, (char *)sysparam.apnuser, (char *)sysparam.apnpassword);
            changeProcess(QIACT_STATUS);

            break;
        case QIACT_STATUS:
            if (moduleState.qipactOk)
            {
                moduleState.qipactOk = 0;
                moduleCtrl.qipactCount = 0;
                changeProcess(NORMAL_STATUS);
                sendModuleCmd(CIFSR_CMD, NULL);
            }
            else
            {
                if (moduleState.qipactSet == 0)
                {
                    moduleState.qipactSet = 1;
                    sendModuleCmd(CIICR_CMD, NULL);
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
            break;
        default:
            changeProcess(AT_STATUS);
            break;
    }
    moduleState.fsmtick++;
}

/**************************************************
@bref		AT+CSQ	指令解析
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
@bref		AT+CREG	指令解析
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







/*------------------------------------合宙----------------------------------------*/
/**************************************************
@bref       +CIICR    指令解析
@param
@return
@note

**************************************************/
static void ciicrParser(uint8_t *buf, uint16_t len)
{
    if (distinguishOK((char *)buf))
    {
        moduleState.qipactOk = 1;
    }

}

/**************************************************
@bref       +RECEIVE    指令解析
@param
@return
@note
+RECEIVE,0,10:
xx\0佘

**************************************************/

uint8_t receiveParser(uint8_t *buf, uint16_t len)
{
    uint8_t *rebuf;
    uint16_t relen;
    int index;
    uint8_t sockId;
    char restore[513];
    uint16_t recvLen;
    rebuf = buf;
    relen = len;

    index = my_getstrindex((char *)rebuf, "+RECEIVE", relen);
    if (index < 0)
        return 0;
    while (index >= 0)
    {
        rebuf += index + 9;
        relen -= index - 9;


        index = getCharIndex((char *)rebuf, relen, ',');
        if (index >= 1)
        {
            memcpy(restore, rebuf, index);
            restore[index] = 0;
            sockId = atoi(restore);
            rebuf += index + 1;
            relen -= index - 1;
            index = my_getstrindex((char *)rebuf, ":", relen);
            if (index >= 0)
            {
                memcpy(restore, rebuf, index);
                restore[index] = 0;
                recvLen = atoi(restore);
                rebuf += index + 3;
                relen -= index - 3;
                LogPrintf(DEBUG_ALL, "recvlen:%d", recvLen);
                socketRecv(sockId, rebuf, recvLen);
            }
        }
        index = my_getstrindex((char *)rebuf, "+RECEIVE", relen);
    }
    return 0;

}

/**************************************************
@bref       CIPSTATRT  指令解析
@param
@return
@note
    0, CONNECT OK
**************************************************/
static void cipstartParser(uint8_t *buf, uint16_t len)
{
    int index;
    uint8_t *rebuf;
    uint16_t relen;
    uint8_t sockId;
    char restore[10];
    rebuf = buf;
    relen = len;

    index = my_getstrindex((char *)rebuf, "CONNECT FAIL", len);
    if (index >= 0 && len < 30)
    {
        rebuf += index - 3;
        relen -= index + 3;
        index = getCharIndex(rebuf, relen, ',');
        if (index > 0)
        {
            memcpy(restore, rebuf, index);
            restore[index] = 0;
            sockId = atoi(restore);
            socketSetConnState(sockId, SOCKET_CONN_ERR);
        }
    }

    index = my_getstrindex((char *)rebuf, "CONNECT OK", relen);
    if (index >= 0 && len < 30)
    {
        rebuf += index - 3;
        relen -= index + 3;
        index = getCharIndex(rebuf, relen, ',');
        if (index > 0)
        {
            memcpy(restore, rebuf, index);
            restore[index] = 0;
            sockId = atoi(restore);
            socketSetConnState(sockId, SOCKET_CONN_SUCCESS);
        }
    }

}

/**************************************************
@bref      closed 指令解析
@param
@return
@note
0, CLOSED

**************************************************/

static void closedParser(uint8_t *buf, uint16_t len)
{
    int index;
    uint8_t *rebuf;
    uint16_t relen;
    uint8_t sockId;
    char restore[10];
    rebuf = buf;
    relen = len;

    index = my_getstrindex(rebuf, "CLOSED", relen);
    if (index < 0)
        return;
    if (index >= 0 && len < 30)
    {
        rebuf += index - 3;
        relen -= index + 3;
        index = getCharIndex(rebuf, relen, ',');
        if (index > 0)
        {
            memcpy(restore, rebuf, index);
            restore[index] = 0;
            sockId = atoi(restore);
            socketSetConnState(sockId, SOCKET_CONN_IDLE);
        }
    }
}




/**************************************************
@bref		模组端数据接收解析器
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

    if(len <= 2)
    {
        return;
    }
    if(dataRestore[len-2]!='\r' || dataRestore[len-1]!='\n')
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
    moduleRspSuccess();
    receiveParser(dataRestore, len);
    cipstartParser(dataRestore, len);
    closedParser(dataRestore, len);

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
        case CIICR_CMD:
            ciicrParser(dataRestore, len);
            break;
        default:
            break;
    }

    moduleState.cmd = 0;
    len = 0;
}
/*--------------------------------------------------------------*/

/**************************************************
@bref		重置信号搜索时长
@param
@return
@note
**************************************************/

void netResetCsqSearch(void)
{
    moduleCtrl.csqTime = 120;
}

/**************************************************
@bref		socket发送数据
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
    createNode((char *)data, len, QISEND_CMD);

    return len;
}

/**************************************************
@bref		模组是否达到联网状态
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

