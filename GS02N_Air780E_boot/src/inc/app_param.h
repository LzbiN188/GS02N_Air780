#ifndef APP_EEPROM_H
#define APP_EEPROM_H
#include <stdint.h>
#include "app_sys.h"

//目前预留前1K字节给bootloader，所以app从0x400开始
#define APP_USER_PARAM_ADDR   0x00  //实际是0x00070000+APP_USER_PARAM_ADDR
#define APP_PARAM_FLAG        0xB0


/*存储在EEPROM中的数据*/
typedef struct
{
    uint8_t VERSION;        /*当前软件版本号*/
    uint8_t updateStatus;
    uint8_t apn[50];
    uint8_t apnuser[50];
    uint8_t apnpassword[50];
    uint8_t SN[20];
    uint8_t updateServer[50];
    uint8_t codeVersion[50];
    uint16_t updatePort;
} SYSTEM_FLASH_DEFINE;


extern SYSTEM_FLASH_DEFINE sysparam;


void paramInit(void);
void paramDefaultInit(uint8_t type);

void paramGetCodeVersion(char *version);
void paramSaveUpdateStatus(uint8_t status);

void paramSaveAll(void);
#endif

