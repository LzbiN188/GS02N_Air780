/*
 * app_encrypt.h
 *
 *  Created on: May 8, 2022
 *      Author: idea
 */

#ifndef APP_INCLUDE_APP_ENCRYPT_H_
#define APP_INCLUDE_APP_ENCRYPT_H_

#include "config.h"

typedef void (*sendCallBack)(uint8_t * ,uint16_t);

uint8 encryptStr(uint8 seed,uint8 * data ,uint8 len,uint8 * outData,uint8 * outDataLen);
uint8 dencryptStr(uint8 * data ,uint8 len,uint8 * outData,uint8 * outDataLen);


uint32 startAuthentication(uint8 seed,uint16 a , uint16 b,sendCallBack callback);
uint32 cauculateAuthResult(uint8 * encryptData , uint8 encryptDataLen);
void sendAuthenticationRespon(uint8 seed, uint8 * DevId,uint32 result);

void createEncrypt(uint8_t *mac, uint8_t *encBuff, uint8_t *encLen);
#endif /* APP_INCLUDE_APP_ENCRYPT_H_ */
