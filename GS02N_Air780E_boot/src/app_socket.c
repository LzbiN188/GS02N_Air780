/*
 * app_socket.c
 *
 *  Created on: Jul 13, 2022
 *      Author: idea
 */
#include "app_sys.h"
#include "app_net.h"
#include "app_socket.h"
#include "app_update.h"
#define SOCKET_LIST_MAX_SIZE    1

SocketInfo_s socketList[SOCKET_LIST_MAX_SIZE];
SocketScheduleInfo_s sockSchInfo;

static void socketSchChange(SocketSchedule_e fsm);

/**************************************************
@bref		初始化socket信息
@param
@return
@note
**************************************************/

void socketListInit(void)
{
    memset(socketList, 0, sizeof(socketList));
    memset(&sockSchInfo, 0, sizeof(SocketScheduleInfo_s));
}

/**************************************************
@bref		添加待连接的socket
@param
@return
@note
**************************************************/

int8_t socketAdd(uint8_t sock, char *server, uint16_t port, void(*recvCb)(char *, uint16_t))
{
    if (sock >= SOCKET_LIST_MAX_SIZE)
    {
        return -1;
    }
    if (socketList[sock].usedFlag != 0)
    {
        return -2;
    }
    if (server == NULL || port == 0)
    {
        return -3;
    }
    if (server[0] == 0)
    {
        return -4;
    }
    memset(&socketList[sock], 0, sizeof(SocketInfo_s));
    //socketID 与编号用同一个
    socketList[sock].socketId = sock;
    strcpy(socketList[sock].server, server);
    socketList[sock].port = port;
    socketList[sock].recvCallBack = recvCb;
    socketList[sock].usedFlag = 1;
    return 1;
}

/**************************************************
@bref		删除socket
@param
@return
@note
**************************************************/

void socketDel(uint8_t sock)
{
    if (sock >= SOCKET_LIST_MAX_SIZE)
    {
        return ;
    }
    if (socketList[sock].usedFlag)
    {
        socketList[sock].usedFlag = 0;
        LogPrintf(DEBUG_ALL, "Delete socket[%d]", socketList[sock].socketId);
        closeSocket(socketList[sock].socketId);
    }
}

/**************************************************
@bref		删除所有socket
@param
@return
@note
**************************************************/

void socketDelAll(void)
{
    uint8_t i;
    for (i = 0; i < SOCKET_LIST_MAX_SIZE; i++)
    {
        socketDel(i);
    }
}

/**************************************************
@bref		重置socket的链接态
@param
@return
@note
**************************************************/

void socketResetConnState(void)
{
    uint8_t i;
    for (i = 0; i < SOCKET_LIST_MAX_SIZE; i++)
    {
        if (socketList[i].usedFlag && socketList[i].connOK == SOCKET_CONN_SUCCESS)
        {
            socketList[i].connOK = SOCKET_CONN_ERR;
            LogPrintf(DEBUG_ALL, "reset socket[%d] conn status", socketList[i].socketId);
        }
    }
}

/**************************************************
@bref		查看socket是否已使用
@param
@return
@note
**************************************************/

uint8_t socketGetUsedFlag(uint8_t sockId)
{
    if (sockId >= SOCKET_LIST_MAX_SIZE)
    {
        return 0;
    }
    return socketList[sockId].usedFlag;
}

/**************************************************
@bref		查看socket连接状态
@param
@return
@note
**************************************************/

uint8_t socketGetConnStatus(uint8_t sockId)
{
    if (sockId >= SOCKET_LIST_MAX_SIZE)
    {
        return 0;
    }
    return socketList[sockId].connOK;
}
/**************************************************
@bref		设置socket连接状态
@param
@return
@note
**************************************************/

void socketSetConnState(uint8_t sockId, uint8_t state)
{
    if (sockId >= SOCKET_LIST_MAX_SIZE)
    {
        return;
    }
    socketList[sockId].connOK = state;
}

/**************************************************
@bref		socket数据接收
@param
@return
@note
**************************************************/

void socketRecv(uint8_t sockId, char *data, uint16_t len)
{
    if (sockId >= SOCKET_LIST_MAX_SIZE)
    {
        return;
    }
    if (socketList[sockId].recvCallBack != NULL)
    {
        socketList[sockId].recvCallBack(data, len);
    }
}
/**************************************************
@bref		切换socket状态机
@param
@return
@note
**************************************************/

static void socketSchChange(SocketSchedule_e fsm)
{
    sockSchInfo.runFsm = fsm;
    sockSchInfo.runTick = 0;
}

/**************************************************
@bref		socket调度器
@param
@return
@note
**************************************************/

void socketSchedule(void)
{
    switch (sockSchInfo.runFsm)
    {
        case SOCKET_SCHEDULE_IDLE:

            sockSchInfo.ind %= SOCKET_LIST_MAX_SIZE;
            for (; sockSchInfo.ind < SOCKET_LIST_MAX_SIZE; sockSchInfo.ind++)
            {
                if (socketList[sockSchInfo.ind].usedFlag == 1 && socketList[sockSchInfo.ind].connOK != SOCKET_CONN_SUCCESS)
                {
                    break;
                }
            }
            if (sockSchInfo.ind >= SOCKET_LIST_MAX_SIZE)
            {
                sockSchInfo.ind = 0;
                break;
            }
            else
            {
                LogPrintf(DEBUG_ALL, "try to connect [%d]:%s", sockSchInfo.ind, socketList[sockSchInfo.ind].server);
                socketSchChange(SOCKET_SCHEDULE_CONN);
            }
        case SOCKET_SCHEDULE_CONN:
            if (sockSchInfo.runTick++ == 0)
            {
                socketList[sockSchInfo.ind].connOK = SOCKET_CONN_IDLE;
                openSocket(socketList[sockSchInfo.ind].socketId, socketList[sockSchInfo.ind].server,
                             socketList[sockSchInfo.ind].port);
            }
            if (socketList[sockSchInfo.ind].connOK == SOCKET_CONN_SUCCESS)
            {
                LogPrintf(DEBUG_ALL, "Socket[%d] connect success", sockSchInfo.ind);
                upgradeSocketSuccess();
                socketSchChange(SOCKET_SCHEDULE_END);
            }
            else if (socketList[sockSchInfo.ind].connOK == SOCKET_CONN_ERR)
            {
                LogPrintf(DEBUG_ALL, "Socket[%d] open error", sockSchInfo.ind);
                upgradeSocketErr();
                socketSchChange(SOCKET_SCHEDULE_END);

            }
            else if (sockSchInfo.runTick > 90)
            {
                LogPrintf(DEBUG_ALL, "Socket[%d] connect timeout", sockSchInfo.ind);
                upgradeSocketErr();
                socketSchChange(SOCKET_SCHEDULE_END);
            }
            else
            {
                break;
            }
        case SOCKET_SCHEDULE_END:
            if (socketList[sockSchInfo.ind].connOK != SOCKET_CONN_SUCCESS)
            {
                closeSocket(socketList[sockSchInfo.ind].socketId);
            }
            sockSchInfo.ind++;
            socketSchChange(SOCKET_SCHEDULE_IDLE);
            break;
    }
}
