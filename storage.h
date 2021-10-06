/*
 * storage.h
 *
 *  Created on: 6 ���. 2017 �.
 *      Author: VasiliSk
 */

#pragma once

#include <stdint.h>

typedef struct {
	uint16_t Size;
	intptr_t* Data;
} Storage;

void Storage_Init(void);
Storage Storage_GetData(uint8_t structindex);
int Storage_LoadData(uint8_t structindex);
void Storage_SaveData(uint8_t Struct_Name);
void Storage_Wipe(void);
void Storage_SaveAll(void);
