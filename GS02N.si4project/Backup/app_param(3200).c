#include "app_param.h"

#include "app_db.h"

bootParam_s bootparam;
systemParam_s sysparam;
dynamicParam_s dynamicParam;


void bootParamSaveAll(void)
{
    bootparam.VERSION = BOOT_PARAM_FLAG;
    EEPROM_ERASE(BOOT_USER_PARAM_ADDR, sizeof(bootParam_s));
    EEPROM_WRITE(BOOT_USER_PARAM_ADDR, &bootparam, sizeof(bootParam_s));
}
void bootParamGetAll(void)
{
    EEPROM_READ(BOOT_USER_PARAM_ADDR, &bootparam, sizeof(bootParam_s));
}

void paramSaveAll(void)
{
    EEPROM_ERASE(APP_USER_PARAM_ADDR, sizeof(systemParam_s));
    EEPROM_WRITE(APP_USER_PARAM_ADDR, &sysparam, sizeof(systemParam_s));
}
void paramGetAll(void)
{
    EEPROM_READ(APP_USER_PARAM_ADDR, &sysparam, sizeof(systemParam_s));
}

void dynamicParamSaveAll(void)
{
	EEPROM_ERASE(APP_DYNAMIC_PARAM_ADDR, sizeof(dynamicParam_s));
	EEPROM_WRITE(APP_DYNAMIC_PARAM_ADDR, &dynamicParam, sizeof(dynamicParam_s));
}

void dynamicParamGetAll(void)
{
	EEPROM_READ(APP_DYNAMIC_PARAM_ADDR, &dynamicParam, sizeof(dynamicParam_s));
}

void paramDefaultInit(uint8_t level)
{
    LogMessage(DEBUG_ALL, "paramDefaultInit");
    if (level == 0)
    {
        memset(&sysparam, 0, sizeof(systemParam_s));
        sysparam.VERSION = APP_PARAM_FLAG;
        strncpy(dynamicParam.SN, "888888887777777", 15);
        strncpy(dynamicParam.jt808sn, "888777", 6);

        strcpy(sysparam.Server, "jzwz.basegps.com");
        strcpy(sysparam.hiddenServer, "jzwz.basegps.com");
        strcpy(sysparam.bleServer, "jzwz.basegps.com");
        strcpy(sysparam.jt808Server, "47.106.96.28");

        sysparam.ServerPort = 9998;
        sysparam.bleServerPort = 9998;
        sysparam.hiddenPort = 9998;
        sysparam.jt808Port = 9997;

        sysparam.utc = 8;
        sysparam.AlarmTime[0] = 720;
        sysparam.AlarmTime[1] = 0xFFFF;
        sysparam.AlarmTime[2] = 0xFFFF;
        sysparam.AlarmTime[3] = 0xFFFF;
        sysparam.AlarmTime[4] = 0xFFFF;
        sysparam.MODE = 2;
        sysparam.gpsuploadgap = 10;
        sysparam.gapDay = 1;
        sysparam.poitype = 2;
        sysparam.lowvoltage = 115;
        sysparam.adccal = 41.81;
        strcpy(sysparam.apn, "cmnet");

        strcpy((char *)sysparam.agpsServer, "agps.domilink.com");
        strcpy((char *)sysparam.agpsUser, "123");
        strcpy((char *)sysparam.agpsPswd, "123");
        sysparam.agpsPort = 10188;

    }
    dynamicParam.runTime = 0;
    dynamicParam.startUpCnt = 0;
    sysparam.accctlgnss = 1;
    sysparam.accdetmode = 2;
    sysparam.heartbeatgap = 180;
    sysparam.ledctrl = 1;
    sysparam.poitype = 2;
    sysparam.fence = 30;
    sysparam.accOnVoltage = 13.2;
    sysparam.accOffVoltage = 12.8;
    dynamicParam.noNmeaRstCnt = 0;
    sysparam.sosalm = ALARM_TYPE_NONE;
	
    sysparam.bleRfThreshold = 48;
    sysparam.bleOutThreshold = 1500;
    sysparam.bleAutoDisc = 0;
    sysparam.blePreShieldVoltage = 48;
    sysparam.blePreShieldDetCnt = 10;
    sysparam.blePreShieldHoldTime = 1;
    sysparam.relaySpeed = 20;
    sysparam.protectVoltage = 4.6;
    sysparam.bf = 0;
    sysparam.gsdettime=15;
    sysparam.gsValidCnt=7;
    sysparam.gsInvalidCnt=0;
    sysparam.hiddenServOnoff = 1;
    sysparam.agpsen = 1;
    sysparam.bleen = 1;

    paramSaveAll();
}

void paramInit(void)
{
    paramGetAll();
    if (sysparam.VERSION != APP_PARAM_FLAG)
    {
        paramDefaultInit(0);
    }
    sysinfo.lowvoltage = sysparam.lowvoltage / 10.0;
    dbInfoRead();
}
