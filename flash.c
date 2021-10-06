#include "flash.h"
#include "Resconfig.h"

#define FLASH_KEY1 ((uint32_t)0x45670123)
#define FLASH_KEY2 ((uint32_t)0xCDEF89AB)

#define FLASH_OPT_KEY1 ((uint32_t)0x08192A3B)
#define FLASH_OPT_KEY2 ((uint32_t)0x4C5D6E7F)

#ifdef STM32L4
#define EMPTY_FLASH 0xFFFFFFFF
#else
#define EMPTY_FLASH 0xFFFF
#endif 

//ld scripts global variables
//extern uint32_t __flash_start__;
extern uint32_t __boot_start__;
extern uint32_t __data_start__;

void flash_unlockAndClean(void);

FlashState Flash_Erase(void* pageAddress) {
	__disable_irq();
	//open access
	flash_unlockAndClean();

#ifdef STM32F4
	//STM F4 EOP set only if EOPIE set, flash by 16 bit, erase CR register!!!!
	FLASH->CR = FLASH_CR_EOPIE | FLASH_CR_PSIZE_0;
#endif
#ifdef STM32L4
	FLASH->CR = FLASH_CR_EOPIE;
#endif

	while(FLASH->SR & FLASH_SR_BSY)
		;
	if(FLASH->SR & FLASH_SR_EOP) {
		FLASH->SR = FLASH_SR_EOP;
	}
	//this is RAM area?
	if((uint32_t)pageAddress > (uint32_t)(&__data_start__))
		return Flash_ERR;
#ifdef STM32L4
	uint32_t pageindex = Flash_GetSector((uint32_t)pageAddress);
	FLASH->CR |= FLASH_CR_PER | pageindex << 3;
#endif
#ifdef STM32F4
	uint32_t pageindex = Flash_GetSector((uint32_t)pageAddress);
	FLASH->CR |= FLASH_CR_SER | pageindex << 3;
#endif
#ifdef STM32F1
	FLASH->CR |= FLASH_CR_PER;
	FLASH->AR = (uint32_t)pageAddress;
#endif

	FLASH->CR |= FLASH_CR_STRT;
	while(!(FLASH->SR & FLASH_SR_EOP))
		;
	FLASH->SR = FLASH_SR_EOP;
#ifdef STM32F4
	FLASH->CR &= ~FLASH_CR_SER;
#else
	FLASH->CR &= ~FLASH_CR_PER;
#endif
	//close flash access
	FLASH->CR |= FLASH_CR_LOCK;
	__enable_irq();
	return Flash_OK;
}

FlashState Flash_Write(void* address, void* data, uint16_t count) {

	uint32_t i;
#ifdef STM32L4
	uint32_t* from = data;
	uint32_t* to = address;
#else
	uint16_t* from = data;
	uint16_t* to = address;
#endif
	//open access
	flash_unlockAndClean();
#ifdef STM32L4
	//64 bit align
	uint16_t cnt = (count + 7) / 8;
	cnt = cnt * 2;     //DWORD
#else
	//16 bit align
	uint16_t cnt = (count + 1) / 2;
#endif
	//check for sector empty, erase if not
	for(i = 0; i < cnt; i++) {
		if(to[i] != EMPTY_FLASH) {
			//close flash access 	
			FLASH->CR |= FLASH_CR_LOCK;
			//if erase goes wrong, everything is wrong
			return Flash_ERR;
		}
	}
#ifdef STM32F4
	//STM F4 EOP set only if EOPIE set, flash by 16 bit, erase CR register!!!!
	FLASH->CR = FLASH_CR_EOPIE | FLASH_CR_PSIZE_0;
#endif
#ifdef STM32L4
	FLASH->CR = FLASH_CR_EOPIE;
#endif

	while(FLASH->SR & FLASH_SR_BSY)
		;
	if(FLASH->SR & FLASH_SR_EOP) {
		FLASH->SR = FLASH_SR_EOP;
	}
	FLASH->CR |= FLASH_CR_PG;
	//round up
	for(i = 0; i < cnt; i++) {
		to[i] = from[i];
#ifdef STM32L4
		i++;
		to[i] = from[i];
#endif
		while(!(FLASH->SR & FLASH_SR_EOP))
			;
		FLASH->SR = FLASH_SR_EOP;
	}
	FLASH->CR &= ~FLASH_CR_PG;
	//close flash access
	FLASH->CR |= FLASH_CR_LOCK;
	return Flash_OK;
}

uint16_t Flash_GetSector(uint32_t address) {
	uint32_t pageindex = 0;
	//boot used as starting number for all sectors (since it's boot)
	address -= (uint32_t)(&__boot_start__);
#ifdef STM32F4
	//flash in F4 consist of 16KB x4 + 64KB x1 + 128KB other six sectors
	//RTFM <Table 5. Flash module organization (STM32F40x and STM32F41x)>
	if(address < (16 * 4 * 1024))
		pageindex = address / (16 * 1024);
	else if(address < (128 * 1024))
		pageindex = 4;
	else {
		address -= 128 * 1024;
		pageindex = address / (128 * 1024) + 5;
	}
#else
	pageindex = address / (FLASH_PAGESIZE);
#endif
	return pageindex;
}

void Flash_UnlockOption(void) {
	//check busy and unlock	
	if(FLASH->CR & FLASH_CR_LOCK) {
		FLASH->KEYR = FLASH_KEY1;
		FLASH->KEYR = FLASH_KEY2;
	}
	while(FLASH->SR & FLASH_SR_BSY)
		;
#ifdef STM32F1
	FLASH->OPTKEYR = FLASH_KEY1;
	FLASH->OPTKEYR = FLASH_KEY2;
#else
	FLASH->OPTKEYR = FLASH_OPT_KEY1;
	FLASH->OPTKEYR = FLASH_OPT_KEY2;
#endif

}

void flash_unlockAndClean(void){
	if(FLASH->CR & FLASH_CR_LOCK) {
		FLASH->KEYR = FLASH_KEY1;
		FLASH->KEYR = FLASH_KEY2;
	}
	//reset all errors
	FLASH->SR = 0xFFFF;
}

#ifdef STM32L4
void Flash_ProgramAndLockOption(void) {
	//program and wait
	__BKPT();
	FLASH->CR |= FLASH_CR_OPTSTRT;
	while(FLASH->SR & FLASH_SR_BSY)
		;
	//Set the bit to force the option byte reloading
	SET_BIT(FLASH->CR, FLASH_CR_OBL_LAUNCH);
}

FlashState Flash_SetWriteProtect(uint32_t from, uint32_t to) {
	static int wpindex = 0;

	from -= (uint32_t)(&__boot_start__);
	from /= FLASH_PAGESIZE;
	to -= (uint32_t)(&__boot_start__);
	to /= FLASH_PAGESIZE;
	uint32_t wrp = (from << FLASH_WRP1AR_WRP1A_STRT_Pos) | (to << FLASH_WRP1AR_WRP1A_END_Pos);
	if(wpindex == 0) {
		FLASH->WRP1AR = wrp;
		wpindex++;
	}
	else
		FLASH->WRP1BR = wrp;
	return Flash_OK;
}


FlashReadProtection Flash_GetReadProtection(void) {
	uint8_t protection = FlashReadProtection_Level0;
#ifdef STM32L4
	protection = (FLASH->OPTR & 0xFF);
#endif

	if(protection == 0xAA)
		return FlashReadProtection_Level0; //read protection not active
	else if(protection == 0xCC)
		return FlashReadProtection_Level2;     //chip read protection active
	else
		return FlashReadProtection_Level1;     //memories read protection active
}
#endif
