/*
 * app_bleRelay.h
 *
 *  Created on: Oct 27, 2022
 *      Author: idea
 */

#ifndef TASK_INC_APP_BLERELAY_H_
#define TASK_INC_APP_BLERELAY_H_

#include "CH58x_common.h"

#define BLE_CONNECT_LIST_SIZE   2

#define CMD_GET_SHIELD_CNT        0x00  //获取屏蔽次数
#define CMD_CLEAR_SHIELD_CNT      0x01  //清除计数器，并接通继电器
#define CMD_DEV_ON                0x02  //主动接通继电器
#define CMD_DEV_OFF               0x03  //主动断开继电器
#define CMD_SET_VOLTAGE           0x04  //设置屏蔽电压阈值
#define CMD_GET_VOLTAGE           0x05  //读取屏蔽电压阈值
#define CMD_GET_ADCV              0x06  //读取屏蔽电压
#define CMD_SET_OUTVOLTAGE        0x07  //设置外部电压阈值
#define CMD_GET_OUTVOLTAGE        0x08  //读取外部电压阈值
#define CMD_GET_OUTV              0x09  //读取外部电压值
#define CMD_GET_ONOFF             0x0A  //读取继电器状态
#define CMD_ALARM                 0x0B  //报警发送
#define CMD_SPEED_FLAG            0x0C  //速度标志
#define CMD_AUTODIS               0x0D  //设置倒计时参数
#define CMD_CLEAR_ALARM           0x0E  //清除报警
#define CMD_PREALARM              0x0F  //预警发送
#define CMD_CLEAR_PREALARM        0x10  //清除预警
#define CMD_SET_PRE_ALARM_PARAM   0x13  //设置屏蔽预警参数
#define CMD_GET_PRE_ALARM_PARAM   0x14  //读取屏蔽预警参数
#define CMD_GET_DISCONNECT_PARAM  0x15  //读取倒计时参数
#define CMD_SET_RTC               0xA8//更新RTC时间


#define BLE_EVENT_GET_OUTV        0x00000001 //读取蓝牙继电器的外部电压
#define BLE_EVENT_GET_RFV         0x00000002 //读取蓝牙继电器的屏蔽电压
#define BLE_EVENT_GET_RF_THRE     0x00000004 //读取屏蔽电压阈值
#define BLE_EVENT_GET_OUT_THRE    0x00000008 //读取外部电压阈值
#define BLE_EVENT_SET_DEVON       0x00000100 //继电器on
#define BLE_EVENT_SET_DEVOFF      0x00000200 //继电器off
#define BLE_EVENT_CLR_CNT         0x00000400 //清除屏蔽次数
#define BLE_EVENT_SET_RF_THRE     0x00000800 //设置屏蔽电压阈值
#define BLE_EVENT_SET_OUTV_THRE   0x00001000 //设置acc检测电压阈值
#define BLE_EVENT_SET_AD_THRE     0x00002000 //设置自动断连参数阈值
#define BLE_EVENT_CLR_ALARM       0x00004000 //清除报警
#define BLE_EVENT_CLR_PREALARM    0x00008000 //清除预警
#define BLE_EVENT_SET_PRE_PARAM   0x00010000 //设置预警参数
#define BLE_EVENT_GET_PRE_PARAM   0x00020000 //读取预警参数
#define BLE_EVENT_GET_AD_THRE     0x00040000 //读取自动断连参数
#define BLE_EVENT_SET_RTC         0x00080000 //设置RTC参数


typedef struct
{
    float outV;         //外电电压
    float rfV;          //屏蔽电压

    float rf_threshold; //屏蔽电压阈值
    float out_threshold;//外电电压阈值

    uint32_t updateTick;//更新时间

    uint8_t bleLost;    //链路状态
    uint8_t preV_threshold;         //预警阈值
    uint8_t preDetCnt_threshold;    //预警检测次数
    uint8_t preHold_threshold;      //预警保持
    uint8_t disc_threshold;         //断连阈值
} bleRelayInfo_s;


typedef struct
{
    uint8_t addr[6];        //mac
    uint8_t addrType;       //type
    uint8_t used;           //是否已使用
    uint32_t dataReq;       //需要发送的指令类型
    bleRelayInfo_s bleInfo; //蓝牙继电器信息
} bleRelayDev_s;

void bleRelaySetReq(uint8_t ind, uint32_t req);
void bleRelaySetAllReq(uint32_t req);
void bleRelayClearReq(uint8_t ind, uint32_t req);
void bleRelayClearAllReq(uint32_t req);
bleRelayInfo_s *bleRelayGeInfo(uint8_t i);
void bleRelayDeleteAll(void);
void bleRelayInit(void);
int8_t bleRelayInsert(uint8_t *addr, uint8_t addrType);
void bleRelayRecvParser(uint8_t connHandle, uint8_t *data, uint8_t len);
void bleRelaySendDataTry(void);
void blePeriodTask(void);


#endif /* TASK_INC_APP_BLERELAY_H_ */
