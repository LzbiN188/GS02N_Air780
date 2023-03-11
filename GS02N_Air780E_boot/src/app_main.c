#include "CH58x_common.h"
#include "app_port.h"
#include "app_sys.h"
#include "app_kernal.h"
#include "app_param.h"
#include "app_net.h"
#include "app_update.h"


/**************************************************
@bref       LED
@param
@return
@note
**************************************************/

void ledRunTask(void)
{
    static uint8_t flag = 0;
    if (flag ^= 0x01)
    {
        LED1_ON;
    }
    else
    {
        LED1_OFF;
    }
}


/**************************************************
@bref       任务运行
@param
@return
@note
**************************************************/

void myTask(void)
{
    netConnectTask();
    upgradeRunTask();
}

/**************************************************
@bref       main
@param
@return
@note
**************************************************/

void main(void)
{
    SetSysClock(CLK_SOURCE_PLL_60MHz);
    portGpioSetDefCfg();
    portUartCfg(APPUSART2, 1, 115200, NULL);
    paramInit();
    if (sysparam.updateStatus == 0)
    {
        LogMessage(0, "Bootloader Done");
        portUartCfg(APPUSART2, 0, 115200, NULL);
        JumpToApp(APPLICATION_ADDRESS);
    }
    portTimerCfg();
    portModuleGpioCfg();
    portLedCfg();
    upgradeInfoInit();
    startTimer(1000, modulePowerOn, 0);
    createSystemTask(myTask,1000);
    createSystemTask(outputNode,200);
	createSystemTask(ledRunTask,100);
    createSystemTask(pollUartData, 50);
    while (1)
    {
        kernalRun();
    }

}
