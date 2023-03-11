/*
 * app_broadcaster.c
 *
 *  Created on: Feb 23, 2022
 *      Author: idea
 */
#include "config.h"
#include "app_broadcaster.h"

static uint8 appBroadcastTaskId = INVALID_TASK_ID;
static gapRolesBroadcasterCBs_t appBroadcasterStruct;


static tmosEvents appBroadcasterEventProcess(tmosTaskID taskID, tmosEvents event);
static void appBroadcasterNofifyCallBack(gapRole_States_t newState);


/**
 * @brief               配置广播信息
 * @param
 *      broadcastnmae   广播名称
 * @return  none
 */
void appBroadcastCfg(uint8 *broadcastnmae)
{
    uint8 len, advLen;
    uint8 advertData[31];
    len = tmos_strlen(broadcastnmae);

    advLen = 0;
    advertData[advLen++] = len + 1;
    advertData[advLen++] = GAP_ADTYPE_LOCAL_NAME_COMPLETE;
    tmos_memcpy(advertData + advLen, broadcastnmae, len);
    advLen += len;
	//设置广播信息
    GAPRole_SetParameter(GAPROLE_ADVERT_DATA, advLen, advertData);
}

/**
 * @brief               广播初始化
 * @param
 * @return  none
 */
void appBroadCasterInit(void)
{

    uint8 value;
    //广播初始化
    GAPRole_BroadcasterInit();
    //任务注册
    appBroadcastTaskId = TMOS_ProcessEventRegister(appBroadcasterEventProcess);
    //回调函数初始化
    appBroadcasterStruct.pfnStateChange = appBroadcasterNofifyCallBack;
    appBroadcasterStruct.pfnScanRecv = NULL;
    //使能广播
    value = ENABLE;
    GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8), &value);
    //不可连接
    value = GAP_ADTYPE_ADV_NONCONN_IND;
    GAPRole_SetParameter(GAPROLE_ADV_EVENT_TYPE, sizeof(uint8), &value);
    //设置广播信息
    appBroadcastCfg("Ch582F");
    //启动广播任务
    tmos_set_event(appBroadcastTaskId, APP_BROADCAST_START_EVENT);

}
/**
 * @brief         广播任务回调
 * @param
 *      taskID
 *      events
 * @return
 */
static tmosEvents appBroadcasterEventProcess(tmosTaskID taskID, tmosEvents events)
{
    if (events & SYS_EVENT_MSG)
    {
        uint8 *pMsg;
        if ((pMsg = tmos_msg_receive(appBroadcastTaskId)) != NULL)
        {
            tmos_msg_deallocate(pMsg);
        }
        return (events ^ SYS_EVENT_MSG);
    }
    if (events & APP_BROADCAST_START_EVENT)
    {
        GAPRole_BroadcasterStartDevice(&appBroadcasterStruct);
        return events ^ APP_BROADCAST_START_EVENT;
    }

    return 0;
}
/**
 * @brief         广播状态通知
 * @param
 *      newState
 * @return
 */
static void appBroadcasterNofifyCallBack(gapRole_States_t newState)
{
    switch (newState)
    {
        case GAPROLE_STARTED:
            LogPrintf(DEBUG_ALL,"GAPRole started");
            break;

        case GAPROLE_ADVERTISING:
            LogPrintf(DEBUG_ALL,"GAPRole advertising");
            break;

        case GAPROLE_WAITING:
            LogPrintf(DEBUG_ALL,"GAPRole waitting");
            break;

        case GAPROLE_ERROR:
            LogPrintf(DEBUG_ALL,"GAPRole error");
            break;

        default:
            break;
    }
}
