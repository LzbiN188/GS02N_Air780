#include "app_param.h"

SYSTEM_FLASH_DEFINE sysparam;

void paramSaveAll(void)
{
    EEPROM_ERASE(APP_USER_PARAM_ADDR, sizeof(SYSTEM_FLASH_DEFINE));
    EEPROM_WRITE(APP_USER_PARAM_ADDR, &sysparam, sizeof(SYSTEM_FLASH_DEFINE));
}
void paramGetAll(void)
{
    EEPROM_READ(APP_USER_PARAM_ADDR, &sysparam, sizeof(SYSTEM_FLASH_DEFINE));
}

void paramDefaultInit(uint8_t level)
{
    sysparam.VERSION = APP_PARAM_FLAG;
    strcpy((char *)sysparam.apn, "cmnet");
    sysparam.apnuser[0] = 0;
    sysparam.apnpassword[0] = 0;
    strcpy((char *)sysparam.updateServer, "47.106.81.204");
    sysparam.updatePort = 9998;
    strcpy((char *)sysparam.SN, "88888888777777");// 88888888777777
    sysparam.updateStatus = 0;
    strcpy((char *)sysparam.codeVersion, "CH573_M09_BOOT");
    paramSaveAll();
    LogMessage(DEBUG_ALL, "Param Defint");
}

void paramInit(void)
{
    paramGetAll();
    if (sysparam.VERSION != APP_PARAM_FLAG)
    {
        paramDefaultInit(0);
    }

}

void paramGetCodeVersion(char *version)
{
    strcpy(version, (char *)sysparam.codeVersion);
}
void paramSaveUpdateStatus(uint8_t status)
{
    sysparam.updateStatus = status;
    paramSaveAll();
}
