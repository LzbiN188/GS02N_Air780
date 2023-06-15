/*
 * app_ble.h
 *
 *  Created on: Jan 28, 2023
 *      Author: nimo
 */

#ifndef TASK_INC_APP_BLECMD_H_
#define TASK_INC_APP_BLECMD_H_

#include "config.h"
#include "app_sys.h"
#include "app_param.h"
#include "app_central.h"
#include "app_server.h"
#include "app_protocol.h"
#include "app_instructioncmd.h"

typedef enum
{
    BLE_PROCESS_IDLE,
    BLE_PROCESS_CONNECT,
    BLE_PROCESS_SEND,
}bleProcessFsm_e;


typedef struct
{
    bleProcessFsm_e fsm;
    uint8_t connTick;
    uint8_t connMac[13];
    uint8_t onoff;//¿ªËøÓë¹ØËø
    uint8_t sendTick;
    uint8_t sendTimeout;
}bleProcess_s;



void doWriteCmdInstrtution(uint8_t *msg, uint16_t len);
void doListCmdInstrution(uint8_t *msg, uint16_t len);

void bleUpload(void);

void bleProcessTask(void);

void bleChangeToConnect(uint8_t *mac, uint8_t onoff);

uint8_t isBleProcessIdle(void);

uint8_t bleProtocolParser(uint8_t *src, uint8_t len);




#endif /* TASK_INC_APP_BLECMD_H_ */
