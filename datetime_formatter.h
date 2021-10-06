/*
 * datetime.h
 *
 *  Created on: 6 сент. 2021 г.
 *      Author: VasiliSk
 */

#pragma once

void gccDateToInt(char *date, uint16_t *outYear, uint8_t *outMonth, uint8_t *outDay);
void printIsoDateFromGcc(char *outBuf, const char *gccDate);
