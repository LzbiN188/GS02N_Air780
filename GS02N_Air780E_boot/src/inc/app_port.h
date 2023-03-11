/*
 * app_port.h
 *
 *  Created on: Aug 2, 2022
 *      Author: idea
 */

#ifndef APP_PORT
#define APP_PORT

#include  "CH58x_common.h"

#define APPUSART0_BUFF_SIZE 768
#define APPUSART1_BUFF_SIZE 10
#define APPUSART2_BUFF_SIZE 128
#define APPUSART3_BUFF_SIZE 10


//4GÄ£×éÉæ¼°IO
#define POWER_SUPPLY_PIN
#define PORT_POWER_SUPPLY_ON
#define PORT_POWER_SUPPLY_OFF

#define POWER_PIN                   GPIO_Pin_14
#define PORT_PWRKEY_H               GPIOA_ResetBits(POWER_PIN)
#define PORT_PWRKEY_L               GPIOA_SetBits(POWER_PIN)

#define RST_PIN                     GPIO_Pin_13
#define PORT_RSTKEY_H               GPIOA_ResetBits(RST_PIN)
#define PORT_RSTKEY_L               GPIOA_SetBits(RST_PIN)


#define DTR_PIN                     GPIO_Pin_15
#define DTR_HIGH                    GPIOA_SetBits(DTR_PIN)
#define DTR_LOW                     GPIOA_ResetBits(DTR_PIN)

#define LED1_PIN        			GPIO_Pin_10
#define LED1_ON         			GPIOB_SetBits(LED1_PIN)
#define LED1_OFF       				GPIOB_ResetBits(LED1_PIN)



typedef enum
{
    APPUSART0,
    APPUSART1,
    APPUSART2,
    APPUSART3,
} UARTTYPE;

typedef struct
{
    uint8_t *rxbuf;
    uint8_t init;
    uint16_t rxbufsize;
    __IO uint16_t rxbegin;
    __IO uint16_t rxend;

    void (*rxhandlefun)(uint8_t *, uint16_t len);
    void (*txhandlefun)(uint8_t *, uint16_t len);

} UART_RXTX_CTL;

extern UART_RXTX_CTL usart0_ctl;
extern UART_RXTX_CTL usart1_ctl;
extern UART_RXTX_CTL usart2_ctl;
extern UART_RXTX_CTL usart3_ctl;

void portUartCfg(UARTTYPE type, uint8_t onoff, uint32_t baudrate,void (*rxhandlefun)(uint8_t *, uint16_t len));
void portUartSend(UART_RXTX_CTL *uartctl, uint8_t *buf, uint16_t len);
void pollUartData(void);

void portTimerCfg(void);

void portGpioSetDefCfg(void);
void portModuleGpioCfg(void);
void portLedCfg(void);
void portSysReset(void);

#endif /* SRC_INC_APP_PORT_H_ */
