/******************** (C) COPYRIGHT 2021 MiraMEMS *****************************
* File Name     : mir3da.c
* Author        : dhzhou@miramems.com
* Version       : V1.0
* Date          : 15/01/2021
* Description   : Demo for configuring mir3da
*******************************************************************************/
#include "app_mir3da.h"
#include <stdio.h>
#include "app_port.h"
#include "app_sys.h"


u8_m i2c_addr = 0x26;

s16_m offset_x = 0, offset_y = 0, offset_z = 0;

s8_m mir3da_register_read(u8_m addr, u8_m *data_m, u8_m len)
{
    //To do i2c read api
    iicReadData(i2c_addr<<1, addr, data_m, len);
    //LogPrintf(DEBUG_ALL, "iicRead %s",ret?"Success":"Fail");
    return 0;
}

s8_m mir3da_register_write(u8_m addr, u8_m data_m)
{
    //To do i2c write api
    iicWriteData(i2c_addr<<1, addr, data_m);
    //LogPrintf(DEBUG_ALL, "iicWrite %s",ret?"Success":"Fail");
    return 0;

}

s8_m mir3da_register_mask_write(u8_m addr, u8_m mask, u8_m data)
{
    int res = 0;
    unsigned char tmp_data = 0;

    res = mir3da_register_read(addr, &tmp_data, 1);
    tmp_data &= ~mask;
    tmp_data |= data & mask;
    res |= mir3da_register_write(addr, tmp_data);

    return 0;
}

//Initialization
s8_m mir3da_init()
{
    u8_m data_m = 0;
    int i;

    mir3da_register_read(REG_CHIP_ID, &data_m, 1);
    if(data_m != 0x13)
    {
        mir3da_register_read(REG_CHIP_ID, &data_m, 1);
        if(data_m != 0x13)
        {
            mir3da_register_read(REG_CHIP_ID, &data_m, 1);
            if(data_m != 0x13)
            {
                LogPrintf(DEBUG_ALL,"mir3da_init==>Read chip error=%X",data_m);
                return -1;
            }
        }
    }
    LogPrintf(DEBUG_ALL, "------mir3da chip id = %x-----", data_m);

    //softreset must be done first after power on
    mir3da_register_write(REG_SPI_CONFIG, 0x81 | 0x24);
    

    mir3da_register_write(REG_RESOLUTION_RANGE, 0x00);	//0x0F; +/-2G,14bit
    mir3da_register_write(REG_ODR_AXIS, 0x07);			//0x10; ODR=125hz
    mir3da_register_write(REG_MODE_BW, 0xB4);			//0x11; suspend mode, osr=8x, bw=1/10odr, autosleep disable

    //load_offset_from_filesystem(offset_x, offset_y, offset_z);	//pseudo-code

    //private register, key to close internal pull up.
    mir3da_register_write(0x7F, 0x83);
    mir3da_register_write(0x7F, 0x69);
    mir3da_register_write(0x7F, 0xBD);

    //mir3da_register_mask_write(0x8F, 0x02, 0x00);	//don't pull up i2c sda/scl pin if needed
    //mir3da_register_mask_write(0x8C, 0x80, 0x00);	//don't pull up cs pin if needed

    if (i2c_addr == 0x26)
    {
        mir3da_register_mask_write(0x8C, 0x40, 0x00);	//must close pin1 sdo pull up
    }

    return 0;
}

//enable/disable the chip
void mir3da_set_enable(int enable)
{
    if (enable)
        mir3da_register_mask_write(REG_MODE_BW, 0x80, 0x00);	//power on
    else
        mir3da_register_mask_write(REG_MODE_BW, 0x80, 0x80);

    
}

//Read three axis data, 2g range, counting 12bit, 1G=1000mg=1024lsb
void mir3da_read_raw_data(short *x, short *y, short *z)
{
    u8_m tmp_data[6] = {0};

    mir3da_register_read(REG_ACC_X_LSB, tmp_data, 6);

    *x = ((short)(tmp_data[1] << 8 | tmp_data[0])) >> 4;
    *y = ((short)(tmp_data[3] << 8 | tmp_data[2])) >> 4;
    *z = ((short)(tmp_data[5] << 8 | tmp_data[4])) >> 4;
}

void mir3da_read_data(short *x, short *y, short *z)
{
    //    u8_m tmp_data[6] = {0};
    //
    //    mir3da_register_read(REG_ACC_X_LSB, tmp_data, 6);
    //
    //    *x = ((short)(tmp_data[1] << 8 | tmp_data[0])) >> 4 - offset_x;
    //    *y = ((short)(tmp_data[3] << 8 | tmp_data[2])) >> 4 - offset_y;
    //    *z = ((short)(tmp_data[5] << 8 | tmp_data[4])) >> 4 - offset_z;
}

//open active interrupt
void mir3da_set_active_interrupt_enable(int enable)
{
    if (enable)
    {
        mir3da_register_write(REG_ACTIVE_DUR, 0x00);
        mir3da_register_write(REG_ACTIVE_THS, 0x14);
        mir3da_register_write(REG_INT_MAP1, 0x04);
        mir3da_register_write(REG_INT_SET1, 0x87);
    }
    else
    {
        mir3da_register_write(REG_INT_SET1, 0x00);
        mir3da_register_write(REG_INT_MAP1, 0x00);
    }
}

//MUST ODR >= 31.25hz
void mir3da_set_step_counter_open(int enable)
{
    if (enable)
    {
        mir3da_register_write(REG_STEP_CONGIF1, 0x01);
        mir3da_register_write(REG_STEP_CONGIF2, 0x62);  //0x22 ¡È√Ù
        mir3da_register_write(REG_STEP_CONGIF3, 0x46);
        mir3da_register_write(REG_STEP_CONGIF4, 0x32);
        mir3da_register_write(REG_STEP_FILTER, 0xa2);
    }
    else
    {
        mir3da_register_write(REG_STEP_FILTER, 0x22);
    }
}

void mir3da_reset_step_counter()
{
    mir3da_register_mask_write(REG_RESET_STEP, 0x80, 0x80);
}

u16_m mir3da_get_step()
{
    u8_m tmp_data[2] = {0};
    u16_m f_step;
    mir3da_register_read(REG_STEPS_MSB, tmp_data, 2);
    f_step = (tmp_data[0] << 8 | tmp_data[1]) / 2;
    return f_step;
}

//open significant motion interrupt
void mir3da_set_sm_interrupt_enable(int enable)
{
    if (enable)
    {
        mir3da_set_step_counter_open(1);   //sm base on step INT
        mir3da_register_write(REG_SM_THRESHOLD, 0xFF);    //256 steps
        mir3da_register_write(REG_INT_MAP1, 0x80);
        mir3da_register_write(REG_INT_SET0, 0x02);
    }
    else
    {
        mir3da_set_step_counter_open(0);
        mir3da_register_write(REG_INT_SET0, 0x00);
        mir3da_register_write(REG_INT_MAP1, 0x00);
    }
}

//open free fall interrupt
void mir3da_set_new_data_interrupt_enable(int enable)
{
    if (enable)
    {
        mir3da_register_write(REG_INT_MAP2, 0x01);
        mir3da_register_write(REG_INT_SET2, 0x10);
    }
    else
    {
        mir3da_register_write(REG_INT_SET1, 0x00);
        mir3da_register_write(REG_INT_MAP1, 0x00);
    }
}

//open free fall interrupt
void mir3da_set_ff_interrupt_enable(int enable)
{
    if (enable)
    {
        //        mir3da_register_write(FREEFALL_DUR, 0x09);
        //        mir3da_register_write(FREEFALL_THS, 0x30);
        //        mir3da_register_write(FREEFALL_HYST, 0x01);
        mir3da_register_write(REG_INT_MAP1, 0x01);
        mir3da_register_write(REG_INT_SET1, 0x80);
        mir3da_register_write(REG_INT_SET2, 0x08);
    }
    else
    {
        mir3da_register_write(REG_INT_SET1, 0x00);
        mir3da_register_write(REG_INT_MAP1, 0x00);
    }
}

//open singel tap interrupt
void mir3da_set_s_tap_interrupt_enable(int enable)
{
    if (enable)
    {
        mir3da_register_write(REG_TAP_DUR, 0x04);
        mir3da_register_write(REG_TAP_THS, 0x0A);
        mir3da_register_write(REG_INT_MAP1, 0x20);
        mir3da_register_write(REG_INT_SET1, 0x20);
    }
    else
    {
        mir3da_register_write(REG_INT_SET1, 0x00);
        mir3da_register_write(REG_INT_MAP1, 0x00);
    }
}

//open double tap interrupt
void mir3da_set_d_tap_interrupt_enable(int enable)
{
    if (enable)
    {
        mir3da_register_write(REG_TAP_DUR, 0x04);
        mir3da_register_write(REG_TAP_THS, 0x0A);
        mir3da_register_write(REG_INT_MAP1, 0x10);
        mir3da_register_write(REG_INT_SET1, 0x10);
    }
    else
    {
        mir3da_register_write(REG_INT_SET1, 0x00);
        mir3da_register_write(REG_INT_MAP1, 0x00);
    }
}

//open orientation interrupt
void mir3da_set_orient_interrupt(int enable)
{
    if (enable)
    {
        mir3da_register_write(REG_INT_SET1, 0x88);
        mir3da_register_write(REG_ORIENT_HYST, 0x18);
        mir3da_register_write(REG_Z_BLOCK, 0x88);
        mir3da_register_write(REG_INT_MAP1, 0x40);
    }
    else
    {
        mir3da_register_write(REG_INT_SET1, 0x00);
        mir3da_register_write(REG_INT_MAP1, 0x00);
    }
}

void mir3da_set_fifo_interrupt_enable(int enable)
{
    if (enable)
    {
        mir3da_register_write(REG_FIFO_CTRL, 0x80 | 0x0A);	//0x14; stream mode, 11 samples
        mir3da_register_write(REG_INT_SET0, 0x08);
        mir3da_register_write(REG_INT_MAP2, 0x02);
    }
    else
    {
        mir3da_register_write(REG_INT_SET0, 0x00);
        mir3da_register_write(REG_INT_MAP2, 0x00);
    }
}

void mir3da_read_fifo_data_lsb(short *x, short *y, short *z)
{
    u8_m tmp_data[6] = {0};
    u8_m data;
    int fifo_length, i;

    mir3da_register_read(REG_FIFO_STATUS, &data, 1);
    if (1 == (data & 0x80))
        ;//printf("watermark int");
    if (1 == (data & 0x40))
        ;//printf("fifo full int");

    fifo_length = data & 0x3F;

    for (i = 0; i < fifo_length; i++)
    {
        mir3da_register_read(REG_ACC_X_LSB, tmp_data, 6);

        *(x + i) = ((short)(tmp_data[1] << 8 | tmp_data[0])) >> 4;
        *(y + i) = ((short)(tmp_data[3] << 8 | tmp_data[2])) >> 4;
        *(z + i) = ((short)(tmp_data[5] << 8 | tmp_data[4])) >> 4;
    }
}

void mir3da_read_fifo_data_msb(short *x, short *y, short *z)
{
    u8_m tmp_data[6] = {0};
    u8_m data;
    int fifo_length, i;

    mir3da_register_read(REG_FIFO_STATUS, &data, 1);
    if (1 == (data & 0x80))
        ;//printf("watermark int");
    if (1 == (data & 0x40))
        ;//printf("fifo full int");

    fifo_length = data & 0x3F;

    for (i = 0; i < fifo_length; i++)
    {
        mir3da_register_read(REG_ACC_Z_MSB, tmp_data, 6);

        *(x + i) = ((short)(tmp_data[4] << 8 | tmp_data[5])) >> 4;
        *(y + i) = ((short)(tmp_data[2] << 8 | tmp_data[3])) >> 4;
        *(z + i) = ((short)(tmp_data[0] << 8 | tmp_data[1])) >> 4;
    }
}

s8_m read_gsensor_id(void)
{
    u8_m data_m = 0;
    //Retry 3 times
    mir3da_register_read(REG_CHIP_ID, &data_m, 1);
    if (data_m != 0x13)
    {
        mir3da_register_read(REG_CHIP_ID, &data_m, 1);
        if (data_m != 0x13)
        {
            mir3da_register_read(REG_CHIP_ID, &data_m, 1);
            if (data_m != 0x13)
            {
                LogPrintf(DEBUG_FACTORY, "Read gsensor chip id error =%x", data_m);
                return -1;
            }
        }
    }
    LogPrintf(DEBUG_FACTORY, "GSENSOR Chk. ID=0x%X", data_m);
    LogMessage(DEBUG_FACTORY, "GSENSOR CHK OK");
    return 0;
}
s8_m readInterruptConfig(void)
{
    uint8_t data_m = 0;
    mir3da_register_read(REG_INT_SET1, &data_m, 1);
    if (data_m != 0x87)
    {
        mir3da_register_read(REG_INT_SET1, &data_m, 1);
        if (data_m != 0x87)
        {
            LogPrintf(DEBUG_ALL, "Gsensor fail %x", data_m);
            return -1;
        }
    }
    LogPrintf(DEBUG_ALL, "Gsensor OK");
    return 0;
}

void startStep(void)
{
    mir3da_set_step_counter_open(1);
}
void stopStep(void)
{
    mir3da_set_step_counter_open(0);
}
u16_m getStep(void)
{
    u16_m step;
    step = mir3da_get_step();
    return step;
}
void clearStep(void)
{
    mir3da_reset_step_counter();
}
