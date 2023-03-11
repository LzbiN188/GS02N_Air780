/*
 * app_update.h
 *
 *  Created on: 2021年10月16日
 *      Author: idea
 */

#ifndef APP_UPDATE
#define APP_UPDATE

#define APPLICATION_ADDRESS     0xC000

#include "CH58x_common.h"


#define jumpApp  ((  void  (*)  ( void ))  ((int*)APPLICATION_ADDRESS))


typedef enum
{
    NETWORK_LOGIN,
    NETWORK_LOGIN_WAIT,
    NETWORK_LOGIN_READY,
    NETWORK_DOWNLOAD_DOING,
    NETWORK_DOWNLOAD_WAIT,
    NETWORK_DOWNLOAD_DONE,
    NETWORK_DOWNLOAD_ERROR,
    NETWORK_WAIT_JUMP
} networkFsm_e;


typedef struct
{
    networkFsm_e fsmstate;
    unsigned short serial;
    uint16_t runtick;
    uint8_t loginCount;
    uint8_t getVerCount;
    uint8_t socketErrCount;
} netConnect_s;

typedef enum
{
    PROTOCOL_01,//登录
    PROTOCOL_F3
} protocolType_e;


typedef struct
{
    char curCODEVERSION[50];
    char newCODEVERSION[50];
    char rxsn[50];
    char rxcurCODEVERSION[50];
    uint32_t file_id;
    uint32_t file_offset;
    uint32_t file_len;
    uint32_t file_totalsize;

    uint32_t rxfileOffset;//已接收文件长度
    uint8_t updateOK;
} upgradeInfo_s;

typedef  void (*pFunction)(void);

void upgradeRunTask(void);
void upgradeInfoInit(void);

void moduleRspSuccess(void);
void moduleRspTimeout(void);

void startJumpToApp(void);
uint8_t JumpToApp(uint32_t appaddress);

void upgradeSocketErr(void);
void upgradeSocketSuccess(void);

#endif /* SRC_INC_APP_UPDATE_H_ */
