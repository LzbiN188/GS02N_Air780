#ifndef APP_PORT
#define APP_PORT

#include <stdint.h>
#include "config.h"
#define APPUSART0_BUFF_SIZE 512
#define APPUSART1_BUFF_SIZE 10
#define APPUSART2_BUFF_SIZE 128
#define APPUSART3_BUFF_SIZE 768

//4G模组涉及IO
#define SUPPLY_PIN      
#define PORT_SUPPLY_ON  
#define PORT_SUPPLY_OFF 

#define POWER_PIN    	GPIO_Pin_14
#define PORT_PWRKEY_H   GPIOA_ResetBits(POWER_PIN)
#define PORT_PWRKEY_L   GPIOA_SetBits(POWER_PIN)

#define RST_PIN			GPIO_Pin_13
#define PORT_RSTKEY_H	GPIOA_ResetBits(RST_PIN)
#define PORT_RSTKEY_L	GPIOA_SetBits(RST_PIN)


#define RING_PIN		GPIO_Pin_12

#define DTR_PIN			GPIO_Pin_15
#define DTR_HIGH		GPIOA_SetBits(DTR_PIN)
#define DTR_LOW			GPIOA_ResetBits(DTR_PIN)

//DA217涉及IO
#define GSPWR_PIN       GPIO_Pin_8
#define GSPWR_ON        GPIOA_ResetBits(GSPWR_PIN)
#define GSPWR_OFF       GPIOA_SetBits(GSPWR_PIN)
#define GSINT_PIN       GPIO_Pin_14
//IIC 涉及IO
#define SCL_PIN         GPIO_Pin_12
#define SDA_PIN         GPIO_Pin_13

//LED 涉及IO
#define LED1_PIN        GPIO_Pin_10
#define LED1_ON         GPIOB_SetBits(LED1_PIN)
#define LED1_OFF        GPIOB_ResetBits(LED1_PIN)
#define LED2_PIN
#define LED2_ON
#define LED2_OFF

//GPS 涉及IO
#define GPSPWR_PIN
#define GPSPWR_ON
#define GPSPWR_OFF

#define GPSLNA_PIN      GPIO_Pin_15
#define GPSLNA_ON       GPIOB_SetBits(GPSLNA_PIN)
#define GPSLNA_OFF      GPIOB_ResetBits(GPSLNA_PIN)

#define VCARD_ADCPIN	GPIO_Pin_9

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

typedef struct{
	uint16_t year;
	uint8_t month;
	uint8_t date;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
}system_date_s;

extern UART_RXTX_CTL usart0_ctl;
extern UART_RXTX_CTL usart1_ctl;
extern UART_RXTX_CTL usart2_ctl;
extern UART_RXTX_CTL usart3_ctl;

void pollUartData(void);
void portUartCfg(UARTTYPE type, uint8_t onoff, uint32_t baudrate, void (*rxhandlefun)(uint8_t *, uint16_t len));
void portUartSend(UART_RXTX_CTL *uartctl, uint8_t *buf, uint16_t len);

void portModuleGpioCfg(void);
void portLedGpioCfg(void);
void portGpsGpioCfg(void);
void portGpioSetDefCfg(void);

void portSysReset(void);

uint8_t iicReadData(uint8_t addr, uint8_t regaddr, uint8_t *data, uint8_t len);
uint8_t iicWriteData(uint8_t addr, uint8_t reg, uint8_t data);
void portGsensorPwrCtl(uint8 onoff);
void portIICCfg(void);
void portGsensorCtl(uint8_t onoff);

void portRtcCfg(void);
void portGetRtcDateTime(uint16_t *year, uint8_t *month, uint8_t *date, uint8_t *hour, uint8_t *minute, uint8_t *second);
void portUpdateRtcDateTime(uint8_t year, uint8_t month, uint8_t date, uint8_t hour, uint8_t minute, uint8_t second);
int portSetNextAlarmTime(void);
void portSetNextWakeUpTime(void);

void portLowPowerCfg(void);


void portAdcCfg(void);
float portGetAdcVol(ADC_SingleChannelTypeDef channel);

void portSleepEn(void);
void portSleepDn(void);
void portGetSleepState(void);

void portWdtCfg(void);
void portWdtFeed(void);
void portFclkChange(uint8_t type);

#endif
