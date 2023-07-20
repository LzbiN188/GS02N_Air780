/*
 * app_server.h
 *
 *  Created on: Jul 14, 2022
 *      Author: idea
 */

#ifndef TASK_INC_APP_SERVER_H_
#define TASK_INC_APP_SERVER_H_

#include "CH58x_common.h"

typedef enum
{
    SERV_LOGIN,
    SERV_LOGIN_WAIT,
    SERV_READY,
    
    SERV_END,
} NetWorkFsmState;
typedef struct
{
    NetWorkFsmState fsmstate;
    unsigned int heartbeattick;
    unsigned short serial;
    uint8_t logintick;
    uint8_t loginCount;
} netConnectInfo_s;
typedef struct bleDev
{
    char imei[16];
    uint8_t batLevel;
    uint16_t startCnt;
    float   vol;
    struct bleDev *next;
} bleInfo_s;


typedef enum
{
    JT808_REGISTER,
    JT808_AUTHENTICATION,
    JT808_NORMAL,
} jt808_connfsm_s;



typedef struct
{
    jt808_connfsm_s connectFsm;
    uint8_t runTick;
    uint8_t regCnt;
    uint8_t authCnt;
    uint16_t hbtTick;
} jt808_Connect_s;

void moduleRspSuccess(void);
void hbtRspSuccess(void);

void privateServerReconnect(void);
void privateServerLoginSuccess(void);

void hiddenServerLoginSuccess(void);
void hiddenServerCloseRequest(void);
void hiddenServerCloseClear(void);


void jt808ServerReconnect(void);
void jt808ServerAuthSuccess(void);


void bleServerAddInfo(bleInfo_s dev);
void showBleList(void);
void bleSerLoginReady(void);

void agpsDataInit(void);
void agpsRequestSet(void);
void agpsRequestClear(void);

uint8_t primaryServerIsReady(void);
uint8_t hiddenServerIsReady(void);

void serverManageTask(void);

#endif /* TASK_INC_APP_SERVER_H_ */
