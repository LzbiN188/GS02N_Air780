/******************** (C) COPYRIGHT 2018 MiraMEMS *****************************
* File Name     : mir3da.h
* Author        : ycwang@miramems.com
* Version       : V1.0
* Date          : 05/18/2018
* Description   : Demo for configuring mir3da
*******************************************************************************/
#ifndef __MIR3DA_h
#define __MIR3DA_h

#include <stdio.h>

#include "app_port.h"
#include "app_sys.h"
#include "app_param.h"

/*******************************************************************************
Gsensor 
********************************************************************************/
#define GSENSOR_ID_DA217				0x13
#define GSENSOR_ID_QMA6100P				0x90


/*******************************************************************************
Gsensor I2C地址
********************************************************************************/
#define QMA6100P_ADDR					0x12
#define DA217_ADDR						0x26

/*******************************************************************************
DA217	Macro definitions - Register define for Gsensor asic
********************************************************************************/
#define NSA_REG_SPI_I2C                 0x00
#define NSA_REG_WHO_AM_I                0x01
#define NSA_REG_ACC_X_LSB               0x02
#define NSA_REG_ACC_X_MSB               0x03
#define NSA_REG_ACC_Y_LSB               0x04
#define NSA_REG_ACC_Y_MSB               0x05
#define NSA_REG_ACC_Z_LSB               0x06
#define NSA_REG_ACC_Z_MSB               0x07 
#define NSA_REG_MOTION_FLAG             0x09 
#define NSA_REG_G_RANGE                 0x0f
#define NSA_REG_ODR_AXIS_DISABLE        0x10
#define NSA_REG_POWERMODE_BW            0x11
#define NSA_REG_SWAP_POLARITY           0x12
#define NSA_REG_FIFO_CTRL               0x14
#define NSA_REG_INTERRUPT_SETTINGS0     0x15
#define NSA_REG_INTERRUPT_SETTINGS1     0x16
#define NSA_REG_INTERRUPT_SETTINGS2     0x17
#define NSA_REG_INTERRUPT_MAPPING1      0x19
#define NSA_REG_INTERRUPT_MAPPING2      0x1a
#define NSA_REG_INTERRUPT_MAPPING3      0x1b
#define NSA_REG_INT_PIN_CONFIG          0x20
#define NSA_REG_INT_LATCH               0x21
#define NSA_REG_ACTIVE_DURATION         0x27
#define NSA_REG_ACTIVE_THRESHOLD        0x28
#define NSA_REG_TAP_DURATION            0x2A
#define NSA_REG_TAP_THRESHOLD           0x2B
#define NSA_REG_ENGINEERING_MODE        0x7f
#define NSA_REG_SENS_COMP               0x8c
#define NSA_REG_MEMS_OPTION             0x8f
#define NSA_REG_CHIP_INFO               0xc0

#define NSA_REG_STEPS_MSB				0x0D
#define NSA_REG_STEPS_LSB				0x0E
#define NSA_REG_RESET_STEP			    0x2E
#define NSA_REG_STEP_CONGIF1			0x2F
#define NSA_REG_STEP_CONGIF2			0x30
#define NSA_REG_STEP_CONGIF3			0x31
#define NSA_REG_STEP_CONGIF4			0x32
#define NSA_REG_STEP_FILTER             0x33

/*******************************************************************************
QMA6100P	Macro definitions - Register define for Gsensor asic
********************************************************************************/
#define QMA6100P_REG_CHIP_ID            0x00

#define QMA6100P_REG_XOUTL              0x01
#define QMA6100P_REG_XOUTH              0x02
#define QMA6100P_REG_YOUTL              0x03
#define QMA6100P_REG_YOUTH              0x04
#define QMA6100P_REG_ZOUTL              0x05
#define QMA6100P_REG_ZOUTH              0x06

#define QMA6100P_REG_STEP_CNT_L         0x07
#define QMA6100P_REG_STEP_CNT_M         0x08
#define QMA6100P_REG_STEP_CNT_H         0x0d

#define QMA6100P_REG_INT_STATUS_0       0x09
#define QMA6100P_REG_INT_STATUS_1       0x0a
#define QMA6100P_REG_INT_STATUS_2       0x0b
#define QMA6100P_REG_INT_STATUS_3       0x0c

#define QMA6100P_REG_FIFO_STATE         0x0e

#define QMA6100P_REG_RANGE              0x0f

#define QMA6100P_REG_BW_ODR             0x10

#define QMA6100P_REG_POWER_MANAGE       0x11

#define QMA6100P_REG_STEP_SAMPLE_CNT    0x12
#define QMA6100P_REG_STEP_PRECISION     0x13
#define QMA6100P_REG_STEP_TIME_LOW      0x14
#define QMA6100P_REG_STEP_TIME_UP       0x15

#define QMA6100P_REG_INT_EN_0           0x16
#define QMA6100P_REG_INT_EN_1           0x17
#define QMA6100P_REG_INT_EN_2           0x18

#define QMA6100P_REG_INT1_MAP_0         0x19
#define QMA6100P_REG_INT1_MAP_1         0x1a
#define QMA6100P_REG_INT2_MAP_0         0x1b
#define QMA6100P_REG_INT2_MAP_1         0x1c

#define QMA6100P_REG_INTPIN_CFG         0x20

#define QMA6100P_REG_INT_CFG            0x21

#define QMA6100P_REG_MOT_CFG0			0x2c
#define QMA6100P_REG_MOT_CFG1			0x2d
#define QMA6100P_REG_MOT_CFG2			0x2e



#define QMA6100P_REG_OS_CUST_X          0x27
#define QMA6100P_REG_OS_CUST_Y          0x28
#define QMA6100P_REG_OS_CUST_Z          0x29

#define QMA6100P_REG_RST_MOT			0x30

#define QMA6100P_REG_NVM                0x33
#define QMA6100P_REG_RESET              0x36


#define QMA6100P_REG_DRDY_BIT           0x10    // enable 1

#define QMA6100P_REG_AMD_X_BIT          0x01
#define QMA6100P_REG_AMD_Y_BIT          0x02
#define QMA6100P_REG_AMD_Z_BIT          0x04


/*******************************************************************************
Typedef definitions
********************************************************************************/
#define ARM_BIT_8               0

#if ARM_BIT_8
//如下数据类型是在8位机上定义的，在其它平台（比如32位）可能存在差别，需要根据实际情况修改 。 
typedef unsigned char    u8_m;                   /* 无符号8位整型变量*/
typedef signed   char    s8_m;                   /* 有符号8位整型变量*/
typedef unsigned int     u16_m;                  /* 无符号16位整型变量*/
typedef signed   int     s16_m;                  /* 有符号16位整型变量*/
typedef unsigned long    u32_m;                  /* 无符号32位整型变量*/
typedef signed   long    s32_m;                  /* 有符号32位整型变量*/
typedef float            fp32_m;                 /* 单精度浮点数（32位长度）*/
typedef double           fp64_m;                 /* 双精度浮点数（64位长度）*/
#else
//如下数据类型是在32位机上定义的，在其它平台（比如8位）可能存在差别，需要根据实际情况修改 。 
typedef unsigned char    u8_m;                   /* 无符号8位整型变量*/
typedef signed   char    s8_m;                   /* 有符号8位整型变量*/
typedef unsigned short   u16_m;                  /* 无符号16位整型变量*/
typedef signed   short   s16_m;                  /* 有符号16位整型变量*/
typedef unsigned int     u32_m;                  /* 无符号32位整型变量*/
typedef signed   int     s32_m;                  /* 有符号32位整型变量*/
typedef float            fp32_m;                 /* 单精度浮点数（32位长度）*/
typedef double           fp64_m;                 /* 双精度浮点数（64位长度）*/
#endif

typedef struct AccData_tag{
   s16_m ax;                                   //加速度计原始数据结构体  数据格式 0 0 1024
   s16_m ay;
   s16_m az;
}AccData;

#define mir3da_abs(x)          (((x) > 0) ? (x) : (-(x)))

s8_m mir3da_init(void);
s8_m mir3da_read_data(s16_m *x, s16_m *y, s16_m *z);
s8_m mir3da_open_interrupt(u8_m th);
s8_m mir3da_close_interrupt(void);
s8_m mir3da_set_enable(u8_m enable);
u8_m read_gsensor_id(void);
s8_m readInterruptConfig(void);

void startStep(void);
void stopStep(void);
u16_m getStep(void);
void clearStep(void);


s8_m qma6100p_init(void);
s8_m da217_init(void);


#endif
