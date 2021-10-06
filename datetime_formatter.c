/*
 * datetime.c
 *
 *  Created on: 6 сент. 2021 г.
 *      Author: VasiliSk
 */
#include <stdint.h>
#include "datetime_formatter.h"

void gccDateToInt(char *date, uint16_t *outYear, uint8_t *outMonth, uint8_t *outDay) {
	if (date == 0) {
		return;
	}
	uint16_t compileYear = (date[7] - '0') * 1000 + (date[8] - '0') * 100 + (date[9] - '0') * 10 + (date[10] - '0');
	uint8_t compileMonth = (date[0] == 'J') ? ((date[1] == 'a') ? 1 : ((date[2] == 'n') ? 6 : 7)) :    // Jan, Jun or Jul
							(date[0] == 'F') ? 2 :                                       // Feb
							(date[0] == 'M') ? ((date[2] == 'r') ? 3 : 5) :              // Mar or May
							(date[0] == 'A') ? ((date[2] == 'p') ? 4 : 8) :              // Apr or Aug
							(date[0] == 'S') ? 9 :                                      // Sep
							(date[0] == 'O') ? 10 :                                      // Oct
							(date[0] == 'N') ? 11 :                                      // Nov
							(date[0] == 'D') ? 12 :                                      // Dec
									0;
	uint8_t compileDay = (date[4] == ' ') ? (date[5] - '0') : (date[4] - '0') * 10 + (date[5] - '0');
	*outYear = compileYear;
	*outMonth = compileMonth;
	*outDay = compileDay;
}

void printIsoDateFromGcc(char *outBuf, const char *gccDate) {
	if (outBuf == 0 || gccDate == 0) {
		return;
	}
	int i = 0;
	uint8_t compileMonthLow = (gccDate[0] == 'J') ? ((gccDate[1] == 'a') ? 1 : ((gccDate[2] == 'n') ? 6 : 7)) :    // Jan, Jun or Jul
								(gccDate[0] == 'F') ? 2 :                                       // Feb
								(gccDate[0] == 'M') ? ((gccDate[2] == 'r') ? 3 : 5) :          // Mar or May
								(gccDate[0] == 'A') ? ((gccDate[2] == 'p') ? 4 : 8) :          // Apr or Aug
								(gccDate[0] == 'S') ? 9 :                                      // Sep
								(gccDate[0] == 'O') ? 0 :                                      // Oct
								(gccDate[0] == 'N') ? 1 :                                      // Nov
								(gccDate[0] == 'D') ? 2 :                                      // Dec
										0;
	uint8_t compileMonthHi = (gccDate[0] == 'O' || gccDate[0] == 'N' || gccDate[0] == 'D') ? 1 : 0;           // Oct Nov Dec
	outBuf[i++] = gccDate[7];
	outBuf[i++] = gccDate[8];
	outBuf[i++] = gccDate[9];
	outBuf[i++] = gccDate[10];
	outBuf[i++] = '-';
	outBuf[i++] = compileMonthHi + '0';
	outBuf[i++] = compileMonthLow + '0';
	outBuf[i++] = '-';
	outBuf[i++] = (gccDate[4] == ' ') ? '0' : gccDate[4];
	outBuf[i++] = gccDate[5];
	outBuf[i++] = 0;
}
