/*
 * app_encrypt.c
 *
 *  Created on: May 8, 2022
 *      Author: idea
 */
#include "app_encrypt.h"
#include "app_port.h"
#include "app_sys.h"
/*
 * @加密
 */
static void encrypt(uint8 *enc, uint8 seed, uint8 *denc)
{
    uint8 i;
    uint8 encryptData[4];
    //源数据
    //加密1
    for (i = 0; i < 4; i++)
    {
        encryptData[i] = (enc[i] + seed + i) % 0x100;
    }
    //对换
    encryptData[0] = encryptData[0] ^ encryptData[3];
    encryptData[3] = encryptData[0] ^ encryptData[3];
    encryptData[0] = encryptData[0] ^ encryptData[3];
    encryptData[1] = encryptData[1] ^ encryptData[2];
    encryptData[2] = encryptData[1] ^ encryptData[2];
    encryptData[1] = encryptData[1] ^ encryptData[2];
    //加密2
    for (i = 0; i < 4; i++)
    {
        encryptData[i] = encryptData[i] ^ (seed + i);
    }
    //加密3
    encryptData[0] = encryptData[0] ^ encryptData[2];
    encryptData[1] = encryptData[1] ^ encryptData[3];
    memcpy(denc, encryptData, 4);
}
/*
 * @解密
 */
static void decrypt(uint8 *data, uint8 seed, uint8 *denc)
{
    uint8 i;
    uint8 dencryptData[4];
    //源数据
    data[0] = data[0] ^ data[2];
    data[1] = data[1] ^ data[3];
    for (i = 0; i < 4; i++)
    {
        dencryptData[i] = data[i] ^ (seed + i);
    }

    dencryptData[0] = dencryptData[0] ^ dencryptData[3];
    dencryptData[3] = dencryptData[0] ^ dencryptData[3];
    dencryptData[0] = dencryptData[0] ^ dencryptData[3];
    dencryptData[1] = dencryptData[1] ^ dencryptData[2];
    dencryptData[2] = dencryptData[1] ^ dencryptData[2];
    dencryptData[1] = dencryptData[1] ^ dencryptData[2];
    //对换

    for (i = 0; i < 4; i++)
    {
        dencryptData[i] = (dencryptData[i] + 0x100 - (seed + i)) % 0x100;
    }
    memcpy(denc, dencryptData, 4);
}

/*
 * @加密数据
 * @bref    加密数据最长不要超过250个字节，加密数据后不足4个字节的，会以0补充
 * @param
 *  seed    随机种子
 *  data    待加密数据
 *  len     待加密数据长度
 *  outData 加密数据存放地址
 *  outDataLen  加密数据长度
 * @return
 *  1       成功
 *  0       失败
 */
uint8 encryptStr(uint8 seed, uint8 *data, uint8 len, uint8 *outData, uint8 *outDataLen)
{
    uint8 usableSize, used, myEncDataLen;
    uint8 en4[4], den4[4];
    if (data == NULL || len == 0 || outData == NULL || outDataLen == NULL)
    {
        return  0;
    }
    myEncDataLen = 0;
    outData[myEncDataLen++] = seed;
    used = 0;
    do
    {
        usableSize = (len - used) > 4 ? 4 : (len - used);
        memset(en4, 0, 4);
        memcpy(en4, data + used, usableSize);
        encrypt(en4, seed, den4);
        memcpy(outData + myEncDataLen, den4, 4);
        myEncDataLen += 4;
        used += usableSize;
    }
    while (used < len);
    *outDataLen = myEncDataLen;
    return 1;
}
/*
 * @解密数据
 * @bref    由于解密数据后不足4字节部分以0补充，所以解密后0的数据丢掉
 * @param
 *  data    待解密数据
 *  len     待解密数据长度
 *  outData 解密数据存放地址
 *  outDataLen  解密数据长度
 * @return
 *  1       成功
 *  0       失败
 */
uint8 dencryptStr(uint8 *data, uint8 len, uint8 *outData, uint8 *outDataLen)
{
    uint8 usableSize, used, myEncDataLen;
    uint8 en4[4], den4[4], seed;
    if (data == NULL || len == 0 || outData == NULL || outDataLen == NULL)
    {
        return  0;
    }
    myEncDataLen = 0;
    seed = data[0];
    used = 1;
    do
    {
        usableSize = (len - used) > 4 ? 4 : (len - used);
        memset(en4, 0, 4);
        memcpy(en4, data + used, usableSize);
        decrypt(en4, seed, den4);
        memcpy(outData + myEncDataLen, den4, 4);
        myEncDataLen += 4;
        used += usableSize;
    }
    while (used < len);
    *outDataLen = myEncDataLen;
    return 1;
}
/********************************************************************************
 ********************************************************************************
 ********************************************************************************/
/*
 * 1、主机这端需要提前知道帽子的唯一ID
 * 2、主机连上帽子后，主机发送一道算术题
 * 3、帽子在2秒内回复主机，并携带帽子唯一ID以及答案
 * 4、如果唯一ID或答案错误，主机断开与帽子的链接
 * 5、只有鉴权成功后，主机与帽子之间才能保持长链接且数据通讯
 */
//主机端发起鉴权
uint32 startAuthentication(uint8 seed, uint16 a, uint16 b, sendCallBack callBack)
{
    uint8 data[20], enc[20], debugStr[100];
    uint8 encLen;
    uint32 result = 0;

    result = a + b + (a % 100) * (b % 100);
    //给出2个因数
    data[0] = a >> 8 & 0xFF;
    data[1] = a & 0xFF;
    data[2] = b >> 8 & 0xFF;
    data[3] = b & 0xFF;

    encryptStr(seed, data, 4, enc, &encLen);

    if (callBack != NULL)
    {
        callBack(enc, encLen);
    }
	//打印原始数据
    //memset(debugStr, 0, 100);
    //byteArrayToHexString(data, debugStr, 4);
    //LogPrintf(0, "原始:%s,结果%#X", debugStr, result);
	//打印加密数据
    //memset(debugStr, 0, 100);
    //byteArrayToHexString(enc, debugStr, encLen);
    //LogPrintf(0, "加密后:%s", debugStr);

    //调用蓝牙接口把 enc and encLen 发送出去

    //sendAuthenticationRespon(seed, "0000AAAA", cauculateAuthResult(enc, encLen));
    return result;
}
/*
 * 帽子解析
 * 解析收到的加密后的鉴权数据
 * 计算出结果
 */
uint32 cauculateAuthResult(uint8 *encryptData, uint8 encryptDataLen)
{
    uint8 dencrypt[20], dencryptLen;
    uint16 a;
    uint16 b;
    uint32 result = 0;

    dencryptStr(encryptData, encryptDataLen, dencrypt, &dencryptLen);

    a = dencrypt[0] << 8 | dencrypt[1];
    b = dencrypt[2] << 8 | dencrypt[3];
    result = a + b + (a % 100) * (b % 100);

	//对加密的鉴权数据进行解密，并得到结果
    LogPrintf(0, "cauculateAuthResult==>%08X", result);
    return result;
}

/*
 * 帽子回复
 * DevId    :唯一ID，8个字节长度
 * result   :计算的鉴权答案
 */
void sendAuthenticationRespon(uint8 seed, uint8 *DevId, uint32 result)
{
    uint8 data[20], i, dataLen = 0;
    uint8 encrypt[20], encryptLen;
    uint8 debugStr[100];
    for (i = 0; i < 8; i++)
    {
        data[dataLen++] = DevId[i];
    }
    data[dataLen++] = result >> 24 & 0xFF;
    data[dataLen++] = result >> 16 & 0xFF;
    data[dataLen++] = result >> 8 & 0xFF;
    data[dataLen++] = result & 0xFF;
    //进行加密
    encryptStr(seed, data, dataLen, encrypt, &encryptLen);

	//打印原始数据
    memset(debugStr, 0, 100);
    byteToHexString(data, debugStr, dataLen);
    LogPrintf(0, "原始:%s", debugStr);
	//打印加密后的数据
    memset(debugStr, 0, 100);
    byteToHexString(encrypt, debugStr, encryptLen);
    LogPrintf(0, "加密后:%s", debugStr);

    //调用蓝牙接口把数据传送出去
    //encrypt and encryptLen


}


void createEncrypt(uint8_t *mac, uint8_t *encBuff, uint8_t *encLen)
{
    uint8_t debug[50];
    uint8_t src[50];
    uint8_t srcLen;
    uint8_t dec[50];
    uint8_t decLen;
    uint8_t i;

    uint16_t year;
    uint8_t  month;
    uint8_t date;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;

    portGetRtcDateTime(&year, &month, &date, &hour, &minute, &second);

    srcLen = 0;
    //蓝牙MAC地址
    for (i = 0; i < 6; i++)
    {
        src[srcLen++] = mac[i];
    }
    //起始有效期：年月日时分
    src[srcLen++] = year % 100;
    src[srcLen++] = month;
    src[srcLen++] = date;
    src[srcLen++] = hour;
    src[srcLen++] = minute;
    //有效时长：分钟
    src[srcLen++] = 0xAA; //0xAA为免时长，相当于不判断时间有限性
    i = rand();
    //生成密钥
    encryptStr(i, src, srcLen, dec, &decLen);

    byteToHexString(mac, debug, 6);
    debug[12] = 0;
    LogPrintf(DEBUG_ALL, "MAC:[%s]", debug);

    byteToHexString(dec, debug, decLen);
    debug[decLen * 2] = 0;
    LogPrintf(DEBUG_ALL, "DEC:[%s],DECLEN:%d", debug, decLen);
    LogPrintf(DEBUG_ALL, "LimiteTime:[%02d/%02d/%02d %02d:%02d],Limit Minute:%d", year, month, date, hour, minute, 5);

    memcpy(encBuff, dec, decLen);
    *encLen = decLen;
}
