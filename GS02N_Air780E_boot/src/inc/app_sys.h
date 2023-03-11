/*
 * app_sys.h
 *
 *  Created on: Aug 2, 2022
 *      Author: idea
 */

#ifndef APP_SYS
#define APP_SYS

#include  "CH58x_common.h"


#define DEBUG_NONE              0
#define DEBUG_LOW               1
#define DEBUG_NET               2
#define DEBUG_GPS               3
#define DEBUG_FACTORY           4
#define DEBUG_BLE               5
#define DEBUG_ALL               9


#define NORMAL_LINK             0

typedef struct{
    uint8_t logLevel;

}sysinfo_s;


unsigned short GetCrc16(const char *pData, int nLength);

void LogMessage(uint8_t level, char *debug);
void LogMessageWL(uint8_t level, char *buf, uint16_t len);
void LogPrintf(uint8_t level, const char *debug, ...);

uint8_t mycmdPatch(uint8_t *cmd1, uint8_t *cmd2);
int getCharIndex(uint8_t *src, int src_len, char ch);
int my_strpach(char *str1, const char *str2);
int my_getstrindex(char *str1, const char *str2, int len);
int my_strstr(char *str1, const char *str2, int len);
int distinguishOK(char *buf);
int getCharIndexWithNum(uint8_t *src, uint16_t src_len, uint8_t ch, uint8_t num);
void byteToHexString(uint8_t *src, uint8_t *dest, uint16_t srclen);

#endif /* SRC_INC_APP_SYS_H_ */
