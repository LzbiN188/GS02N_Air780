#include "app_port.h"

#include "CH583SFR.h"
#include "SLEEP.h"

#include "app_kernal.h"
#include "app_mir3da.h"
#include "app_param.h"
#include "app_sys.h"
#include "app_task.h"
#include "app_db.h"
//IIC
#define SCL_OUTHIGH    GPIOB_ModeCfg(SCL_PIN,GPIO_ModeIN_Floating)//����ߵ�ƽʱ�����������룬���ⲿ����Ϊ�ߵ�ƽ
#define SCL_OUTLOW     {GPIOB_ModeCfg(SCL_PIN,GPIO_ModeOut_PP_5mA);GPIOB_ResetBits(SCL_PIN);}//��Ϊ���ģʽ��������͵�ƽ
#define SCL_READ       sclReadInput()//��Ϊ����ģʽ��������ƽ

#define SDA_OUTHIGH    GPIOB_ModeCfg(SDA_PIN,GPIO_ModeIN_Floating)
#define SDA_OUTLOW     {GPIOB_ModeCfg(SDA_PIN,GPIO_ModeOut_PP_5mA);GPIOB_ResetBits(SDA_PIN);}
#define SDA_READ       sdaReadInput()
/*--------------------------------------------------------------*/
/*
 * UART
 */
uint8_t uart0RxBuff[APPUSART0_BUFF_SIZE];
uint8_t uart0trigB;

uint8_t uart1RxBuff[APPUSART1_BUFF_SIZE];
uint8_t uart1trigB;

uint8_t uart2RxBuff[APPUSART2_BUFF_SIZE];
uint8_t uart2trigB;

uint8_t uart3RxBuff[APPUSART3_BUFF_SIZE];
uint8_t uart3trigB;

UART_RXTX_CTL usart0_ctl = {0};
UART_RXTX_CTL usart1_ctl = {0};
UART_RXTX_CTL usart2_ctl = {0};
UART_RXTX_CTL usart3_ctl = {0};



/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/
/**
 * @brief               �����յĴ������������Ӧ�Ĵ��ڻ�����
 * @param
 *      uartctl         ���ڲ�����
 *      buf             �ѽ�������
 *      len             ���ݳ���
 * @return
 *      -1              ��������
 *      0               �ɹ�
 */
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
    {
        LogMessage(DEBUG_ALL, "pushUartRxData==>no space");
        return -1;
    }

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

/**
 * @brief               ���Ҵ��ڽ��ն������Ƿ�������
 * @param
 *      uartctl         ���ڲ�����
 * @return  none
 */
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
/**
 * @brief               ��ѯ���ڲ�������������������
 * @param
 * @return  none
 */
void pollUartData(void)
{
    postUartRxData(&usart0_ctl);
    postUartRxData(&usart1_ctl);
    postUartRxData(&usart2_ctl);
    postUartRxData(&usart3_ctl);
}
/**
 * @brief               ���ڳ�ʼ��
 * @param
 *      type            ���ںţ���UARTTYPE
 *      onoff           ����
 *      baudrate        ������
 *      rxhandlefun     ���ջص�
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
    if (onoff)
    {
        LogPrintf(DEBUG_ALL, "Open uart%d==>%d", type, baudrate);
    }
    else
    {
        LogPrintf(DEBUG_ALL, "Close uart%d", type);
    }
}
/**
 * @brief               ���ڷ�������
 * @param
 *      uartctl         ���ڲ�����
 *      buf             ����������
 *      len             ���ݳ���
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
 * UART0 �жϽ���
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
        case UART_II_LINE_STAT:        // ��·״̬����
        {
            linestate = UART0_GetLinSTA();
            (void) linestate;
            break;
        }

        case UART_II_RECV_RDY:          // ���ݴﵽ���ô�����
            for (i = 0; i != uart0trigB; i++)
            {
                rxbuff[i] = UART0_RecvByte();
            }
            pushUartRxData(&usart0_ctl, rxbuff, uart0trigB);
            break;

        case UART_II_RECV_TOUT:         // ���ճ�ʱ����ʱһ֡���ݽ������
            i = UART0_RecvString(rxbuff);
            pushUartRxData(&usart0_ctl, rxbuff, i);
            break;

        case UART_II_THR_EMPTY:         // ���ͻ������գ��ɼ�������
            break;

        case UART_II_MODEM_CHG:         // ֻ֧�ִ���0
            break;

        default:
            break;
    }
}
/*
 * UART1 �жϽ���
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
        case UART_II_LINE_STAT:        // ��·״̬����
        {
            linestate = UART1_GetLinSTA();
            (void) linestate;
            break;
        }

        case UART_II_RECV_RDY:          // ���ݴﵽ���ô�����
            for (i = 0; i != uart1trigB; i++)
            {
                rxbuff[i] = UART1_RecvByte();
            }
            pushUartRxData(&usart1_ctl, rxbuff, uart1trigB);
            break;

        case UART_II_RECV_TOUT:         // ���ճ�ʱ����ʱһ֡���ݽ������
            i = UART1_RecvString(rxbuff);
            pushUartRxData(&usart1_ctl, rxbuff, i);
            break;

        case UART_II_THR_EMPTY:         // ���ͻ������գ��ɼ�������
            break;

        case UART_II_MODEM_CHG:         // ֻ֧�ִ���0
            break;

        default:
            break;
    }
}
/*
 * UART2 �жϽ���
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
        case UART_II_LINE_STAT:        // ��·״̬����
        {
            linestate = UART2_GetLinSTA();
            (void) linestate;
            break;
        }

        case UART_II_RECV_RDY:          // ���ݴﵽ���ô�����
            for (i = 0; i != uart2trigB; i++)
            {
                rxbuff[i] = UART2_RecvByte();
            }
            pushUartRxData(&usart2_ctl, rxbuff, uart2trigB);
            break;

        case UART_II_RECV_TOUT:         // ���ճ�ʱ����ʱһ֡���ݽ������
            i = UART2_RecvString(rxbuff);
            pushUartRxData(&usart2_ctl, rxbuff, i);
            break;

        case UART_II_THR_EMPTY:         // ���ͻ������գ��ɼ�������
            break;

        case UART_II_MODEM_CHG:         // ֻ֧�ִ���0
            break;

        default:
            break;
    }
}
/*
 * UART3 �жϽ���
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


__attribute__((interrupt("WCH-Interrupt-fast")))
__attribute__((section(".highcode")))
void GPIOA_IRQHandler(void)
{
    uint16_t iqr;
    iqr = GPIOA_ReadITFlagPort();
    if (iqr & RING_PIN)
    {
        wakeUpByInt(0, 15);
        GPIOA_ClearITFlagBit(RING_PIN);
    }
}

__attribute__((interrupt("WCH-Interrupt-fast")))
__attribute__((section(".highcode")))
void GPIOB_IRQHandler(void)
{
    uint16_t iqr;
    iqr = GPIOB_ReadITFlagPort();
    if (iqr & GSINT_PIN)
    {
        motionOccur();
        
        GPIOB_ClearITFlagBit(GSINT_PIN);
    }
}

/**
 * @brief   ģ��GPIO��ʼ��
 * @param
 * @return
 */
void portModuleGpioCfg(void)
{
    GPIOA_ModeCfg(POWER_PIN, GPIO_ModeOut_PP_5mA);
    GPIOA_ModeCfg(RST_PIN, GPIO_ModeOut_PP_5mA);
    GPIOA_ModeCfg(DTR_PIN, GPIO_ModeOut_PP_5mA);
    GPIOA_ModeCfg(RING_PIN, GPIO_ModeIN_Floating);
    GPIOA_ITModeCfg(RING_PIN, GPIO_ITMode_FallEdge);
    PFIC_EnableIRQ(GPIO_A_IRQn);
    PORT_PWRKEY_H;
    PORT_RSTKEY_L;
    PORT_SUPPLY_OFF;
}

/**
 * @brief   LED GPIO��ʼ��
 * @param
 * @return
 */
void portLedGpioCfg(void)
{
    GPIOB_ModeCfg(LED1_PIN, GPIO_ModeOut_PP_5mA);
    LED1_OFF;
    LED2_OFF;
}

/**
 * @brief   GPS GPIO��ʼ��
 * @param
 * @return
 */
void portGpsGpioCfg(void)
{
    GPIOB_ModeCfg(GPSLNA_PIN, GPIO_ModeOut_PP_5mA);
    GPSLNA_OFF;
}

/**
 * @brief   GPIO ����
 * @param
 * @return
 */
void portGpioSetDefCfg(void)
{
    GPIOA_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_Floating);
    GPIOB_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_Floating);
}

/**
 * @brief   ϵͳ��λ
 * @param
 * @return
 */
void portSysReset(void)
{
    LogMessage(DEBUG_ALL, "system reset");
    dbSaveRelease();
    SYS_ResetExecute();
}
/**
 * @brief   gsensor ��Դ����
 * @param
 *      onoff
 * @return
 */
void portGsensorPwrCtl(uint8 onoff)
{
    if (onoff)
    {
        GPIOA_ModeCfg(GSPWR_PIN, GPIO_ModeOut_PP_5mA);
        GSPWR_OFF;
    }
    else
    {
        GPIOA_ModeCfg(GSPWR_PIN, GPIO_ModeIN_Floating);
        GSPWR_ON;
    }
}
/**
 * @brief   IIC ���ų�ʼ��
 * @param
 *      onoff
 * @return
 */
void portIICCfg(void)
{
    SCL_OUTHIGH;
    SDA_OUTHIGH;
}

/**
 * @brief   IIC SCL
 * @param
 * @return
 */
uint8_t sclReadInput(void)
{
    GPIOB_ModeCfg(SCL_PIN, GPIO_ModeIN_Floating);
    if (GPIOB_ReadPortPin(SCL_PIN))
    {
        return 1;
    }
    return 0;
}
/**
 * @brief   IIC SDA
 * @param
 * @return
 */
uint8_t sdaReadInput(void)
{
    GPIOB_ModeCfg(SDA_PIN, GPIO_ModeIN_Floating);
    if (GPIOB_ReadPortPin(SDA_PIN))
    {
        return 1;
    }
    return 0;
}
/**
 * @brief   IIC ������ʼ�ź�
 * @param
 * @return
 */
static void iicStart(void)
{
    SDA_OUTHIGH;
    SCL_OUTHIGH;
    DelayUs(5);
    SDA_OUTLOW
    ;
    DelayUs(5);
}
/**
 * @brief   IIC ����ֹͣ�ź�
 * @param
 * @return
 */
static void iicStop(void)
{
    SCL_OUTLOW;
    SDA_OUTLOW;
    DelayUs(5);
    SCL_OUTHIGH;
    ;
    DelayUs(5);
    SDA_OUTHIGH;
    DelayUs(5);
}
/**
 * @brief   IIC ����һ���ֽ�
 * @param
 *      data
 * @return
 */
static uint8_t iicSendOneByte(uint8_t data)
{
    int8_t i, ret = 0;
    for (i = 7; i >= 0; i--)
    {
        SCL_OUTLOW;
        DelayUs(5);
        if (data & (1 << i))
        {
            SDA_OUTHIGH;
        }
        else
        {
            SDA_OUTLOW;
        }
        DelayUs(5);
        SCL_OUTHIGH;
        DelayUs(5);
    }
    SCL_OUTLOW;
    DelayUs(5);
    SDA_OUTHIGH;
    DelayUs(5);
    SCL_OUTHIGH;
    if (SDA_READ == 0)
    {
        ret = 1;
    }
    SCL_OUTLOW;
    DelayUs(5);
    return ret;
}
/**
 * @brief   IIC ��ȡһ���ֽ�
 * @param
 * @return
 */
static uint8_t iicReadOneByte(uint8_t ack)
{
    uint8_t readByte = 0;
    int8_t i;
    for (i = 7; i >= 0; i--)
    {
        SCL_OUTLOW;
        DelayUs(5);
        SCL_OUTHIGH;
        DelayUs(5);
        if (SDA_READ)
        {
            readByte |= (1 << i);
        }
        DelayUs(5);
    }
    SCL_OUTLOW;
    DelayUs(5);
    if (ack)
    {
        SDA_OUTLOW;
    }
    else
    {
        SDA_OUTHIGH;
    }
    DelayUs(5);
    SCL_OUTHIGH;
    DelayUs(5);
    SCL_OUTLOW;
    SDA_OUTHIGH;
    DelayUs(5);
    return readByte;
}
/**
 * @brief   IIC ��ȡ����ֽ�
 * @param
 * @return
 */
uint8_t iicReadData(uint8_t addr, uint8_t regaddr, uint8_t *data, uint8_t len)
{
    uint8_t i, ret = 0;
    if (data == NULL)
        return ret;
    iicStart();
    addr &= ~0x01;
    ret = iicSendOneByte(addr);
    iicSendOneByte(regaddr);
    iicStart();
    addr |= 0x01;
    iicSendOneByte(addr);
    for (i = 0; i < len; i++)
    {
        if (i == (len - 1))
        {
            data[i] = iicReadOneByte(0);
        }
        else
        {
            data[i] = iicReadOneByte(1);
        }
    }
    iicStop();
    return ret;
}
/**
 * @brief   IIC д����ֽ�
 * @param
 * @return
 */
uint8_t iicWriteData(uint8_t addr, uint8_t reg, uint8_t data)
{
    uint8_t ret = 0;
    iicStart();
    addr &= ~0x01;
    ret = iicSendOneByte(addr);
    iicSendOneByte(reg);
    iicSendOneByte(data);
    iicStop();
    return ret;
}
/**
 * @brief   gsensor����
 * @param
 *      onoff
 * @return
 */
void portGsensorCtl(uint8_t onoff)
{

    portIICCfg();
    if (onoff)
    {
        sysinfo.gsensorOnoff = 1;
        GSPWR_ON;
        mir3da_init();
        mir3da_open_interrupt(10);
        mir3da_set_enable(1);

        GPIOB_ModeCfg(GSINT_PIN, GPIO_ModeIN_PU);
        GPIOB_ITModeCfg(GSINT_PIN, GPIO_ITMode_FallEdge);
        PFIC_EnableIRQ(GPIO_B_IRQn);
    }
    else
    {
        sysinfo.gsensorOnoff = 0;
        GSPWR_OFF;
        GPIOB_ModeCfg(GSINT_PIN, GPIO_ModeIN_PD);
        R16_PB_INT_EN &= ~GSINT_PIN;
    }
    LogPrintf(DEBUG_ALL, "gsensor %s", onoff ? "On" : "Off");

}

/**
 * @brief   RTC ����
 * @param
 * @return
 */
void portRtcCfg(void)
{
    LClk32K_Select(Clk32K_LSI);
    R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG1;
    R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG2;
    R8_SLP_WAKE_CTRL |= RB_SLP_RTC_WAKE;// RTC����
    R8_RTC_MODE_CTRL |= (RB_RTC_TMR_EN | RB_RTC_TMR_MODE); //��ʱģʽ

    R8_SAFE_ACCESS_SIG = 0;
    PFIC_EnableIRQ(RTC_IRQn);
}

/**
 * @brief   ��ȡRTCʱ��
 * @param
 * @return
 */
void portGetRtcDateTime(uint16_t *year, uint8_t *month, uint8_t *date, uint8_t *hour, uint8_t *minute, uint8_t *second)
{
    uint16_t a, b, c, d, e, f;
    RTC_GetTime(&a, &b, &c, &d, &e, &f);
    *year = a;
    *month = b;
    *date = c;
    *hour = d;
    *minute = e;
    *second = f;
}

/**
 * @brief   ����RTCʱ��
 * @param
 * @note    ʹ������Э��ջʱ���������rtcʱ��
 * @return
 */
void portUpdateRtcDateTime(uint8_t year, uint8_t month, uint8_t date, uint8_t hour, uint8_t minute, uint8_t second)
{
    uint16_t a, b, c, d, e, f;
    uint32_t sec1, sec2;
    RTC_GetTime(&a, &b, &c, &d, &e, &f);
    if (sysinfo.logLevel == DEBUG_FACTORY)
    {
        return ;
    }

    sec1 = d * 3600 + e * 60 + f;
    sec2 = hour * 3600 + minute * 60 + second;
    if ((year + 2000) != a || b != month || c != date || abs(sec1 - sec2) >= 60)
    {
        RTC_InitTime(year + 2000, month, date, hour, minute, second);
        LogPrintf(DEBUG_ALL, "update time==>%02d/%02d/%02d-%02d:%02d:%02d", year + 2000, month, date, hour, minute, second);
        portSysReset();
    }
    else
    {
        LogPrintf(DEBUG_ALL, "current Datetime==>%02d/%02d/%02d %02d:%02d:%02d", a, b, c, d, e, f);
    }
}

/**
 * @brief   ��������ʱ��
 * @param
 * @return
 */
static void portUpdateAlarmTime(uint8_t date, uint8_t hour, uint8_t minutes)
{
    sysinfo.alarmDate = date;
    sysinfo.alarmHour = hour;
    sysinfo.alarmMinute = minutes;
    LogPrintf(DEBUG_ALL, "SetTimeAlarm==>Date:%02d,Time:%02d:%02d", date, hour, minutes);
}

/**
  * @brief  ������һ��ʱ���
  * @param  None
  * @retval None
  */

int portSetNextAlarmTime(void)
{
    unsigned short  rtc_mins, next_ones;
    unsigned char next_date, set_nextdate = 1;
    uint16_t  YearToday;      /*��ǰ��*/
    uint8_t  MonthToday;     /*��ǰ��*/
    uint8_t  DateToday;      /*��ǰ��*/
    int i;
    //RTC_AlarmTypeDef rtc_a;
    uint16_t year;
    uint8_t  month;
    uint8_t date;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;

    portGetRtcDateTime(&year, &month, &date, &hour, &minute, &second);

    //1����ȡ��ǰʱ�����ܷ�����
    rtc_mins = (hour & 0x1F) * 60;
    rtc_mins += (minute & 0x3f);
    //2����ȡ��ǰ����
    YearToday = year; //���㵱ǰ�꣬��2000�꿪ʼ����
    MonthToday = month;
    DateToday = date;
    //3�����ݵ�ǰ�£������¸�������
    if (MonthToday == 4 || MonthToday == 6 || MonthToday == 9 || MonthToday == 11)
    {
        next_date = (DateToday + sysparam.gapDay) % 30; //��ǰ���ڼ��ϼ���գ�������һ�ε�ʱ���
        if (next_date == 0)
            next_date = 30;
    }
    else if (MonthToday == 2)
    {
        //4�������2�£��ж��ǲ�������
        if (((YearToday % 100 != 0) && (YearToday % 4 == 0)) || (YearToday % 400 == 0))  //����
        {
            next_date = (DateToday + sysparam.gapDay) % 29;
            if (next_date == 0)
                next_date = 29;
        }
        else
        {
            next_date = (DateToday + sysparam.gapDay) % 28;
            if (next_date == 0)
                next_date = 28;
        }
    }
    else
    {
        next_date = (DateToday + sysparam.gapDay) % 31;
        if (next_date == 0)
            next_date = 31;
    }
    next_ones = 0xFFFF;
    //5����������������Ƿ����ڵ�ǰʱ���֮���ʱ��
    for (i = 0; i < 5; i++)
    {
        if (sysparam.AlarmTime[i] == 0xFFFF)
            continue;
        if (sysparam.AlarmTime[i] > rtc_mins)   //����ǰʱ��ȶ�
        {
            next_ones = sysparam.AlarmTime[i];  //�õ��µ�ʱ��
            set_nextdate = 0;
            break;
        }
    }

    if (next_ones == 0xFFFF)  //û������ʱ��
    {
        //Set Current Alarm Time
        next_ones = sysparam.AlarmTime[0];
        if (next_ones == 0xFFFF)
        {
            next_ones = 720; //Ĭ��12:00
        }
    }
    //6�������´��ϱ������ں�ʱ��
    if (set_nextdate)
        portUpdateAlarmTime(next_date, (next_ones / 60) % 24, next_ones % 60);
    else
        portUpdateAlarmTime(date, (next_ones / 60) % 24, next_ones % 60);
    return 0;
}

/**
 * @brief   �����´λ���ʱ��
 * @param
 * @return
 */
void portSetNextWakeUpTime(void)
{
    uint16_t  YearToday;      /*��ǰ��*/
    uint8_t  MonthToday;     /*��ǰ��*/
    uint8_t  DateToday;      /*��ǰ��*/
    uint8_t  date = 0, next_date;
    uint16_t totalminutes;
    uint16_t year;
    uint8_t  month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;

    portGetRtcDateTime(&year, &month, &day, &hour, &minute, &second);
    YearToday = year; //���㵱ǰ�꣬��2000�꿪ʼ����
    MonthToday = month;
    DateToday = day;
    totalminutes = hour * 60 + minute;
    totalminutes += sysparam.gapMinutes;
    if (totalminutes >= 1440)
    {
        date = 1;
        totalminutes -= 1440;
    }
    //3�����ݵ�ǰ�£������¸�������
    if (MonthToday == 4 || MonthToday == 6 || MonthToday == 9 || MonthToday == 11)
    {
        next_date = (DateToday + date) % 30; //��ǰ���ڼ��ϼ���գ�������һ�ε�ʱ���
        if (next_date == 0)
            next_date = 30;
    }
    else if (MonthToday == 2)
    {
        //4�������2�£��ж��ǲ�������
        if (((YearToday % 100 != 0) && (YearToday % 4 == 0)) || (YearToday % 400 == 0))  //����
        {
            next_date = (DateToday + date) % 29;
            if (next_date == 0)
                next_date = 29;
        }
        else
        {
            next_date = (DateToday + date) % 28;
            if (next_date == 0)
                next_date = 28;
        }
    }
    else
    {
        next_date = (DateToday + date) % 31;
        if (next_date == 0)
            next_date = 31;
    }
    portUpdateAlarmTime(next_date, (totalminutes / 60) % 24, totalminutes % 60);
}

/**
 * @brief   �͹�������
 * @param
 * @return
 */
void portLowPowerCfg(void)
{
    R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG1;
    R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG2;
    R8_SLP_WAKE_CTRL |= RB_SLP_RTC_WAKE;    //RTC����
    R8_SLP_WAKE_CTRL |= RB_SLP_GPIO_WAKE;   //gpio�жϻ���
    R8_SLP_WAKE_CTRL &= ~RB_SLP_BAT_WAKE;   //�رյ�ص�ѹ����
    R8_SAFE_ACCESS_SIG = 0;
}

/**
 * @brief   adc�������
 * @param
 * @return
 */
void portAdcCfg(void)
{
    //PA9  channel 13
    GPIOA_ModeCfg(VCARD_ADCPIN, GPIO_ModeIN_Floating);
}
/**
 * @brief   ��ȡADC��ѹ
 * @param
 *      channel
 * @return
 *      float adc��ѹֵ
 */
float portGetAdcVol(ADC_SingleChannelTypeDef channel)
{
    float value;
    ADC_ChannelCfg(channel);
    ADC_ExtSingleChSampInit(SampleFreq_8, ADC_PGA_0);
    value = (ADC_ExcutSingleConver() / 2048.0) * 1.05;
    if (value >= 2.0)
    {
        ADC_ExtSingleChSampInit(SampleFreq_8, ADC_PGA_1_2);
        ADC_ExcutSingleConver();
        value = (ADC_ExcutSingleConver() / 1024.0 - 1) * 1.05;
    }
    LogPrintf(DEBUG_ALL, "value:%.2f", value);
    return value;
}
/**
 * @brief   ʹ��˯��
 * @param
 * @return
 */
void portSleepEn(void)
{
    sleepSetState(1);
}
/**
 * @brief   ��ֹ˯��
 * @param
 * @return
 */
void portSleepDn(void)
{
    sleepSetState(0);
}
/**
 * @brief   ��ȡ˯��״̬
 * @param
 * @return
 */
void portGetSleepState(void)
{
    LogPrintf(DEBUG_ALL, "sleep state==>%d", getSleepState());
}

/**
 * @brief   ʹ�ܿ��Ź�
 * @param
 * @return
 */
void portWdtCfg(void)
{
    WWDG_SetCounter(0);
    WWDG_ResetCfg(ENABLE);
}

/**
 * @brief   ι��
 * @param
 * @return
 */
void portWdtFeed(void)
{
    WWDG_SetCounter(0);
}

/**
 * @brief   �л�ϵͳ��Ƶ
 * @param
 	1		PLL����Ƶ
 	0		HSE����Ƶ
 * @return
 */

void portFclkChange(uint8_t type)
{
    if (type)
    {
    	SetSysClock(CLK_SOURCE_PLL_60MHz);
    }
    else
    {
        SetSysClock(CLK_SOURCE_HSE_16MHz);
    }
    //��Ƶ�ı䣬�޸Ĳ����ʼ���ֵ
    UART0_BaudRateCfg(115200);
    UART1_BaudRateCfg(115200);
    UART2_BaudRateCfg(115200);
    UART3_BaudRateCfg(115200);
}
