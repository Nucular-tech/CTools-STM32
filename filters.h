/*
 * filters.h
 *
 *  Created on: 24 sept. 2019
 *      Author: Vasily Sukhoparov
 */

#include "Resconfig.h"

#pragma once

typedef struct {
	float *Data;
	//chain index in sync with data
	uint8_t *NextIndex;
	uint8_t *PrevIndex;

	uint8_t Size;
	uint8_t DeleteSize;
	uint8_t InputIndex;

	uint8_t Start;
//	uint8_t End;
} AvgNDeleteX_t;

int AvgNDeleteX_Init(uint8_t size, uint8_t del_size, float init_data, AvgNDeleteX_t *filter);
void AvgNDeleteX_AddValue(float input, AvgNDeleteX_t *filter);
float AvgNDeleteX_GetAverage(AvgNDeleteX_t *filter);
