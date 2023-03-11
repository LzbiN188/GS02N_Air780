#ifndef APP_NET
#define APP_NET
#include "CH58x_common.h"
#include "app_port.h"

#define POWER_ON            PORT_POWER_SUPPLY_ON
#define POWER_OFF           PORT_POWER_SUPPLY_OFF

#define PWRKEY_HIGH         PORT_PWRKEY_H
#define PWRKEY_LOW          PORT_PWRKEY_L

#define RSTKEY_HIGH         PORT_RSTKEY_H
#define RSTKEY_LOW          PORT_RSTKEY_L
//DTR
#define WAKEMODULE          DTR_LOW
#define SLEEPMODULE         DTR_HIGH

#define MODULE_RX_BUFF_SIZE		2048

typedef enum
{
    AT_CMD = 1,
    CPIN_CMD,
    CSQ_CMD,
    CGREG_CMD,
    CEREG_CMD,
    QICSGP_CMD,
    QIACT_CMD,
    QIOPEN_CMD,
    QISEND_CMD,
    QSCLK_CMD,
    CFUN_CMD,
    QICLOSE_CMD,
    CGDCONT_CMD,
} atCmdType_e;


/*定义系统运行状态*/
typedef enum
{
    AT_STATUS,
    CPIN_STATUS,
    CSQ_STATUS,
    CGREG_STATUS,
    CONFIG_STATUS,
    QIACT_STATUS,
    NORMAL_STATUS,
} moduleStatus_s;

/*指令集对应结构体*/
typedef struct
{
    atCmdType_e cmd_type;
    char *cmd;
} atCmd_s;

//发送队列结构体
typedef struct cmdNode
{
    char *data;
    uint16_t datalen;
    uint8_t currentcmd;
    struct cmdNode *nextnode;
} cmdNode_s;



typedef struct
{
    uint8_t powerState			: 1;
    uint8_t atResponOK			: 1;
    uint8_t cpinResponOk		: 1;
    uint8_t csqOk				: 1;
    uint8_t cgregOK				: 1;
    uint8_t ceregOK				: 1;
    uint8_t qipactSet			: 1;
    uint8_t qipactOk			: 1;

    uint8_t fsmState;
    uint8_t cmd;
    uint8_t rssi;
    uint16_t lac;
    uint32_t cid;
    uint32_t powerOnTick;
    uint32_t fsmtick;

} moduleState_s;

typedef struct
{
    uint8_t atCount;
    uint8_t csqCount;
    uint8_t cgregCount;
    uint8_t qipactCount;
    uint16_t csqTime;
} moduleCtrl_s;





void outputNode(void);
uint8_t  sendModuleCmd(uint8_t cmd, char *param);


void modulePowerOn(void);
void modulePowerOff(void);
void moduleReset(void);

void openSocket(uint8_t link, char *server, uint16_t port);
void closeSocket(uint8_t link);

void netConnectTask(void);
void moduleRecvParser(uint8_t *buf, uint16_t bufsize);

void netResetCsqSearch(void);
int socketSendData(uint8_t link, uint8_t *data, uint16_t len);

uint8_t isModuleRunNormal(void);


#endif

