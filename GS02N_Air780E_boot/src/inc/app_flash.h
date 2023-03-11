/*
 * app_flash.h
 *
 *  Created on: 2021Äê10ÔÂ16ÈÕ
 *      Author: idea
 */

#ifndef APP_FLASH
#define APP_FLASH

#include "CH58x_common.h"


void flashEarseByFileSize(uint32_t offset, uint32_t totalsize);
uint8_t flashWriteCode(uint32_t offset, uint8_t *code, uint16_t len);

#endif /* SRC_INC_APP_FLASH_H_ */
