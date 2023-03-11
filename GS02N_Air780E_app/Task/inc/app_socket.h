/*
 * app_socket.h
 *
 *  Created on: Jul 13, 2022
 *      Author: idea
 */

#ifndef TASK_INC_APP_SOCKET_H_
#define TASK_INC_APP_SOCKET_H_

#include "CH58x_common.h"

typedef struct
{
    uint8_t usedFlag    : 1;
    uint8_t connOK;
    uint8_t socketId;
    char server[40];
    uint16_t port;
    void (*recvCallBack)(char *data, uint16_t len);
} SocketInfo_s ;

typedef enum
{
    SOCKET_SCHEDULE_IDLE,
    SOCKET_SCHEDULE_CONN,
    SOCKET_SCHEDULE_END,
    SOCKET_SCHEDULE_QUERY,

} SocketSchedule_e;

typedef enum
{
    SOCKET_CONN_IDLE,
    SOCKET_CONN_SUCCESS,
    SOCKET_CONN_ERR,
} SocketConnState_e;

typedef struct
{
    SocketSchedule_e    runFsm;
    uint8_t     runTick;
    uint8_t		ind;

} SocketScheduleInfo_s;

void socketListInit(void);
int8_t socketAdd(uint8_t sock, char *server, uint16_t port, void(*recvCb)(char *, uint16));
void socketDel(uint8_t sock);
void socketDelAll(void);

void socketResetConnState(void);


uint8_t socketGetUsedFlag(uint8_t sockId);
uint8_t socketGetConnStatus(uint8_t sockId);


void socketSetConnState(uint8_t sockId, uint8_t state);
void socketRecv(uint8_t sockId, char *data, uint16_t len);

void socketSchedule(void);

#endif /* TASK_INC_APP_SOCKET_H_ */
