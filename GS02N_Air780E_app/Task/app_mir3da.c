/******************** (C) COPYRIGHT 2018 MiraMEMS *****************************
* File Name     : mir3da.c
* Author        : ycwang@miramems.com
* Version       : V1.0
* Date          : 05/18/2018
* Description   : Demo for configuring mir3da
*******************************************************************************/
#include "app_mir3da.h"


u8_m gsensor_id = 0;

s8_m mir3da_register_read(u8_m addr, u8_m *data_m, u8_m len)
{
    //To do i2c read api
    iicReadData(DA217_ADDR<<1, addr, data_m, len);
    //LogPrintf(DEBUG_ALL, "iicRead %s",ret?"Success":"Fail");
    return 0;

}

s8_m mir3da_register_write(u8_m addr, u8_m data_m)
{
    //To do i2c write api
    iicWriteData(DA217_ADDR<<1, addr, data_m);
    //LogPrintf(DEBUG_ALL, "iicWrite %s",ret?"Success":"Fail");
    return 0;

}

s8_m qma6100p_register_read(u8_m addr, u8_m *data_m, u8_m len)
{
    //To do i2c read api
    iicReadData(QMA6100P_ADDR<<1, addr, data_m, len);
    //LogPrintf(DEBUG_ALL, "iicRead %s",ret?"Success":"Fail");
    return 0;
}

s8_m qma6100p_register_write(u8_m addr, u8_m data_m)
{
    //To do i2c write api
    iicWriteData(QMA6100P_ADDR<<1, addr, data_m);
    //LogPrintf(DEBUG_ALL, "iicWrite %s",ret?"Success":"Fail");
    return 0;
}


/**
 * 
 * @brief 初始化gsensor参数
 * @return 
 * @date 2023-06-13
 * @author lvvv
 * 
 * @details 
 * @note 
 * @par 修改日志:
 * <table>
 * <tr><th>Date 			<th>Author		   <th>Description
 * <tr><td>2023-6-13	   <td>lvvv 			  <td>新建
 * </table>
 */
void UpdateGsensorDet(uint8_t type)
{
	if (sysparam.gsdettime != 0 || sysparam.gsValidCnt != 0 || sysparam.gsInvalidCnt != 0)
	{
		return;
	}
	if (type == GSENSOR_ID_DA217)
	{
		sysparam.gsdettime=15;
	    sysparam.gsValidCnt=7;
	    sysparam.gsInvalidCnt=0;
	    LogPrintf(DEBUG_ALL, "UpdateGsensorDet==> %d,%d,%d", sysparam.gsdettime, sysparam.gsValidCnt, sysparam.gsInvalidCnt);
    }
    else
    {
		sysparam.gsdettime=15;
	    sysparam.gsValidCnt=5;
	    sysparam.gsInvalidCnt=0;
	    LogPrintf(DEBUG_ALL, "UpdateGsensorDet==> %d,%d,%d", sysparam.gsdettime, sysparam.gsValidCnt, sysparam.gsInvalidCnt);
    }
    paramSaveAll();
}

/**
 * 
 * @brief 初始化gsensor
 * @return s8_m
 * @date 2023-06-12
 * @author lvvv
 * 
 * @details 
 * @note 
 * @par 修改日志:
 * <table>
 * <tr><th>Date 			<th>Author		   <th>Description
 * <tr><td>2023-6-12	   <td>lvvv 			  <td>新建
 * </table>
 */

s8_m mir3da_init(void)
{
	s8_m res = 0;
	u8_m data_m = 0;

	data_m = read_gsensor_id();
	if (data_m == GSENSOR_ID_DA217)
	{
		LogPrintf(DEBUG_ALL,"mir3da_init[DA217]==>Read chip id=0x%x",data_m);
	}
	else if (data_m == GSENSOR_ID_QMA6100P)
	{
		LogPrintf(DEBUG_ALL,"mir3da_init[QMA6100P]==>Read chip id=0x%x",data_m);
	}
	else
	{
		LogPrintf(DEBUG_FACTORY,"Read gsensor chip id error =0x%x",data_m);
	}
	switch (gsensor_id)
	{
		case GSENSOR_ID_DA217:
			da217_init();
			UpdateGsensorDet(GSENSOR_ID_DA217);
			break;
		case GSENSOR_ID_QMA6100P:
			qma6100p_init();
			UpdateGsensorDet(GSENSOR_ID_QMA6100P);
			break;
		default:
			break;
	}
	return res;
}

//enable/disable the chip
s8_m mir3da_set_enable(u8_m enable)
{
	s8_m res = 0;
	if(enable)
		res = mir3da_register_write(NSA_REG_POWERMODE_BW,0x30);
	else
		res = mir3da_register_write(NSA_REG_POWERMODE_BW,0x80);

	return res;
}

//Read three axis data, 1024 LSB = 1 g
s8_m mir3da_read_data(s16_m *x, s16_m *y, s16_m *z)
{
	u8_m    tmp_data[6] = {0};

	if (mir3da_register_read(NSA_REG_ACC_X_LSB, tmp_data,6) != 0)
	{
		return -1;
	}

	*x = ((s16_m)(tmp_data[1] << 8 | tmp_data[0]))>> 4;
	*y = ((s16_m)(tmp_data[3] << 8 | tmp_data[2]))>> 4;
	*z = ((s16_m)(tmp_data[5] << 8 | tmp_data[4]))>> 4;

	return 0;
}

//open active interrupt
s8_m mir3da_open_interrupt(u8_m th)
{
	s8_m   res = 0;

	res = mir3da_register_write(NSA_REG_INTERRUPT_SETTINGS1,0x87);
	res = mir3da_register_write(NSA_REG_ACTIVE_DURATION,0x00 );
	res = mir3da_register_write(NSA_REG_ACTIVE_THRESHOLD,th);
	res = mir3da_register_write(NSA_REG_INTERRUPT_MAPPING1,0x04 );

	return res;
}

//close active interrupt
s8_m mir3da_close_interrupt(void)
{
	s8_m   res = 0;

	res = mir3da_register_write(NSA_REG_INTERRUPT_SETTINGS1,0x00 );
	res = mir3da_register_write(NSA_REG_INTERRUPT_MAPPING1,0x00 );

	return res;
}



/**
 * 
 * @brief 获取gsensorid
 * @return u8_m
 * @date 2023-06-12
 * @author lvvv
 * 
 * @details 
 * @note 
 * @par 修改日志:
 * <table>
 * <tr><th>Date 			<th>Author		   <th>Description
 * <tr><td>2023-6-12	   <td>lv 			  <td>新建
 * </table>
 */

u8_m read_gsensor_id(void)
{
	s8_m res = 0;
	u8_m data_m = 0;

	res = mir3da_register_read(NSA_REG_WHO_AM_I,&data_m,1);
	if(data_m != GSENSOR_ID_DA217)
	{
		res = mir3da_register_read(NSA_REG_WHO_AM_I,&data_m,1);
		if(data_m != GSENSOR_ID_DA217)
		{
			res = mir3da_register_read(NSA_REG_WHO_AM_I,&data_m,1);
			if(data_m != GSENSOR_ID_DA217)
			{
				//LogPrintf(DEBUG_FACTORY,"Read gsensor chip id error =%x",data_m);
			}
		}
	}
	if (data_m == GSENSOR_ID_DA217)
	{
		gsensor_id = GSENSOR_ID_DA217;
		LogPrintf(DEBUG_FACTORY,"GSENSOR Chk. ID=0x%X\r\nGSENSOR CHK OK",data_m);
		return data_m;
	}

	res = qma6100p_register_read(QMA6100P_REG_CHIP_ID,&data_m,1);
	if(data_m != GSENSOR_ID_QMA6100P)
	{
		res = qma6100p_register_read(QMA6100P_REG_CHIP_ID,&data_m,1);
		if(data_m != GSENSOR_ID_QMA6100P)
		{
			res = qma6100p_register_read(QMA6100P_REG_CHIP_ID,&data_m,1);
			if(data_m != GSENSOR_ID_QMA6100P)
			{
				//LogPrintf(DEBUG_FACTORY,"qma6100p_init==>Read chip error=%X",data_m);
			}
		}
	}

	if (data_m == GSENSOR_ID_QMA6100P)
	{
		gsensor_id = GSENSOR_ID_QMA6100P;
		LogPrintf(DEBUG_FACTORY,"GSENSOR Chk. ID=0x%X\r\nGSENSOR CHK OK",data_m);
		return data_m;
	}
	else
	{
		LogPrintf(DEBUG_FACTORY,"Read gsensor chip id error =%x",data_m);
		return 0;
	}

}

s8_m readInterruptConfig(void)
{
    uint8_t data_m;
    if (gsensor_id == GSENSOR_ID_DA217)
    {
	    mir3da_register_read(NSA_REG_ODR_AXIS_DISABLE,&data_m,1);
	    if(data_m != 0x06)
	    {
	        mir3da_register_read(NSA_REG_ODR_AXIS_DISABLE,&data_m,1);
	        if(data_m != 0x06)
	        {
	           return -1;
	        }
	    }
	    LogPrintf(DEBUG_ALL,"Gsensor OK", gsensor_id);
	    return 0;
    }
    else if (gsensor_id == GSENSOR_ID_QMA6100P)
    {
		qma6100p_register_read(QMA6100P_REG_INT_EN_2, &data_m, 1);
		if (data_m != 0x07)
		{
			qma6100p_register_read(QMA6100P_REG_INT_EN_2, &data_m, 1);
			if (data_m != 0x07)
			{
				return -1;
			}
		}
		LogPrintf(DEBUG_ALL,"Gsensor OK", data_m);
		return 0;
    }
    else
    {
    	return -1;
    }
}


void startStep(void)
{
    mir3da_register_write(NSA_REG_STEP_CONGIF1, 0x01);
    mir3da_register_write(NSA_REG_STEP_CONGIF2, 0x62);
    mir3da_register_write(NSA_REG_STEP_CONGIF3, 0x46);
    mir3da_register_write(NSA_REG_STEP_CONGIF4, 0x32);
    mir3da_register_write(NSA_REG_STEP_FILTER, 0xa2);

}
void stopStep(void)
{
	mir3da_register_write(NSA_REG_STEP_FILTER, 0x22);

}
u16_m getStep(void)
{
    uint8_t data[2];
    uint16_t va;
    mir3da_register_read(NSA_REG_STEPS_MSB,data,2);
    va=data[0]<<8|data[1];
    return va;
}

void clearStep(void)
{
	mir3da_register_write(NSA_REG_RESET_STEP, 0x80);	
}

/**
 * 
 * @brief 初始化qma6100p
 * @return s8_m
 * @date 2023-06-12
 * @author lvvv
 * 
 * @details 
 * @note 
 * @par 修改日志:
 * <table>
 * <tr><th>Date 			<th>Author		   <th>Description
 * <tr><td>2023-6-12	   <td>lvvv 			  <td>新建
 * </table>
 */

s8_m qma6100p_init(void)
{
	s8_m res = 0;
	u8_m gsensor_id = 0;

	u8_m reg_0x18 = 0;
    u8_m reg_0x1a = 0;
    u8_m reg_0x2c = 0;
	u8_m reg_0x33 = 0;
	u8_m reg_0x11 = 0;
    s8_m param = 0;
    u8_m  retry = 0;
	
	//复位初始化流程
	//--------------------------------------------------------
    qma6100p_register_write(QMA6100P_REG_RESET, 0xb6);    //软复位所有寄存器
    DelayMs(5);
    qma6100p_register_write(QMA6100P_REG_RESET, 0x00);    //软复位后，用户应写入0x00
    DelayMs(100);

    retry = 0;
    while (reg_0x11 != 0x84) 
    {
        qma6100p_register_write(QMA6100P_REG_POWER_MANAGE, 0x84);  //开机后默认模式为待机模式，设置为活动模式
        DelayMs(2);
        qma6100p_register_read(QMA6100P_REG_POWER_MANAGE, &reg_0x11, 1);
        //LogPrintf(DEBUG_ALL, "confirm read 0x11 = 0x%x\n", reg_0x11);
        if (retry++ > 100)
        {
            break;
        }    
    }
    // 0x33是内部寄存器 按照demo去初始化 默认值0x05
    qma6100p_register_write(0x33, 0x08);  //开启NVM映射 
    DelayMs(5);
    retry = 0;
    while ((reg_0x33 & 0x05) != 0x05) 
    {
        qma6100p_register_read(0x33, &reg_0x33, 1);
        //LogPrintf(DEBUG_ALL, "confirm read 0x33 = 0x%x\n", reg_0x33);
        DelayMs(2);
        if (retry++ > 100)
        {
             break;
        }           
    }
	
    //LogPrintf(DEBUG_ALL, "confirm read 0x33 = 0x%x\n", reg_0x33);
	//常规配置
	//--------------------------------------------------------
	qma6100p_register_write(0x4A, 0x20);
	qma6100p_register_write(0x50, 0x51);  //0x49  0x51
	qma6100p_register_write(0x56, 0x01);
	
	qma6100p_register_write(QMA6100P_REG_RANGE, 0x01);			//2G
	qma6100p_register_write(QMA6100P_REG_BW_ODR, 0x05); 		//no LPF
	qma6100p_register_write(QMA6100P_REG_POWER_MANAGE, 0x84);	//active mode
	qma6100p_register_write(0x4A, 0x28);						// force  I2C interface
	DelayMs(5);
	qma6100p_register_write(QMA6100P_REG_INTPIN_CFG, 0x05);		//开启上拉,中断输出低电平
	DelayMs(5);
	qma6100p_register_write(0x5F, 0x80);
	DelayMs(5);
	qma6100p_register_write(0x5F, 0x00);
	DelayMs(5);

    qma6100p_register_read(QMA6100P_REG_INT_EN_2, &reg_0x18, 1);
    qma6100p_register_read(QMA6100P_REG_INT1_MAP_1, &reg_0x1a, 1);
    qma6100p_register_read(QMA6100P_REG_MOT_CFG0, &reg_0x2c, 1);
    reg_0x2c |= 0x00;

    qma6100p_register_write(QMA6100P_REG_MOT_CFG0, reg_0x2c);
    qma6100p_register_write(QMA6100P_REG_MOT_CFG2, 0x09);  		// 0.488*16*32 = 250mg  //定义中断阈值
    qma6100p_register_write(QMA6100P_REG_RST_MOT, 0x80 | 0x3f); // default 0x3f //绕过LPF过滤

    reg_0x18 |= 0x07;
    reg_0x1a |= 0x01;
    qma6100p_register_write(QMA6100P_REG_INT_EN_2, reg_0x18);	//enable any motion 任何运动 xyz 轴中断使能
    qma6100p_register_write(QMA6100P_REG_INT1_MAP_1, reg_0x1a); //将任何运动中断映射到INT1引脚
	
	return 1;
}
/**
 * 
 * @brief 初始化da217
 * @return s8_m
 * @date 2023-06-12
 * @author lvvv
 * 
 * @details 
 * @note 
 * @par 修改日志:
 * <table>
 * <tr><th>Date 			<th>Author		   <th>Description
 * <tr><td>2023-6-12	   <td>lvvv 			  <td>新建
 * </table>
 */
s8_m da217_init(void)
{
	s8_m res = 0;
	res |= mir3da_register_write(NSA_REG_SPI_I2C, 0x24);
	//HAL_Delay(20);

	res |= mir3da_register_write(NSA_REG_G_RANGE, 0x00);               //+/-2G,14bit
	res |= mir3da_register_write(NSA_REG_POWERMODE_BW, 0x34);          //normal mode
	res |= mir3da_register_write(NSA_REG_ODR_AXIS_DISABLE, 0x06);      //ODR = 62.5hz

	//Engineering mode
	res |= mir3da_register_write(NSA_REG_ENGINEERING_MODE, 0x83);
	res |= mir3da_register_write(NSA_REG_ENGINEERING_MODE, 0x69);
	res |= mir3da_register_write(NSA_REG_ENGINEERING_MODE, 0xBD);
	mir3da_register_write(NSA_REG_SENS_COMP, 0x00);

   	mir3da_open_interrupt(10);
    mir3da_set_enable(1);
}

