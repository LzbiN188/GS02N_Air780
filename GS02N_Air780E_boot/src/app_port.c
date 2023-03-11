/*
 * app_port.c
 *
 *  Created on: Aug 2, 2022
 *      Author: idea
 */
#include "app_port.h"
#include "app_sys.h"
#include "app_kernal.h"

static uint8_t uart0RxBuff[APPUSART0_BUFF_SIZE];
static uint8_t uart0trigB;

static uint8_t uart1RxBuff[APPUSART1_BUFF_SIZE];
static uint8_t uart1trigB;

static uint8_t uart2RxBuff[APPUSART2_BUFF_SIZE];
static uint8_t uart2trigB;

static uint8_t uart3RxBuff[APPUSART3_BUFF_SIZE];
static uint8_t uart3trigB;

UART_RXTX_CTL usart0_ctl;
UART_RXTX_CTL usart1_ctl;
UART_RXTX_CTL usart2_ctl;
UART_RXTX_CTL usart3_ctl;



/**************************************************
@bref       串口输入塞入缓冲区
@param
    uartctl     串口控制体
    buf
    len
@return
@note
**************************************************/
static int8_t pushUartRxData(UART_RXTX_CTL *uartctl, uint8_t *buf, uint16_t len)
{
    uint16_t spare, sparetoend;
    if (uartctl->rxend > uartctl->rxbegin)
    {
        spare = uartctl->rxbufsize - (uartctl->rxend - uartctl->rxbegin);
    }
    else if (uartctl->rxbegin > uartctl->rxend)
    {
        spare = uartctl->rxbegin - uartctl->rxend;
    }
    else
    {
        spare = uartctl->rxbufsize;
    }
    if (len > spare - 1)
        return -1;

    if (uartctl->rxend >= uartctl->rxbegin)
    {
        sparetoend = uartctl->rxbufsize - uartctl->rxend;
        if (sparetoend > len)
        {
            memcpy(uartctl->rxbuf + uartctl->rxend, buf, len);
            uartctl->rxend = (uartctl->rxend + len) % uartctl->rxbufsize;
        }
        else
        {
            memcpy(uartctl->rxbuf + uartctl->rxend, buf, sparetoend);
            uartctl->rxend = (uartctl->rxend + sparetoend) % uartctl->rxbufsize;
            memcpy(uartctl->rxbuf + uartctl->rxend, buf + sparetoend,
                   len - sparetoend);
            uartctl->rxend += (len - sparetoend);
        }
    }
    else
    {
        memcpy(uartctl->rxbuf + uartctl->rxend, buf, len);
        uartctl->rxend += len;
    }
    return 0;
}

/**************************************************
@bref       查找缓冲区中是否有数据
@param
    uartctl     串口控制体
@return
@note
**************************************************/
static void postUartRxData(UART_RXTX_CTL *uartctl)
{
    uint16_t end, rxsize;
    if (uartctl->rxbegin == uartctl->rxend)
    {
        return;
    }
    end = uartctl->rxend;
    if (end > uartctl->rxbegin)
    {
        rxsize = end - uartctl->rxbegin;
        if (uartctl->rxhandlefun != NULL)
        {
            uartctl->rxhandlefun(uartctl->rxbuf + uartctl->rxbegin, rxsize);
        }
        uartctl->rxbegin = end;
    }
    else
    {
        rxsize = uartctl->rxbufsize - uartctl->rxbegin;
        if (uartctl->rxhandlefun != NULL)
        {
            uartctl->rxhandlefun(uartctl->rxbuf + uartctl->rxbegin, rxsize);
        }
        uartctl->rxbegin = 0;
        if (uartctl->rxhandlefun != NULL)
        {
            uartctl->rxhandlefun(uartctl->rxbuf + uartctl->rxbegin, end);
        }
        uartctl->rxbegin = end;
    }
}
/**************************************************
@bref       轮询所以串口控制体
@param
@return
@note
**************************************************/
void pollUartData(void)
{
    postUartRxData(&usart0_ctl);
    postUartRxData(&usart1_ctl);
    postUartRxData(&usart2_ctl);
    postUartRxData(&usart3_ctl);
}
/**
 * @brief               串口初始化
 * @param
 *      type            串口号，见UARTTYPE
 *      onoff           开关
 *      baudrate        波特率
 *      rxhandlefun     接收回调
 * @note
 *      UART0_TX:PB7     UART0_RX:PB4
 *      UART1_TX:PB13    UART1_RX:PB12
 *      UART2_TX:PB23    UART2_RX:PB22
 *      UART3_TX:PA5     UART3_RX:PA4
 * @return  none
 */
void portUartCfg(UARTTYPE type, uint8_t onoff, uint32_t baudrate,
                 void (*rxhandlefun)(uint8_t *, uint16_t len))
{
    switch (type)
    {
        case APPUSART0:
            memset(&usart0_ctl, 0, sizeof(UART_RXTX_CTL));
            usart0_ctl.rxbuf = uart0RxBuff;
            usart0_ctl.rxbufsize = APPUSART0_BUFF_SIZE;
            usart0_ctl.rxhandlefun = rxhandlefun;
            usart0_ctl.txhandlefun = UART0_SendString;

            if (onoff)
            {
                usart0_ctl.init = 1;
                GPIOB_SetBits(GPIO_Pin_7);
                GPIOB_ModeCfg(GPIO_Pin_4, GPIO_ModeIN_PU);
                GPIOB_ModeCfg(GPIO_Pin_7, GPIO_ModeOut_PP_5mA);
                UART0_DefInit();
                UART0_BaudRateCfg(baudrate);
                if (rxhandlefun != NULL)
                {
                    UART0_ByteTrigCfg(UART_7BYTE_TRIG);
                    uart0trigB = 7;
                    UART0_INTCfg(ENABLE, RB_IER_RECV_RDY | RB_IER_LINE_STAT);
                    PFIC_EnableIRQ(UART0_IRQn);
                }
                LogPrintf(DEBUG_ALL, "Open uart%d==>%d", type, baudrate);
            }
            else
            {
                usart0_ctl.init = 0;
                UART0_Reset();
                GPIOB_ModeCfg(GPIO_Pin_4, GPIO_ModeIN_Floating);
                GPIOB_ModeCfg(GPIO_Pin_7, GPIO_ModeIN_Floating);
                UART0_INTCfg(DISABLE, RB_IER_RECV_RDY | RB_IER_LINE_STAT);
                PFIC_DisableIRQ(UART0_IRQn);
            }
            break;
        case APPUSART1:

            usart1_ctl.rxbegin = 0;
            usart1_ctl.rxend = 0;
            usart1_ctl.rxbuf = uart1RxBuff;
            usart1_ctl.rxbufsize = APPUSART1_BUFF_SIZE;
            usart1_ctl.rxhandlefun = rxhandlefun;
            usart1_ctl.txhandlefun = UART1_SendString;

            if (onoff)
            {
                usart1_ctl.init = 1;
                GPIOB_SetBits(GPIO_Pin_13); //Tx default high
                GPIOB_ModeCfg(GPIO_Pin_12, GPIO_ModeIN_PU);  //Rx default input high
                GPIOB_ModeCfg(GPIO_Pin_13, GPIO_ModeOut_PP_5mA);  // Set tx as output

                GPIOPinRemap(ENABLE, RB_PIN_UART1);

                UART1_DefInit(); //
                UART1_BaudRateCfg(baudrate);
                if (rxhandlefun != NULL)
                {

                    UART1_ByteTrigCfg(UART_7BYTE_TRIG);
                    uart1trigB = 7;
                    UART1_INTCfg(ENABLE, RB_IER_RECV_RDY | RB_IER_LINE_STAT);
                    PFIC_EnableIRQ(UART1_IRQn);
                }
            }
            else
            {
                usart1_ctl.init = 0;
                UART1_Reset();
                GPIOA_ModeCfg(GPIO_Pin_8, GPIO_ModeIN_Floating);
                GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeIN_Floating);
                UART1_INTCfg(DISABLE, RB_IER_RECV_RDY | RB_IER_LINE_STAT);
                PFIC_DisableIRQ(UART1_IRQn);
            }
            break;
        case APPUSART2:

            usart2_ctl.rxbegin = 0;
            usart2_ctl.rxend = 0;
            usart2_ctl.rxbuf = uart2RxBuff;
            usart2_ctl.rxbufsize = APPUSART2_BUFF_SIZE;
            usart2_ctl.rxhandlefun = rxhandlefun;
            usart2_ctl.txhandlefun = UART2_SendString;

            if (onoff)
            {
                usart2_ctl.init = 1;
                GPIOPinRemap(ENABLE, RB_PIN_UART2);
                GPIOB_SetBits(GPIO_Pin_23); //Tx default high
                GPIOB_ModeCfg(GPIO_Pin_22, GPIO_ModeIN_PU);  //Rx default input high
                GPIOB_ModeCfg(GPIO_Pin_23, GPIO_ModeOut_PP_5mA);  // Set tx as output
                UART2_DefInit(); //
                UART2_BaudRateCfg(baudrate);
                if (rxhandlefun != NULL)
                {
                    UART2_ByteTrigCfg(UART_7BYTE_TRIG);
                    uart2trigB = 7;
                    UART2_INTCfg(ENABLE, RB_IER_RECV_RDY | RB_IER_LINE_STAT);
                    PFIC_EnableIRQ(UART2_IRQn);
                }
            }
            else
            {
                usart2_ctl.init = 0;
                UART2_Reset();
                GPIOB_ModeCfg(GPIO_Pin_22, GPIO_ModeIN_Floating);
                GPIOB_ModeCfg(GPIO_Pin_23, GPIO_ModeIN_Floating);
                UART2_INTCfg(DISABLE, RB_IER_RECV_RDY | RB_IER_LINE_STAT);
                PFIC_DisableIRQ(UART2_IRQn);
            }
            break;
        case APPUSART3:

            usart3_ctl.rxbegin = 0;
            usart3_ctl.rxend = 0;
            usart3_ctl.rxbuf = uart3RxBuff;
            usart3_ctl.rxbufsize = APPUSART3_BUFF_SIZE;
            usart3_ctl.rxhandlefun = rxhandlefun;
            usart3_ctl.txhandlefun = UART3_SendString;

            if (onoff)
            {
                usart3_ctl.init = 1;
                GPIOA_SetBits(GPIO_Pin_5); //Tx default high
                GPIOA_ModeCfg(GPIO_Pin_4, GPIO_ModeIN_PU);  //Rx default input high
                GPIOA_ModeCfg(GPIO_Pin_5, GPIO_ModeOut_PP_5mA);  // Set tx as output
                UART3_DefInit(); //
                UART3_BaudRateCfg(baudrate);
                if (rxhandlefun != NULL)
                {
                    UART3_ByteTrigCfg(UART_7BYTE_TRIG);
                    uart3trigB = 7;
                    UART3_INTCfg(ENABLE, RB_IER_RECV_RDY | RB_IER_LINE_STAT);
                    PFIC_EnableIRQ(UART3_IRQn);
                }
            }
            else
            {
                usart3_ctl.init = 0;
                UART3_Reset();
                GPIOA_ModeCfg(GPIO_Pin_4, GPIO_ModeIN_Floating);
                GPIOA_ModeCfg(GPIO_Pin_5, GPIO_ModeIN_Floating);
                UART3_INTCfg(DISABLE, RB_IER_RECV_RDY | RB_IER_LINE_STAT);
                PFIC_DisableIRQ(UART3_IRQn);
            }
            break;
        default:
            break;
    }
    //    if (onoff)
    //    {
    //        LogPrintf(DEBUG_ALL, "Open uart%d==>%d", type, baudrate);
    //    }
    //    else
    //    {
    //        LogPrintf(DEBUG_ALL, "Close uart%d", type);
    //    }
}
/**
 * @brief               串口发送数据
 * @param
 *      uartctl         串口操作符
 *      buf             待发送数据
 *      len             数据长度
 * @return  none
 */
void portUartSend(UART_RXTX_CTL *uartctl, uint8_t *buf, uint16_t len)
{
    if (uartctl->txhandlefun != NULL && uartctl->init == 1)
    {
        uartctl->txhandlefun(buf, len);
    }
}
/*
 * UART0 中断接收
 * */
__attribute__((interrupt("WCH-Interrupt-fast")))
__attribute__((section(".highcode")))
void UART0_IRQHandler(void)
{
    UINT8V i;
    UINT8V linestate;
    uint8_t rxbuff[7];
    switch (UART0_GetITFlag())
    {
        case UART_II_LINE_STAT:        // 线路状态错误
        {
            linestate = UART0_GetLinSTA();
            (void) linestate;
            break;
        }

        case UART_II_RECV_RDY:          // 数据达到设置触发点
            for (i = 0; i != uart0trigB; i++)
            {
                rxbuff[i] = UART0_RecvByte();
            }
            pushUartRxData(&usart0_ctl, rxbuff, uart0trigB);
            break;

        case UART_II_RECV_TOUT:         // 接收超时，暂时一帧数据接收完成
            i = UART0_RecvString(rxbuff);
            pushUartRxData(&usart0_ctl, rxbuff, i);
            break;

        case UART_II_THR_EMPTY:         // 发送缓存区空，可继续发送
            break;

        case UART_II_MODEM_CHG:         // 只支持串口0
            break;

        default:
            break;
    }
}
/*
 * UART1 中断接收
 * */
__attribute__((interrupt("WCH-Interrupt-fast")))
__attribute__((section(".highcode")))
void UART1_IRQHandler(void)
{
    UINT8V linestate;
    UINT8V i;
    uint8_t rxbuff[7];
    switch (UART1_GetITFlag())
    {
        case UART_II_LINE_STAT:        // 线路状态错误
        {
            linestate = UART1_GetLinSTA();
            (void) linestate;
            break;
        }

        case UART_II_RECV_RDY:          // 数据达到设置触发点
            for (i = 0; i != uart1trigB; i++)
            {
                rxbuff[i] = UART1_RecvByte();
            }
            pushUartRxData(&usart1_ctl, rxbuff, uart1trigB);
            break;

        case UART_II_RECV_TOUT:         // 接收超时，暂时一帧数据接收完成
            i = UART1_RecvString(rxbuff);
            pushUartRxData(&usart1_ctl, rxbuff, i);
            break;

        case UART_II_THR_EMPTY:         // 发送缓存区空，可继续发送
            break;

        case UART_II_MODEM_CHG:         // 只支持串口0
            break;

        default:
            break;
    }
}
/*
 * UART2 中断接收
 * */
__attribute__((interrupt("WCH-Interrupt-fast")))
__attribute__((section(".highcode")))
void UART2_IRQHandler(void)
{
    UINT8V i;
    UINT8V linestate;
    uint8_t rxbuff[7];
    switch (UART2_GetITFlag())
    {
        case UART_II_LINE_STAT:        // 线路状态错误
        {
            linestate = UART2_GetLinSTA();
            (void) linestate;
            break;
        }

        case UART_II_RECV_RDY:          // 数据达到设置触发点
            for (i = 0; i != uart2trigB; i++)
            {
                rxbuff[i] = UART2_RecvByte();
            }
            pushUartRxData(&usart2_ctl, rxbuff, uart2trigB);
            break;

        case UART_II_RECV_TOUT:         // 接收超时，暂时一帧数据接收完成
            i = UART2_RecvString(rxbuff);
            pushUartRxData(&usart2_ctl, rxbuff, i);
            break;

        case UART_II_THR_EMPTY:         // 发送缓存区空，可继续发送
            break;

        case UART_II_MODEM_CHG:         // 只支持串口0
            break;

        default:
            break;
    }
}
/*
 * UART3 中断接收
 * */
__attribute__((interrupt("WCH-Interrupt-fast")))
__attribute__((section(".highcode")))
void UART3_IRQHandler(void)
{
    UINT8V i;
    UINT8V linestate;
    uint8_t rxbuff[7];
    switch (UART3_GetITFlag())
    {
        case UART_II_LINE_STAT:
        {
            linestate = UART3_GetLinSTA();
            (void) linestate;
            break;
        }

        case UART_II_RECV_RDY:
            for (i = 0; i != uart3trigB; i++)
            {
                rxbuff[i] = UART3_RecvByte();
            }
            pushUartRxData(&usart3_ctl, rxbuff, uart3trigB);
            break;

        case UART_II_RECV_TOUT:
            i = UART3_RecvString(rxbuff);
            pushUartRxData(&usart3_ctl, rxbuff, i);
            break;

        case UART_II_THR_EMPTY:
            break;

        case UART_II_MODEM_CHG:
            break;

        default:
            break;
    }
}

/**************************************************
@bref       定时器初始化
@param
@return
@note
**************************************************/
void portTimerCfg(void)
{
    TMR0_TimerInit(GetSysClock() / 1000);
    R8_TMR0_INTER_EN |= 0x01;
    PFIC_EnableIRQ(TMR0_IRQn);
}

__attribute__((interrupt("WCH-Interrupt-fast")))
__attribute__((section(".highcode")))
void TMR0_IRQHandler(void)
{
    systemTickInc();
    if (R8_TMR0_INT_FLAG & 0x01)
    {
        R8_TMR0_INT_FLAG |= 0x01;
    }
}


/**************************************************
@bref       重置GPIO
@param
@return
@note
**************************************************/

void portGpioSetDefCfg(void)
{
    GPIOA_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_Floating);
    GPIOB_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_Floating);
}

/**************************************************
@bref       模组控制引脚初始化
@param
@return
@note
**************************************************/
void portModuleGpioCfg(void)
{
    GPIOA_ModeCfg(POWER_PIN, GPIO_ModeOut_PP_5mA);
    GPIOA_ModeCfg(RST_PIN, GPIO_ModeOut_PP_5mA);
    GPIOA_ModeCfg(DTR_PIN, GPIO_ModeOut_PP_5mA);

    PORT_POWER_SUPPLY_OFF;
    PORT_PWRKEY_H;
    PORT_RSTKEY_L;
}

/**************************************************
@bref       模组控制引脚初始化
@param
@return
@note
**************************************************/
void portLedCfg(void)
{
    GPIOB_ModeCfg(LED1_PIN, GPIO_ModeOut_PP_5mA);
    LED1_OFF;
}
/**
 * @brief   系统复位
 * @param
 * @return
 */
void portSysReset(void)
{
    LogMessage(DEBUG_ALL, "system reset");
    SYS_ResetExecute();
}

