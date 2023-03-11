/*
 * app_flash.c
 *
 *  Created on: 2021年10月16日
 *      Author: idea
 */
#include "app_flash.h"
#include "app_sys.h"
/*
 * 代码区擦除
 */
void flashEarseByFileSize(uint32_t offset, uint32_t totalsize)
{
    int ret;
    uint16_t blockSize;
    blockSize = totalsize / EEPROM_BLOCK_SIZE;
    if (totalsize % EEPROM_BLOCK_SIZE != 0)
    {
        blockSize += 1;
    }

    ret = FLASH_ROM_ERASE(offset, EEPROM_BLOCK_SIZE * blockSize);
    LogPrintf(DEBUG_ALL, "Earse ret:%d", ret);
}
/*
 * 写入代码区
 */
uint8_t flashWriteCode(uint32_t offset, uint8_t *code, uint16_t len)
{
    int ret;
    uint8_t codeSize[1024];
    memcpy(codeSize, code, len);
    ret = FLASH_ROM_WRITE(offset, codeSize, 1024);
    LogPrintf(DEBUG_ALL, "Write ret:%d", ret);
    ret = FLASH_ROM_VERIFY(offset, codeSize, 1024);
    LogPrintf(DEBUG_ALL, "Verify ret:%d", ret);
    if (ret == 0)
        return 1;
    return 0;
}
