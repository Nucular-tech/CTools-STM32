/*
 * Cross platform STM32 flash HAL
 */
#pragma once

#include <stdint.h>

typedef enum {
	Flash_OK,
	Flash_ERR
} FlashState;

typedef enum {
	FlashReadProtection_Level0,
	FlashReadProtection_Level1,
	FlashReadProtection_Level2,
} FlashReadProtection;

FlashState Flash_Erase(void* pageAddress);
FlashState Flash_Write(void* address, void* data, uint16_t count);
uint16_t Flash_GetSector(uint32_t address);

void Flash_UnlockOption(void);
void Flash_ProgramAndLockOption(void);	
FlashState Flash_SetWriteProtect(uint32_t from, uint32_t to);
FlashReadProtection Flash_GetReadProtection(void);
