#ifndef APP_JT808
#define APP_JT808

#include "CH58x_common.h"
#include "app_protocol.h"
#include "app_gps.h"

#define JT808_LOWVOLTAE_ALARM		(1<<7)//低电报警
#define JT808_LOSTVOLTAGE_ALARM		(1<<8)//断电报警


#define JT808_STATUS_ACC			(1<<0)//acc on
#define JT808_STATUS_FIX			(1<<1)//gps 定位
#define JT808_STATUS_LATITUDE		(1<<2)//南纬
#define JT808_STATUS_LONGTITUDE		(1<<3)//西经
#define JT808_STATUS_MOVE			(1<<4)//运营状态
#define JT808_STATUS_OIL			(1<<10)//油路断开
#define JT808_STATUS_ELECTRICITY	(1<<11)//电路断开
#define JT808_STATUS_USE_GPS		(1<<18)//gps定位
#define JT808_STATUS_USE_BEIDOU		(1<<19)//北斗定位
#define JT808_STATUS_USE_GLONASS	(1<<20)//格瑞那斯定位
#define JT808_STATUS_USE_GALILEO	(1<<21)//伽利略定位


#define TERMINAL_REGISTER_MSGID				0x0100  //终端注册
#define TERMINAL_AUTHENTICATION_MSGID		0x0102  //终端鉴权
#define TERMINAL_HEARTBEAT_MSGID			0x0002  //终端心跳
#define TERMINAL_POSITION_MSGID				0x0200  //位置信息
#define TERMINAL_ATTRIBUTE_MSGID			0x0107  //终端属性
#define TERMINAL_GENERIC_RESPON_MSGID		0x0001  //通用回答
#define TERMINAL_DATAUPLOAD_MSGID			0x0900  //数据上行
#define TERMINAL_BATCHUPLOAD_MSG			0x0704	//批量上传

#define PLATFORM_GENERIC_RESPON_MSGID		0x8001
#define PLATFORM_REGISTER_RESPON_MSGID		0x8100
#define PLATFORM_TERMINAL_CTL_MSGID			0x8105
#define PLATFORM_DOWNLINK_MSGID				0x8900
#define PLATFORM_TERMINAL_SET_PARAM_MSGID	0x8103
#define PLATFORM_QUERY_ATTR_MSGID			0x8107
#define PLATFORM_TEXT_MSGID					0x8300

#define TRANSMISSION_TYPE_F8	0xF8

#define RESTORE_MAX_SZIE		24




typedef enum
{
    TERMINAL_REGISTER,
    TERMINAL_AUTH,
    TERMINAL_HEARTBEAT,
    TERMINAL_POSITION,
    TERMINAL_ATTRIBUTE,
    TERMINAL_GENERICRESPON,
    TERMINAL_DATAUPLOAD,
    TERMINAL_BATCHUPLOAD,
} JT808_PROTOCOL;

typedef struct
{
    //位置基本信息
    uint32_t alarm;  		//报警标志
    uint32_t status; 		//状态
    uint32_t latitude;		//维度
    uint32_t longtitude;	//经度
    uint16_t hight;			//高度
    uint16_t speed;			//速度
    uint16_t course;		//方向
    uint8_t year;	//BCD时间 GMT+8
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t mintues;
    uint8_t seconds;
    uint8_t statelliteUsed;
} jt808Position_s;



typedef struct
{
    uint8_t jt808isRegister;
    uint8_t jt808sn[6];
    uint8_t jt808manufacturerID[5];
    uint8_t jt808terminalType[20];
    uint8_t jt808terminalID[7];
    uint8_t jt808AuthCode[50];
    uint8_t jt808AuthLen;
    uint16_t serial;

    int (*jt808Send)(uint8_t, uint8_t *, uint16_t);
} jt808Info_s;

typedef struct
{
    uint16_t platform_serial;
    uint16_t platform_id;
    uint8_t  result;
} jt808TerminalRsp;

typedef struct
{
    uint8_t id;
    void *param;
} trammission_s;

typedef struct{
	gpsRestore_s *grs;
	uint16_t num;
	uint8_t type;
}batch_upload_s;


void jt808InfoInit(void);
void jt808CreateSn(uint8_t *jt808sn, uint8_t *imei, uint8_t snlen);
void jt808UpdateStatus(uint32_t bit, uint8_t onoff);
void jt808UpdateAlarm(uint32_t bit, uint8_t onoff);
void jt808RegisterTcpSend(int (*tcpSend)(uint8_t, uint8_t *, uint16_t));
void jt808RegisterLoginInfo(uint8_t *sn, uint8_t reg, uint8_t *authCode, uint8_t authLen);
void jt808RegisterManufactureId(uint8_t *id);
void jt808RegisterTerminalType(uint8_t *type);
void jt808RegisterTerminalId(uint8_t *id);

int jt808TcpSend(uint8_t *data, uint16_t len);



uint16_t jt808gpsRestoreDataSend(uint8_t *dest, gpsRestore_s *grs);
void jt808SendToServer(JT808_PROTOCOL protocol, void *param);
void jt808MessageSend(uint8_t *buf, uint16_t len);
void jt808ReceiveParser(uint8_t *src, uint16_t len);

void jt808BatchInit(void);
void jt808BatchRestorePushIn(gpsRestore_s *grs , uint8_t type);
void jt808BatchPushIn(gpsinfo_s *gpsinfo);
void jt808BatchPushOut(void);



#endif

