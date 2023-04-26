#ifndef APP_INSTRUCTION_H
#define APP_INSTRUCTION_H

#include <stdint.h>


typedef enum{
	DEBUG_MODE,
	SMS_MODE,
	NET_MODE,
	BLE_MODE,
}insMode_e;

typedef enum{
	PARAM_INS,
	STATUS_INS,
	VERSION_INS,
	SN_INS,
	SERVER_INS,
	MODE_INS,
	HBT_INS,
	POSITION_INS,
	APN_INS,
	UPS_INS,
	LOWW_INS,
	LED_INS,
	POITYPE_INS,
	RESET_INS,
	UTC_INS,
	DEBUG_INS,
	ACCCTLGNSS_INS,
	ACCDETMODE_INS,
	FENCE_INS,
	FACTORY_INS,
	ICCID_INS,
	SETAGPS_INS,
	JT808SN_INS,
	HIDESERVER_INS,
	BLESERVER_INS,
    RELAY_INS,
    READPARAM_INS,
    SETBLEPARAM_INS,
    SETBLEWARNPARAM_INS,
    SETBLEMAC_INS,
    RELAYSPEED_INS,
    RELAYFORCE_INS,
    BF_INS,
    CF_INS,
    PROTECTVOL_INS,
    TIMER_INS,
    SOS_INS,
    CENTER_INS,
    SOSALM_INS,
    QGMR_INS,
    MOTIONDET_INS,
    FCG_INS,
    QFOTA_INS,
    BLEEN_INS,
    AGPSEN_INS,

}INSTRUCTIONID;



typedef struct
{
	uint16_t cmdid;
	char *   cmdstr;
}instruction_s;

typedef struct{
	char * telNum;
	char * data;
	uint16_t len;
	uint8_t link;
}insParam_s;

void instructionParser(uint8_t *str, uint16_t len, insMode_e mode, void * param);
void dorequestSend123(void);

#endif
