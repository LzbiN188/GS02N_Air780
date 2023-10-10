#include "app_protocol.h"
#include "app_atcmd.h"
#include "app_db.h"
#include "app_gps.h"
#include "app_instructioncmd.h"
#include "app_kernal.h"
#include "app_mir3da.h"
#include "app_net.h"
#include "app_param.h"
#include "app_port.h"
#include "app_sys.h"
#include "app_socket.h"
#include "app_server.h"
#include "aes.h"
#include "app_bleRelay.h"
#include "app_jt808.h"
#include "app_task.h"
#include "app_central.h"


/*
 * 指令集
 */
const CMDTABLE atcmdtable[] =
{
    {AT_DEBUG_CMD, "DEBUG"},
    {AT_SMS_CMD, "SMS"},
    {AT_NMEA_CMD, "NMEA"},
    {AT_ZTSN_CMD, "ZTSN"},
    {AT_IMEI_CMD, "IMEI"},
    {AT_FMPC_NMEA_CMD, "FMPC_NMEA"},
    {AT_FMPC_BAT_CMD, "FMPC_BAT"},
    {AT_FMPC_GSENSOR_CMD, "FMPC_GSENSOR"},
    {AT_FMPC_ACC_CMD, "FMPC_ACC"},
    {AT_FMPC_GSM_CMD, "FMPC_GSM"},
    {AT_FMPC_RELAY_CMD, "FMPC_RELAY"},
    {AT_FMPC_LDR_CMD, "FMPC_LDR"},
    {AT_FMPC_ADCCAL_CMD, "FMPC_ADCCAL"},
    {AT_FMPC_CSQ_CMD, "FMPC_CSQ"},
    {AT_FMPC_IMSI_CMD, "FMPC_IMSI"},
    {AT_FMPC_CHKP_CMD, "FMPC_CHKP"},
    {AT_FMPC_CM1_CMD, "FMPC_CM1"},
    {AT_FMPC_CM1GET_CMD, "FMPC_CM1GET"},
    {AT_FMPC_CM2_CMD, "FMPC_CM2"},
    {AT_FMPC_CM2GET_CMD, "FMPC_CM2GET"},
    {AT_FMPC_EXTVOL_CMD, "FMPC_EXTVOL"},
    {AT_BLECONN_CMD, "BLECONN"},
};
/**************************************************
@bref		查找指令
@param
@return
@note
**************************************************/

static int16_t getatcmdid(uint8_t *cmdstr)
{
    uint16_t i = 0;
    for (i = 0; i < sizeof(atcmdtable) / sizeof(atcmdtable[0]); i++)
    {
        if (mycmdPatch(cmdstr, (uint8_t *)atcmdtable[i].cmdstr) != 0)
            return atcmdtable[i].cmdid;
    }
    return -1;
}

/**************************************************
@bref		AT^DEBUG 指令解析
@param
@return
@note
**************************************************/

static void doAtdebugCmd(uint8_t *buf, uint16_t len)
{
    int8_t ret;
    ITEM item;
    stringToItem(&item, buf, len);
    strToUppper(item.item_data[0], strlen(item.item_data[0]));

    if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"SUSPEND"))
    {
        LogMessage(DEBUG_LOW, "Suspend all task");
        systemTaskSuspend(sysinfo.sysTaskId);
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"RESUME"))
    {
        LogMessage(DEBUG_LOW, "Resume all task");
        systemTaskResume(sysinfo.sysTaskId);
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"DTRH"))
    {
        WAKEMODULE;
        LogMessage(DEBUG_ALL, "DTR high");
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"DTRL"))
    {
        SLEEPMODULE;
        LogMessage(DEBUG_ALL, "DTR low");
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"POWERKEYH"))
    {
        PORT_PWRKEY_H;
        LogMessage(DEBUG_ALL, "Power key hight");
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"POWERKEYL"))
    {
        PORT_PWRKEY_L;
        LogMessage(DEBUG_ALL, "Power key low");
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"SUPPLYON"))
    {
        PORT_SUPPLY_ON;
        LogMessage(DEBUG_ALL, "supply on");
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"SUPPLYOFF"))
    {
        PORT_SUPPLY_OFF;
        LogMessage(DEBUG_ALL, "supply off");
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"GPSCLOSE"))
    {
        sysinfo.gpsRequest = 0;
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"MODULEOFF"))
    {
        modulePowerOff();
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"MODULERESET"))
    {
        moduleReset();
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"MODULEON"))
    {
        modulePowerOn();
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"SLEEP"))
    {
        portSleepEn();
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"WAKEUP"))
    {
		portSleepDn();
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"SLEEPSTATE"))
    {
        portGetSleepState();
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"AGPS"))
    {
        agpsRequestSet();
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"BLESCAN"))
    {
		bleCentralStartDiscover();
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"AA"))
    {
		sysinfo.debugflag = 1;
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"BB"))
    {
		sysinfo.debugflag = 0;
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"WIFI"))
    {
        moduleGetWifiScan();

    }
    else
    {
        if (item.item_data[0][0] >= '0' && item.item_data[0][0] <= '9')
        {
            sysinfo.logLevel = item.item_data[0][0] - '0';
            LogPrintf(DEBUG_NONE, "Debug LEVEL:%d OK", sysinfo.logLevel);
        }
    }
}
/**************************************************
@bref		AT^FMPC_ADCCAL 指令解析
@param
@return
@note
**************************************************/

static void atCmdFmpcAdccalParser(void)
{
    float x;
    x = portGetAdcVol(CH_EXTIN_13);
    LogPrintf(DEBUG_ALL, "vCar:%.2f", x);
    sysparam.adccal = 24.0 / x;
    paramSaveAll();
    LogPrintf(DEBUG_FACTORY, "Update the ADC calibration parameter to %f", sysparam.adccal);
}

/**************************************************
@bref		AT^NMEA 指令解析
@param
@return
@note
**************************************************/

static void atCmdNmeaParser(uint8_t *buf, uint16_t len)
{
    char buff[80];
    if (my_strstr((char *)buf, "ON", len))
    {
        strcpy(buff, "$PCAS03,0,0,0,1,1,0,0,0,0,0,0,0,0,0*02\r\n");
        portUartSend(&usart3_ctl, buff, strlen(buff));
        LogMessage(DEBUG_FACTORY, "NMEA ON OK");
        sysinfo.nmeaOutPutCtl = 1;
        gpsRequestSet(GPS_REQUEST_DEBUG);
    }
    else
    {
        LogMessage(DEBUG_FACTORY, "NMEA OFF OK");
        gpsRequestClear(GPS_REQUEST_ALL);
        sysinfo.nmeaOutPutCtl = 0;
    }


}

/**************************************************
@bref		AT^FMPC_GSENSOR 指令解析
@param
@return
@note
**************************************************/

static void atCmdFMPCgsensorParser(void)
{
    read_gsensor_id();
}

/**************************************************
@bref		ZTSN 指令
@param
	buf
	len
@return
@note
**************************************************/

static void atCmdZTSNParser(uint8_t *buf, uint16_t len)
{
//    char IMEI[15];
//    uint8_t sndata[30];
//    changeHexStringToByteArray(sndata, buf, len / 2);
//    decryptSN(sndata, IMEI);
//    LogPrintf(DEBUG_FACTORY, "Decrypt: %s", IMEI);
//    strncpy((char *)sysparam.SN, IMEI, 15);
//    sysparam.SN[15] = 0;
//    jt808CreateSn(sysparam.jt808sn, (uint8_t *)sysparam.SN + 3, 12);
//    sysparam.jt808isRegister = 0;
//    paramSaveAll();
//    LogMessage(DEBUG_FACTORY, "Write Sn Ok");
}


/**************************************************
@bref		IMEI 指令
@param
	buf
	len
@return
@note
**************************************************/

void atCmdIMEIParser(void)
{
    char imei[20];
    LogPrintf(DEBUG_FACTORY, "ZTINFO:%s:%s:%s", dynamicParam.SN, getModuleIMEI(), EEPROM_VERSION);
}

//unsigned char nmeaCrc(char *str, int len)
//{
//    int i, index, size;
//    unsigned char crc;
//    crc = str[1];
//    index = getCharIndex((uint8_t *)str, len, '*');
//    size = len - index;
//    for (i = 2; i < len - size; i++)
//    {
//        crc ^= str[i];
//    }
//    return crc;
//}

/**************************************************
@bref		FMPC_NMEA 指令
@param
	buf
	len
@return
@note
**************************************************/

static void atCmdFmpcNmeaParser(uint8_t *buf, uint16_t len)
{
    char buff[80];
    if (my_strstr((char *)buf, "ON", len))
    {
        sysinfo.nmeaOutPutCtl = 1;
        //开启AGPS有效性检测
   		sprintf(buff, "$PCAS03,,,,,,,,,,,1*1F\r\n");
    	portUartSend(&usart3_ctl, (uint8_t *)buff, strlen(buff));
        LogMessage(DEBUG_FACTORY, "NMEA OPEN");
    }
    else
    {
//    	//关闭AGPS有效性检测
   		sprintf(buff, "$PCAS03,,,,,,,,,,,0*1E\r\n");
    	portUartSend(&usart3_ctl, (uint8_t *)buff, strlen(buff));
        sysinfo.nmeaOutPutCtl = 0;
        LogMessage(DEBUG_FACTORY, "NMEA CLOSE");
    }
}
/**************************************************
@bref		FMPC_BAT 指令
@param
	buf
	len
@return
@note
**************************************************/

static void atCmdFmpcBatParser(void)
{
    queryBatVoltage();
    LogPrintf(DEBUG_FACTORY, "Vbat: %.3fv", sysinfo.insidevoltage);
}
/**************************************************
@bref		FMPC_ACC 指令
@param
	buf
	len
@return
@note
**************************************************/

static void atCmdFmpcAccParser(void)
{
    LogPrintf(DEBUG_FACTORY, "ACC is %s", ACC_READ == ACC_STATE_ON ? "ON" : "OFF");
}

/**************************************************
@bref		FMPC_GSM  指令
@param
	buf
	len
@return
@note
**************************************************/

static void atCmdFmpcGsmParser(void)
{
    if (isModuleRunNormal())
    {
        LogMessage(DEBUG_FACTORY, "GSM SERVICE OK");
    }
    else
    {
        LogMessage(DEBUG_FACTORY, "GSM SERVICE FAIL");
    }
}

/**************************************************
@bref		FMPC_CSQ 指令
@param
	buf
	len
@return
@note
**************************************************/

static void atCmdFmpcCsqParser(void)
{
    sendModuleCmd(CSQ_CMD, NULL);
    LogPrintf(DEBUG_FACTORY, "+CSQ: %d,99\r\nOK", getModuleRssi());
}


/**************************************************
@bref		FMPC_IMSI 指令
@param
	buf
	len
@return
@note
**************************************************/

static void atCmdFmpcIMSIParser(void)
{
    sendModuleCmd(CIMI_CMD, NULL);
    sendModuleCmd(ICCID_CMD, NULL);
    sendModuleCmd(CGSN_CMD, NULL);
    LogPrintf(DEBUG_FACTORY, "FMPC_IMSI_RSP OK, IMSI=%s&&%s&&%s", dynamicParam.SN, getModuleIMSI(), getModuleICCID());
}

/**************************************************
@bref		FMPC_CHKP 指令
@param
	buf
	len
@return
@note
**************************************************/

static void atCmdFmpcChkpParser(void)
{
    LogPrintf(DEBUG_FACTORY, "+FMPC_CHKP:%s,%s:%d", dynamicParam.SN, sysparam.Server, sysparam.ServerPort);
}

/**************************************************
@bref		FMPC_CM1 指令
@param
	buf
	len
@return
@note
**************************************************/

static void atCmdFmpcCm1Parser(void)
{
    sysparam.cm = 1;
    paramSaveAll();
    LogMessage(DEBUG_FACTORY, "CM1 OK");

}
/**************************************************
@bref		FMPC_CM1GET 指令
@param
@return
@note
**************************************************/

static void atCmdCm1GetParser(void)
{
    if (sysparam.cm == 1)
    {
        LogMessage(DEBUG_FACTORY, "CM1 OK");
    }
    else
    {
        LogMessage(DEBUG_FACTORY, "CM1 FAIL");
    }
}

/**************************************************
@bref		FMPC_CM2 指令
@param
	buf
	len
@return
@note
**************************************************/
static void atCmdFmpcCm2Parser(void)
{
	sysparam.cm2 = 1;
	paramSaveAll();
	LogMessage(DEBUG_FACTORY, "CM2 OK");
}

/**************************************************
@bref		FMPC_CM2GET 指令
@param
@return
@note
**************************************************/
static void atCmdCm2GetParser(void)
{
	if (sysparam.cm2)
	{
		LogMessage(DEBUG_FACTORY, "CM2 OK");
	}
	else
	{
		LogMessage(DEBUG_FACTORY, "CM2 FAIL");
	}
}


/**************************************************
@bref		FMPC_EXTVOL 指令
@param
@return
@note
**************************************************/

static void atCmdFmpcExtvolParser(void)
{
    LogPrintf(DEBUG_FACTORY, "+FMPC_EXTVOL: %.2f", sysinfo.outsidevoltage);
}

static void atCmdBleconnParser(uint8_t *buf, uint16_t len)
{
	uint8_t ind = 0, j = 0, l;
	char mac[20];
	bleRelayDeleteAll();
	tmos_memset(sysparam.bleConnMac, 0, sizeof(sysparam.bleConnMac));
    ind = 0;

    //aa bb cc dd ee ff
    //ff ee dd cc bb aa

    l = 5;
    for (j = 0; j < 3; j++)
    {
        tmos_memcpy(mac, buf + (j * 2), 2);
        tmos_memcpy(buf + (j * 2), buf + (l * 2), 2);
        tmos_memcpy(buf + (l * 2), mac, 2);
        l--;
    }
    if (ind < 2)
    {
        changeHexStringToByteArray(sysparam.bleConnMac[0], buf, 6);
        bleRelayInsert(sysparam.bleConnMac[0], 0);
        ind++;
    }
    LogPrintf(DEBUG_FACTORY, "+FMPC:Connect ble %s", buf);
    startTimer(150, bleRelayDeleteAll, 0);
    
}

/**************************************************
@bref		AT 指令解析
@param
@return
@note
**************************************************/

void atCmdParserFunction(uint8_t *buf, uint16_t len)
{
    int ret, cmdlen, cmdid;
    char cmdbuf[51];
    //识别AT^
    if (buf[0] == 'A' && buf[1] == 'T' && buf[2] == '^')
    {
        LogMessageWL(DEBUG_ALL, (char *)buf, len);
        ret = getCharIndex(buf, len, '=');
        if (ret < 0)
        {
            ret = getCharIndex(buf, len, '\r');

        }
        if (ret >= 0)
        {
            cmdlen = ret - 3;
            if (cmdlen < 50)
            {
                strncpy(cmdbuf, (const char *)buf + 3, cmdlen);
                cmdbuf[cmdlen] = 0;
                cmdid = getatcmdid((uint8_t *)cmdbuf);
                switch (cmdid)
                {
                    case AT_SMS_CMD:
                        instructionParser(buf + ret + 1, len - ret - 1, DEBUG_MODE, NULL);
                        break;
                    case AT_DEBUG_CMD:
                        doAtdebugCmd(buf + ret + 1, len - ret - 1);
                        break;
                    case AT_NMEA_CMD:
                        atCmdNmeaParser(buf + ret + 1, len - ret - 1);
                        break;
                    case AT_ZTSN_CMD:
                        atCmdZTSNParser((uint8_t *)buf + ret + 1, len - ret - 1);
                        break;
                    case AT_IMEI_CMD:
                        atCmdIMEIParser();
                        break;
                    case AT_FMPC_NMEA_CMD :
                        atCmdFmpcNmeaParser((uint8_t *)buf + ret + 1, len - ret - 1);
                        break;
                    case AT_FMPC_BAT_CMD :
                        atCmdFmpcBatParser();
                        break;
                    case AT_FMPC_GSENSOR_CMD :
                        atCmdFMPCgsensorParser();
                        break;
                    case AT_FMPC_ACC_CMD :
                        atCmdFmpcAccParser();
                        break;
                    case AT_FMPC_GSM_CMD :
                        atCmdFmpcGsmParser();
                        break;
                    case AT_FMPC_ADCCAL_CMD:
                        atCmdFmpcAdccalParser();
                        break;
                    case AT_FMPC_CSQ_CMD:
                        atCmdFmpcCsqParser();
                        break;
                    case AT_FMPC_IMSI_CMD:
                        atCmdFmpcIMSIParser();
                        break;
                    case AT_FMPC_CHKP_CMD:
                        atCmdFmpcChkpParser();
                        break;
                    case AT_FMPC_CM1_CMD:
                        atCmdFmpcCm1Parser();
                        break;
                    case AT_FMPC_CM1GET_CMD:
                        atCmdCm1GetParser();
                        break;
                    case AT_FMPC_CM2_CMD:
                    	atCmdFmpcCm2Parser();
                    	break;
                    case AT_FMPC_CM2GET_CMD:
                    	atCmdCm2GetParser();
                    	break;
                    case AT_FMPC_EXTVOL_CMD:
                        atCmdFmpcExtvolParser();
                        break;
                    case AT_BLECONN_CMD:
						atCmdBleconnParser((uint8_t *)buf + ret + 1, len - ret - 1);
                    	break;
                    default:
                        LogMessage(DEBUG_ALL, "Unknown Cmd");
                        break;
                }
            }
        }
    }
    else
    {
        createNode(buf, len, 0);
       	//portUartSend(&usart0_ctl, buf, len);
    }
}
