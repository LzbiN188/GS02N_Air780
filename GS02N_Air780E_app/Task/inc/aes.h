#ifndef AES_H
#define AES_H

#include "CH58x_common.h"

#define AESKEY	"ZTINFO----ZTINFO"
/**
 * 参数 p: 明文的字符串数组。
 * 参数 plen: 明文的长度,长度必须为16的倍数。
 * 参数 key: 密钥的字符串数组。
 */
void aes(char *p, int plen, char *key);

/**
 * 参数 c: 密文的字符串数组。
 * 参数 clen: 密文的长度,长度必须为16的倍数。
 * 参数 key: 密钥的字符串数组。
 */
void deAes(char *c, int clen, char *key);


void encryptData(char *encrypt,unsigned char *encryptlen,unsigned char *data,unsigned char len);
uint8_t dencryptData(char *dencrypt, unsigned char *dencryptlen,unsigned char *data, unsigned char len);

#endif

