#include <stdint.h>
#include <stdlib.h>
#include "string.h"
#ifndef Flash_Erase
#include "flash.h"
#endif
#include "storage.h"
#include "Resconfig.h"

typedef struct {
	uint16_t Size; //first saved on power off
	uint8_t ID;
	uint8_t Version;
	uint32_t CRC_result;
} header;

const StorageData_t *storageStructInfo;
uint16_t storageInfoSize = 0;
intptr_t *banks[2] = { 0 };
int32_t cnt[2] = { 0 };
int valid[2] = { 0 };
int activeBank = 0;
uint8_t banksSwap = 0;
enum {
	Bank0, Bank1
};

#define CRC_CONST 0xC0FFEE
//local functions
void checkBanks(void);
header* searchLatestValid(uint8_t structindex, const intptr_t *bank);
void eraseBank(uint8_t bankIndex);
#ifdef STORAGE_CRC
uint32_t calculateCRC(const void *address, uint16_t n);
#endif
header* searchEnd(intptr_t *bank);
int checkEmptyEnd(intptr_t *bank, header *header);
int32_t checkStructuresAndValidate(intptr_t *bank, int32_t *count);
uint32_t getAlignedSizeWithHeader(uint32_t size);
int save_struct(intptr_t *address, uint32_t size, uint8_t structindex, uint8_t structversion);
int moveStructuresAndErase(void);

void Storage_Init(const StorageData_t *storageInfo, uint16_t storageSize, intptr_t *bank1, intptr_t *bank2) {
#ifdef STORAGE_CRC
#if defined(STM32F4) | defined(STM32L4)
	RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;
#endif
#if defined (STM32F030) | defined (STM32F031) | defined (STM32F051) | defined (STM32F072) | defined (STM32F042)
	RCC->AHBENR |= RCC_AHBENR_CRCEN;
#endif
#endif
	storageStructInfo = storageInfo;
	storageInfoSize = storageSize;

	banks[Bank0] = bank1, banks[Bank1] = bank2;
	if (banks[Bank0] == 0 || banks[Bank1] == 0)
		return;
	//get valid bank
	checkBanks();
	recoverdata: //step to recover data in case if move structs fails, this may happen when both banks are full!
	if (valid[activeBank] == 0) {
		//we have a broken storage, store data in a stack and erase & write it again
		//probably data got damaged while quick save, so active bank should have 1 or 2 structures
		//if this code interrupted we will loose only small portion of data
		if (cnt[!activeBank] == 0 && valid[!activeBank] == 1) {
			//inactive bank valid and empty, write data to it without buffer
			activeBank = !activeBank;
		} else {
			uint32_t totalsize = 0;
			for (int i = 0; i < storageInfoSize; i++) {
				header *validhdr = searchLatestValid(i, banks[activeBank]);
				if (validhdr) {
					totalsize += getAlignedSizeWithHeader(validhdr->Size);
				}
			}
			if (totalsize > 0) {
				//temporary heap bank
				uint8_t buffer[totalsize];

				uint32_t position = 0;
				for (int i = 0; i < storageInfoSize; i++) {
					header *validhdr = searchLatestValid(i, banks[activeBank]);
					if (validhdr) {
						memcpy(&buffer[position], validhdr, validhdr->Size + sizeof(header));
						position += getAlignedSizeWithHeader(validhdr->Size);
					}
				}
				int attempt = 3;
				//try erase multiple time
				do {
					attempt--;
					eraseBank(activeBank);
					valid[activeBank] = checkStructuresAndValidate(banks[activeBank], &cnt[activeBank]);
				} while (valid[activeBank] == 0 && attempt > 0);

				//save using pointers
				Flash_Write(banks[activeBank], &buffer, totalsize);
				cnt[activeBank] = totalsize;
			} else {
				eraseBank(activeBank);
			}
		}
	}
	//transfer leftovers from old bank and erase it
	if (cnt[!activeBank] > 0 || valid[!activeBank] == 0) {
		if (moveStructuresAndErase() == EXIT_FAILURE) //move always erases INACTIVE banks
		{
			//cant save anything, so it is invalid
			valid[activeBank] = 0;
			goto recoverdata;
		}
	}
	//at the end of this route always have 1 EMPTY bank for quick save
}

uint32_t getAlignedSizeWithHeader(uint32_t size) {
	uint32_t rdup = (size + sizeof(header) + FLASH_ALIGN - 1) / FLASH_ALIGN;
	return rdup * FLASH_ALIGN;
}

/// Wipes all storage, but keep data in RAM
void Storage_Wipe(void) {
	eraseBank(Bank0);
	eraseBank(Bank1);
	activeBank = 0;
	banksSwap = 0;
	//saveAddress = 0;
}

void Storage_SaveAll(void) {
	for (uint8_t i = 0; i < storageInfoSize; i++) {
		Storage_SaveData(i); //length mismatch
	}
}
/// Returns pointer and size to latest valid data
/// @param structindex Data index
/// @return returns 0=ERROR if version or size differ, 1=SUCCESS
StorageData_t Storage_GetData(uint8_t structindex) {
	header *dat = searchLatestValid(structindex, banks[activeBank]);
	StorageData_t str = { 0 };
	if (dat != 0) {
		str.Size = dat->Size;
		str.Version = dat->Version;
		str.Data = (intptr_t*) (dat + 1);
	}
	return str;
}

/// Loads data array by specified index
/// @param structindex Data index
/// @return returns 1=EXIT_FAILURE if version or size differ, 0=EXIT_SUCCESS
int Storage_LoadData(uint8_t structindex) {
	if (storageStructInfo[structindex].Data == 0)
		return EXIT_FAILURE; //empty object
	StorageData_t data = Storage_GetData(structindex);
	if (data.Size != storageStructInfo[structindex].Size)
		return EXIT_FAILURE; //different size
	if (data.Version != storageStructInfo[structindex].Version)
		return EXIT_FAILURE; //different version
	memcpy((void*) storageStructInfo[structindex].Data, data.Data, data.Size);
	return EXIT_SUCCESS;
}

int Storage_SaveData(uint8_t structindex) {
	int res = Storage_SaveQuick(structindex);
	if (banksSwap) {
		//move other structures also. if this code failed on power off - storage_init will re-copy again
		moveStructuresAndErase();
		banksSwap = 0;
		//try again?
		if (res != EXIT_SUCCESS) {
			res = Storage_SaveQuick(structindex);
		}
	}
	return res;
}

int Storage_SaveQuick(uint8_t structindex) {
	return save_struct(storageStructInfo[structindex].Data, storageStructInfo[structindex].Size, structindex, storageStructInfo[structindex].Version);
}

int save_struct(intptr_t *address, uint32_t size, uint8_t structindex, uint8_t structversion) {
	static volatile int semaphore = 0;
	if (address == 0 || size == 0 || semaphore == 1)
		return EXIT_FAILURE;

	semaphore = 1;
	header *saveAddress = 0;
	intptr_t bankEnd = (intptr_t) banks[activeBank] + FLASH_PAGE_SIZE_DATA * FLASH_PAGES_IN_BANK;
	//check position is in range of active bank
	//if (saveAddress == 0 || saveAddress->Size != 0xFFFF || (intptr_t) saveAddress < (intptr_t) banks[activeBank] || (intptr_t) saveAddress > bankEnd)
	saveAddress = searchEnd(banks[activeBank]);

	uint32_t byteCount = getAlignedSizeWithHeader(size);
	//enough space?
	if (saveAddress == 0 || ((bankEnd - (intptr_t) saveAddress) < (intptr_t) byteCount)) {
		//exchange banks
		if (cnt[!activeBank] > 0 || banksSwap == 1 || valid[!activeBank] == 0) {
			semaphore = 0;
			banksSwap = 1; //force to move
			return EXIT_FAILURE; //invalid inactive bank or already moved
		}
		activeBank = !activeBank;
		banksSwap = 1;
		//if bank moved, it should be empty
		saveAddress = (header*) banks[activeBank];
	}

	//init header
	header newsave = { 0 };
	newsave.ID = structindex;
	newsave.Size = size;
	newsave.Version = structversion;
#ifdef STORAGE_CRC
	newsave.CRC_result = calculateCRC(address, size);
#else
	newsave.CRC_result = CRC_CONST;
#endif
	//save header first
	Flash_Write(saveAddress, &newsave, sizeof(header));
	//save data after header
	Flash_Write(saveAddress + 1, address, size);
	cnt[activeBank] += byteCount;
	semaphore = 0;
	return EXIT_SUCCESS;
}

void eraseBank(uint8_t bankIndex) {
	if (bankIndex > 1)
		return;
	intptr_t *bankSelected = banks[bankIndex];
	for (uint8_t i = 0; i < FLASH_PAGES_IN_BANK; i++) {
		Flash_Erase((intptr_t*) ((intptr_t) bankSelected + FLASH_PAGE_SIZE_DATA * i));
	}
	valid[bankIndex] = 1;
	cnt[bankIndex] = 0;
}

int moveStructuresAndErase(void) {
	int res = EXIT_SUCCESS;
	//Move structures from old one
	for (int i = 0; i < storageInfoSize; i++) {
		header *exist = searchLatestValid(i, banks[activeBank]);
		//there is no such struct?
		if (exist == 0) {
			//try to get it in old bank
			exist = searchLatestValid(i, banks[!activeBank]);
			//exists?
			if (exist != 0) {
				//transfer from invalid bank
				res = save_struct((intptr_t*) (exist + 1), exist->Size, exist->ID, exist->Version);
				//probably cant transfer anything behind this point
				if (res == EXIT_FAILURE)
					return res;
			}
		}
	}
	//clean up old - it should be prepeared for quick save
	eraseBank(!activeBank);
	return res;
}

#ifdef STORAGE_CRC
uint32_t calculateCRC(const void *address, uint16_t n) {
	uint16_t i;
	uint32_t *addr = (uint32_t*) address;
	CRC->CR = CRC_CR_RESET;
	while (CRC->DR != 0xFFFFFFFF)
		;
	CRC->DR = 0xDEADBEEF;
	uint16_t bit32_count = n / 4;
	for (i = 0; i < bit32_count; i++) {
		CRC->DR = addr[i];
	}
	//calculate bytes not fit in 32 bit
	n = n - bit32_count * 4;
	//for not 32-bit size buffer
	if (n > 0) {
		uint32_t data = addr[i]; //get last bytes, with unused memory
		data |= (uint32_t) 0xFFFFFFFF << (8 * n); //fill other bytes with 0xFF's
		CRC->DR = data; //CRC!
	}

	return CRC->DR;
}
#endif

void checkBanks(void) {
	valid[Bank0] = checkStructuresAndValidate(banks[Bank0], &cnt[Bank0]);
	valid[Bank1] = checkStructuresAndValidate(banks[Bank1], &cnt[Bank1]);

	//selected bank
	//check if banks are not-empty
	if (cnt[Bank0] == 0) {
		if (cnt[Bank1] == 0) {
			activeBank = Bank0;
		} else {
			activeBank = Bank1;
		}
	} else {
		if (cnt[Bank1] == 0) {
			activeBank = Bank0;
		} else {
			//every bank should contain every struct, so older bank is one with more structs
			if (cnt[Bank1] < cnt[Bank0]) {
				activeBank = Bank1;
			} else {
				activeBank = Bank0;
			}
		}
	}
}

header* searchLatestValid(uint8_t structindex, const intptr_t *bank) {
	//escape null type data
	if (storageStructInfo[structindex].Data == 0 || storageStructInfo[structindex].Size == 0 || bank == 0)
		return 0;

	header *head = (header*) bank;
	header *data = 0;
	intptr_t bankEnd = (intptr_t) bank + FLASH_PAGE_SIZE_DATA * FLASH_PAGES_IN_BANK;
	do {
		//FFFF=end, bank oversized = error. todo add error check?
		if (/*(head->Size == 0) ||*/(head->Size == UINT16_MAX) || ((intptr_t) head + (intptr_t) getAlignedSizeWithHeader(head->Size) > bankEnd))
			return data;

		if (head->ID == structindex) {
			//validate, head +1 = (uint32_t)head+ sizeof(header) go to data address
#ifdef STORAGE_CRC
			uint32_t crc = calculateCRC(head + 1, head->Size);
#else
			uint32_t crc = CRC_CONST;
#endif
			//todo add markers to previous data to speed up searching and CRC only latest one, if incorrect go back
			if (crc == head->CRC_result) {
				data = head;
			} else {
#ifdef DEBUG
				trace_printf("CRC storage mismatch id %d\n", head->ID);
#endif
			}
		}
		head = (header*) ((intptr_t) head + getAlignedSizeWithHeader(head->Size));
	} while ((intptr_t) head < bankEnd);
	return data;
}

header* searchEnd(intptr_t *bank) {
	header *head = (header*) bank;
	do {
		if (head->Size == UINT16_MAX)
			return head;
		uint32_t rdup = (head->Size + FLASH_ALIGN - 1) / FLASH_ALIGN;
		//round size up, always 16 bit align on ending
		head = (header*) ((intptr_t) head + rdup * FLASH_ALIGN + sizeof(header));
	} while ((intptr_t) head < ((intptr_t) bank + (FLASH_PAGE_SIZE_DATA * FLASH_PAGES_IN_BANK)));
	return 0; // no end found
}

int32_t checkStructuresAndValidate(intptr_t *bank, int32_t *count) {
	int32_t valid = 1;
	int32_t counter = 0; //bytes written
	header *head = (header*) bank;
	intptr_t end = (intptr_t) bank + (FLASH_PAGE_SIZE_DATA * FLASH_PAGES_IN_BANK);
	//calculate structures at start
	do {
		//detect empty area beginning, allow size =0 cos some structures had it ;(
		if (head->Size == UINT16_MAX)
			break;
		//round size up, always FLASH_ALIGN bytes align on ending
		uint32_t size = getAlignedSizeWithHeader(head->Size);
		//increment kinda-real structures
		if ((intptr_t) ((intptr_t) head + size) > end) {
			//boundary error detected
			valid = 0;
			break;
		} else {
			//count only valid size
			counter += size;
		}
		head = (header*) (size + (intptr_t) head);
	} while ((intptr_t) head < end);

	//scan end of flash that it is empty.
	uint32_t *ptr = (uint32_t*) head;
	for (; (intptr_t) ptr < end && valid != 0; ptr++) {
		if (*ptr != UINT32_MAX) {
			valid = 0; //FAULT
		}
	}

	if (count) {
		*count = counter;
	}
	return valid;
}

int checkEmptyEnd(intptr_t *bank, header *header) {
	intptr_t *position = (intptr_t*) header;
	intptr_t end = ((intptr_t) bank + (FLASH_PAGE_SIZE_DATA * FLASH_PAGES_IN_BANK));
	if (header == 0 || (intptr_t) header == end) {
		return EXIT_FAILURE;
	}
	for (; (intptr_t) position < end; position++) {

		if ((uintptr_t) *position != UINTPTR_MAX) {
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}
