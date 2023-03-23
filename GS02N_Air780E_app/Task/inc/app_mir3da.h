#ifndef APP_MIR3DA
#define APP_MIR3DA


#include "app_mir3da.h"
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


/*******************************************************************************
Macro definitions - Register define for Gsensor
********************************************************************************/
#define REG_SPI_CONFIG              0x00
#define REG_CHIP_ID                 0x01
#define REG_ACC_X_LSB               0x02
#define REG_ACC_X_MSB               0x03
#define REG_ACC_Y_LSB               0x04
#define REG_ACC_Y_MSB               0x05
#define REG_ACC_Z_LSB               0x06
#define REG_ACC_Z_MSB               0x07
#define REG_FIFO_STATUS             0x08
#define REG_MOTION_FLAG             0x09
#define REG_NEWDATA_FLAG            0x0A
#define REG_TAP_ACTIVE_STATUS       0x0B
#define REG_ORIENT_STATUS	        0x0C
#define REG_STEPS_MSB               0x0D
#define REG_STEPS_LSB               0x0E
#define REG_RESOLUTION_RANGE        0x0F
#define REG_ODR_AXIS                0x10
#define REG_MODE_BW                 0x11
#define REG_SWAP_POLARITY           0x12
#define REG_FIFO_CTRL               0x14
#define REG_INT_SET0                0x15
#define REG_INT_SET1                0x16
#define REG_INT_SET2                0x17
#define REG_INT_MAP1                0x19
#define REG_INT_MAP2                0x1a
#define REG_INT_MAP3                0x1b
#define REG_INT_CONFIG              0x20
#define REG_INT_LATCH               0x21
#define REG_FREEFALL_DUR            0x22
#define REG_FREEFALL_THS            0x23
#define REG_FREEFALL_HYST           0x24
#define REG_ACTIVE_DUR              0x27
#define REG_ACTIVE_THS              0x28
#define REG_TAP_DUR                 0x2A
#define REG_TAP_THS                 0x2B
#define REG_ORIENT_HYST             0x2C
#define REG_Z_BLOCK                 0x2D
#define REG_RESET_STEP				0x2E
#define REG_STEP_CONGIF1			0x2F
#define REG_STEP_CONGIF2			0x30
#define REG_STEP_CONGIF3			0x31
#define REG_STEP_CONGIF4			0x32
#define REG_STEP_FILTER             0x33
#define	REG_SM_THRESHOLD            0x34

#define DIFF_CALIBRATION 1

s8_m mir3da_init();
void mir3da_set_enable(int enable);
void mir3da_set_active_interrupt_enable(int enable);



s8_m read_gsensor_id(void);
s8_m readInterruptConfig(void);

void startStep(void);
void stopStep(void);
u16_m getStep(void);
void clearStep(void);
#endif
