/*
 * filters.c
 *
 *  Created on: 24 sept. 2019
 *      Author: Vasily Sukhoparov
 */
#include "filters.h"

int AvgNDeleteX_Init(uint8_t size, uint8_t del_size, float init_data, AvgNDeleteX_t *filter) {
	if (filter == 0 || size == 0 || size == 0xFF || (del_size * 2 + 1) > size || filter->Data == 0 || filter->NextIndex == 0 || filter->PrevIndex == 0)
		return 1;
	for (int i = 0; i < size; i++) {
		filter->Data[i] = init_data;
		filter->NextIndex[i] = size;
		filter->PrevIndex[i] = size;
	}
	filter->Size = size;
	filter->Start = size;
	filter->DeleteSize = del_size;
	filter->InputIndex = 0;
	return 0;
}

void AvgNDeleteX_AddValue(float input, AvgNDeleteX_t *filter) {
	//put it in last position of cycle buffer
	int dindex = filter->InputIndex;
	filter->Data[dindex] = input;
	filter->InputIndex = (dindex + 1) % filter->Size;
	//get pre-delete index
	int nextindex = filter->NextIndex[dindex];
	int preindex = filter->PrevIndex[dindex];
	//cleanup
	filter->NextIndex[dindex] = filter->Size;
	//filter->PrevIndex[dindex] = filter->Size;
	//collapse start
	if (preindex < filter->Size) {
		filter->NextIndex[preindex] = nextindex; //set next value on previous
	} else if (nextindex < filter->Size) {
		//filter->Start == dindex
		filter->Start = nextindex; //if it was first element, put next element on start
	}
	//collapse end
	if (nextindex < filter->Size) {
		filter->PrevIndex[nextindex] = preindex;
	}
	//filter it in array indexes
	int sort_index = filter->Start;
	int prev_sort_i = filter->Start;
	while (1) {
		//sorted data index
		if (sort_index >= filter->Size) {
			//end or start of filtered array
			//filter->NextIndex[dindex] = filter->Size;
			filter->PrevIndex[dindex] = prev_sort_i;
			//set next value on previous
			if (prev_sort_i < filter->Size)
				filter->NextIndex[prev_sort_i] = dindex;
			else {
				//or put current as start
				filter->Start = dindex;
			}
			break;
		} else if (filter->Data[sort_index] > input) {
			//sort_index value bigger
			preindex = filter->PrevIndex[sort_index];
			nextindex = filter->NextIndex[sort_index];
			//place current value as previos for sort_index
			filter->PrevIndex[sort_index] = dindex;
			//for current value next is sort_index (its bigger) and prev is previous of sort_index
			filter->NextIndex[dindex] = sort_index;
			filter->PrevIndex[dindex] = preindex;
			//paste current as next for sort_index prev
			if (preindex < filter->Size) {
				filter->NextIndex[preindex] = dindex; //set next value on previous
			} else {
				//or put it as start
				filter->Start = dindex;
			}
			break;
		} //else continue;
		prev_sort_i = sort_index;
		sort_index = filter->NextIndex[sort_index];
	}
}

float AvgNDeleteX_GetAverage(AvgNDeleteX_t *filter) {
	int shift = filter->DeleteSize;
	int n = filter->Size - filter->DeleteSize * 2;
	float *data = filter->Data;
	float total = 0;
	int next = filter->Start;
	for (int skip = 0; skip < shift; skip++) {
		if (next < filter->Size)
			next = filter->NextIndex[next];
		else
			break;
	}
	int i = 0;
	for (; i < n; i++) {
		if (next < filter->Size) {
			total += data[next];
			next = filter->NextIndex[next];
		} else
			break;
	}
	if (i == 0)
		return 0;
	else
		return total / i;
}

