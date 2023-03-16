/********************************** (C) COPYRIGHT *******************************
 * File Name          : SLEEP.c
 * Author             : WCH
 * Version            : V1.2
 * Date               : 2022/01/18
 * Description        : 睡眠配置及其初始化
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

/******************************************************************************/
/* 头文件包含 */
#include "HAL.h"


/*
 * 0:禁止睡眠
 * 1:运行睡眠
 */
static uint8_t sleepCtl=0;

void sleepSetState(uint8_t s)
{
    sleepCtl=s;
}

uint8_t getSleepState(void)
{
	return sleepCtl;
}
/*******************************************************************************
 * @fn          CH58X_LowPower
 *
 * @brief       启动睡眠
 *
 * @param   time    - 唤醒的时间点（RTC绝对值）
 *
 * @return      state.
 */
uint32_t CH58X_LowPower(uint32_t time)
{
#if(defined(HAL_SLEEP)) && (HAL_SLEEP == TRUE)
    uint32_t tmp, irq_status;
    if(sleepCtl==0)
        return 0;
    SYS_DisableAllIrq(&irq_status);
    tmp = RTC_GetCycle32k();
    if((time < tmp) || ((time - tmp) < 30))
    { // 检测睡眠的最短时间
        SYS_RecoverIrq(irq_status);
        return 2;
    }
    RTC_SetTignTime(time);
    SYS_RecoverIrq(irq_status);
    // LOW POWER-sleep模式
    if(!RTCTigFlag)
    {
        LowPower_Sleep(RB_PWR_RAM2K | RB_PWR_RAM30K | RB_PWR_EXTEND);
        if(!RTCTigFlag) // 注意如果使用了RTC以外的唤醒方式，需要注意此时32M晶振未稳定
        {
            time += WAKE_UP_RTC_MAX_TIME;
            if(time > 0xA8C00000)
            {
                time -= 0xA8C00000;
            }
            RTC_SetTignTime(time);
            LowPower_Idle();
        }
        HSECFG_Current(HSE_RCur_100); // 降为额定电流(低功耗函数中提升了HSE偏置电流)
    }
    else
    {
        return 3;
    }
#endif
    return 0;
}

/*******************************************************************************
 * @fn      HAL_SleepInit
 *
 * @brief   配置睡眠唤醒的方式   - RTC唤醒，触发模式
 *
 * @param   None.
 *
 * @return  None.
 */
void HAL_SleepInit(void)
{
#if(defined(HAL_SLEEP)) && (HAL_SLEEP == TRUE)
    R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG1;
    R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG2;
    SAFEOPERATE;
    R8_SLP_WAKE_CTRL |= RB_SLP_RTC_WAKE; // RTC唤醒
    R8_RTC_MODE_CTRL |= RB_RTC_TRIG_EN;  // 触发模式
    R8_SAFE_ACCESS_SIG = 0;              //
    PFIC_EnableIRQ(RTC_IRQn);
#endif
}
