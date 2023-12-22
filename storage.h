/*
 * storage.h
 *
 *  Created on: 6 окт. 2017 г.
 *      Author: VasiliSk
 */

#pragma once

#include <stdint.h>

typedef struct {
	uint16_t Size;
	uint8_t Version;
	intptr_t* Data;
} StorageData_t;

void Storage_Init(const StorageData_t *storageInfo, uint16_t storageSize, intptr_t *bank1, intptr_t *bank2);
StorageData_t Storage_GetData(uint8_t structindex);
int Storage_LoadData(uint8_t structindex);
int Storage_SaveData(uint8_t structindex);
int Storage_SaveQuick(uint8_t structindex);
void Storage_Wipe(void);
void Storage_SaveAll(void);
