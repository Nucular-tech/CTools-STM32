#include "string.h"
#include "flash.h"
#include "storage.h"
#include "Resconfig.h"

typedef struct {
	uint16_t Size; //first saved on power off
	uint8_t ID;
	uint8_t Revision;
	uint32_t CRC_result;
} header;

//your structures, to be stored in flash. should be defined somewhere
extern const intptr_t *Storage_Address[];
extern const uint16_t Storage_Size[];

//ld scripts global varibles
extern unsigned int __storage_start__;
//todo add check on storage end
extern unsigned int __storage_end__;

#define FLASH_ADDRES_BANK0  (intptr_t*)(&__storage_start__)
#define FLASH_ADDRES_BANK1 (intptr_t*)((uint32_t)(&__storage_start__) + FLASH_PAGE_SIZE_DATA * FLASH_PAGES_IN_BANK)
intptr_t *validBank = 0;
intptr_t *invalidBank = 0;
header *writingPosition = 0;

//local functions
void getValidBank(void);
header* searchLatestValid(uint8_t structindex, const intptr_t *bank);
void eraseBank(intptr_t *bank);
uint32_t calculateCRC(const void *address, uint16_t n);
header* searchEnd(intptr_t *bank);
uint8_t checkEmptyEnd(intptr_t *bank, header *header);

void Storage_Init(void) {
#ifdef STORAGE_CRC
#if defined(STM32F4) | defined(STM32L4)
	RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;
#endif
#if defined (STM32F030) | defined (STM32F031) | defined (STM32F051) | defined (STM32F072) | defined (STM32F042)
	RCC->AHBENR |= RCC_AHBENR_CRCEN;
#endif
#endif
	//get valid bank
	getValidBank();
	writingPosition = searchEnd(validBank);
	//fill in all structs in bank always
	for (int i = 0; i < Struct_number; i++) {
		header *valid = searchLatestValid(i, validBank);
		//there is no such struct?
		if (valid == 0) {
			//try to get it in old bank
			valid = searchLatestValid(i, invalidBank);
			//exists?
			if (valid != 0) {
				//transfer from invalid bank
				header *tostr = searchEnd(validBank);
				Flash_Write(tostr, valid, valid->Size + sizeof(header));
			} else
				Storage_SaveData(i);
		} else {
			//same size = copy
			/*if (valid->Size == Storage_Size[i]) {
			 memcpy(Storage_Address[i], valid + 1, valid->Size);
			 }else
			 Storage_SaveData(i); //update new size*/
		}
	}
	//clear invalid bank if needed
	for (int i = 0; i < (FLASH_PAGE_SIZE_DATA * FLASH_PAGES_IN_BANK / 4); i++) {
		if (((uint32_t*) invalidBank)[i] != UINT32_MAX) {
			eraseBank(invalidBank);
			break;
		}
	}
	/*
	 for (uint8_t i = 0; i < Struct_number; i++) {
	 if (Storage_LoadData(i) == ERROR) {
	 Storage_SaveData(i); //length mismatch
	 }
	 }*/
}

/// Wipes all storage, but keep data in RAM
void Storage_Wipe(void) {
	eraseBank(FLASH_ADDRES_BANK0);
	eraseBank(FLASH_ADDRES_BANK1);

	validBank = FLASH_ADDRES_BANK0;
	writingPosition = searchEnd(validBank);
}

void Storage_SaveAll(void) {
	for (uint8_t i = 0; i < Struct_number; i++) {
		Storage_SaveData(i); //length mismatch
	}
}
/// Returns pointer and size to latest valid data
/// @param structindex Data index
/// @return returns 0=ERROR if version or size differ, 1=SUCCESS
Storage Storage_GetData(uint8_t structindex) {
	header *dat = searchLatestValid(structindex, validBank);
	Storage str;
	if (dat == 0) {
		str.Size = 0;
		str.Data = 0;
	} else {
		str.Size = dat->Size;
		str.Data = (intptr_t*) (dat + 1);
	}
	return str;
}

/// Loads data array by specified index
/// @param structindex Data index
/// @return returns 0=ERROR if version or size differ, 1=SUCCESS
int Storage_LoadData(uint8_t structindex) {
	if (Storage_Address[structindex] == 0)
		return ERROR; //empty object
	Storage data = Storage_GetData(structindex);
	//todo add user version??
	if (data.Size != Storage_Size[structindex])
		return ERROR; //different size
	memcpy((void*) Storage_Address[structindex], data.Data, data.Size);
	return SUCCESS;
}

void Storage_SaveData(uint8_t structindex) {
	//check valid
	if (Storage_Address[structindex] == 0 || Storage_Size[structindex] == 0)
		return;

	uint16_t bankMoved = 0;
	intptr_t *oldvalidbank = validBank;
	header *lastSavedAdress = searchLatestValid(structindex, validBank);
	header *saveAddress = writingPosition;
	if (saveAddress->Size != 0xFFFF && (uint32_t) saveAddress > (uint32_t) validBank
			&& (uint32_t) saveAddress < ((uint32_t) validBank + FLASH_PAGE_SIZE_DATA * FLASH_PAGES_IN_BANK))
		saveAddress = searchEnd(validBank);

	header newsave;
	uint32_t byteCount = Storage_Size[structindex] + sizeof(header);
	//enough space?
	if (((((int32_t) validBank + FLASH_PAGE_SIZE_DATA * FLASH_PAGES_IN_BANK) - (int32_t) saveAddress) < (int32_t) byteCount) || saveAddress == 0) {
		//exchange banks
		intptr_t *temp = validBank;
		validBank = invalidBank;
		invalidBank = temp;
		bankMoved = 1;
		//if bank moved, it should be empty
		saveAddress = (header*) validBank;
		//bank not ready to use, erase
		if (saveAddress->Size != 0xFFFF)
			eraseBank(validBank); //this should not happen under normal conditions

	}
	//round rev
	if (lastSavedAdress->Revision == 255)
		newsave.Revision = 0;
	else
		newsave.Revision = lastSavedAdress->Revision + 1;

	//init header
	newsave.ID = structindex;
	newsave.Size = Storage_Size[structindex];
	newsave.CRC_result = calculateCRC(Storage_Address[structindex], Storage_Size[structindex]);
	//save header first
	Flash_Write(saveAddress, &newsave, sizeof(header));
	//save data after header
	Flash_Write(saveAddress + 1, (intptr_t*) Storage_Address[structindex], Storage_Size[structindex]);
	//round size up, always 32 bit align on ending
	uint32_t rdup = ((uint32_t) newsave.Size + FLASH_ALIGN - 1) / FLASH_ALIGN;
	//update writing position
	writingPosition = (header*) ((uint32_t) saveAddress + rdup * FLASH_ALIGN + sizeof(header));
	//move other structures also. if this code failed on power off storage_init will re-copy again
	if (bankMoved) {
		//copy all structs from old bank to new excl. already one copied
		for (uint8_t i = 0; i < Struct_number; i++) {
			if (i != structindex) {
				header *fromstr = searchLatestValid(i, oldvalidbank);
				header *tostr = searchEnd(validBank);
				Flash_Write(tostr, fromstr, fromstr->Size + sizeof(header));
			}
		}
		eraseBank(oldvalidbank);
	}
}

void eraseBank(intptr_t *bank) {
	for (uint8_t i = 0; i < FLASH_PAGES_IN_BANK; i++) {
		Flash_Erase((intptr_t*) ((uint32_t) bank + FLASH_PAGE_SIZE_DATA * i));
	}
}

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

void getValidBank(void) {
	header *bank0 = (header*) FLASH_ADDRES_BANK0;
	header *bank1 = (header*) FLASH_ADDRES_BANK1;
	if (bank0->Size == 0xFFFF) {
		if (bank1->Size == 0xFFFF) {
			validBank = FLASH_ADDRES_BANK0;
			invalidBank = FLASH_ADDRES_BANK1;
		} else {
			validBank = FLASH_ADDRES_BANK1;
			invalidBank = FLASH_ADDRES_BANK0;
		}
	} else {
		if (bank1->Size == 0xFFFF) {
			validBank = FLASH_ADDRES_BANK0;
			invalidBank = FLASH_ADDRES_BANK1;
		} else {
			//every bank should contain every struct, so older bank is one with more structs
			//get written length
			header *one = (header*) ((uint32_t) searchEnd( FLASH_ADDRES_BANK0) - (uint32_t) FLASH_ADDRES_BANK0 );
			header *two = (header*) ((uint32_t) searchEnd( FLASH_ADDRES_BANK1) - (uint32_t) FLASH_ADDRES_BANK1 );
			//check border conditions also
			if (two > one) {
				validBank = FLASH_ADDRES_BANK0;
				invalidBank = FLASH_ADDRES_BANK1;
			} else {
				validBank = FLASH_ADDRES_BANK1;
				invalidBank = FLASH_ADDRES_BANK0;
			}
		}
	}

	header *end = searchEnd(validBank);
	uint8_t bank_failed = checkEmptyEnd(validBank, end);
	if (bank_failed) {
		header *inv = (header*) invalidBank;
		if (inv->Size != 0xFFFF) {
			//there some data here, swap banks back
			invalidBank = validBank;
			validBank = (intptr_t*) inv;
			eraseBank(invalidBank);
		} else {
			//well..data is gone now
			eraseBank(validBank);
		}
	}
}

header* searchLatestValid(uint8_t structindex, const intptr_t *bank) {
	//escape null type data
	if (Storage_Address[structindex] == 0 || Storage_Size[structindex] == 0)
		return 0;

	header *head = (header*) bank;
	header *data = 0;

	do {
		//FFFF=end, bank oversized = error
		if ((head->Size == 0xFFFF) || ((uint32_t) head + (uint32_t) head->Size + sizeof(header) > (uint32_t) bank + (FLASH_PAGE_SIZE_DATA * FLASH_PAGES_IN_BANK)))
			return data;
		if (head->ID == structindex) {
			//new or zero roll-up?
			if ((data == 0) || (head->Revision == (uint8_t) (data->Revision + 1))) {
				//validate, head +1 = (uint32_t)head+ sizeof(header) go to data address
				uint32_t crc = calculateCRC(head + 1, head->Size);
				//todo add markers to previous data to speed up searching and CRC only latest one, if incorrect go back
				if (crc == head->CRC_result)
					data = head;
			}
		}

		uint32_t rdup = (head->Size + FLASH_ALIGN - 1) / FLASH_ALIGN;
		//round size up, always 32 bit align on ending
		head = (header*) ((uint32_t) head + rdup * FLASH_ALIGN + sizeof(header));

	} while ((uint32_t) head < ((uint32_t) bank + (FLASH_PAGE_SIZE_DATA * FLASH_PAGES_IN_BANK)));
	return data;
}

header* searchEnd(intptr_t *bank) {
	header *head = (header*) bank;
	do {
		if (head->Size == 0xFFFF)
			return head;
		uint32_t rdup = (head->Size + FLASH_ALIGN - 1) / FLASH_ALIGN;
		//round size up, always 16 bit align on ending
		head = (header*) ((uint32_t) head + rdup * FLASH_ALIGN + sizeof(header));
	} while ((uint32_t) head < ((uint32_t) bank + (FLASH_PAGE_SIZE_DATA * FLASH_PAGES_IN_BANK)));
	return 0;
}

uint8_t checkEmptyEnd(intptr_t *bank, header *header) {
	uint32_t *position = (uint32_t*) header;
	for (; (uint32_t)position < ((uint32_t) bank + (FLASH_PAGE_SIZE_DATA * FLASH_PAGES_IN_BANK)); position++) {

		if (*position != 0xFFFFFFFF)
			return 1;
	}
	return 0;
}
