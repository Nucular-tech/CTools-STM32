/*
 * xformathandler.c
 *
 *  Created on: 6 апр. 2020 г.
 *      Author: VasiliSk
 */
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#include "xformatc.h"

typedef struct {
	char *s_ptr;
	int32_t max;
} vsn_t;

static void myPutchar(void *arg, char c) {
	vsn_t *type = (vsn_t*) arg;
	//is it num limited?
	if (type->max < 0 || type->max > 0) {
		*(type->s_ptr)++ = c;
		type->max--; // int is fine
	}
}

int vsformatf(char *s, const char *template, va_list ap) {
	vsn_t ftype = { s, -1 };
	int count;

	count = xvformat(myPutchar, (void*) &ftype, template, ap);
	*ftype.s_ptr = 0;

	return count;
}

int vsnformatf(char *s, size_t size, const char *template, va_list ap) {
	vsn_t ftype = { s, size };
	int count;

	count = xvformat(myPutchar, (void*) &ftype, template, ap);
	//put '0' at end of str
	if ((ftype.max < 0) || (ftype.max > 0))
		*ftype.s_ptr = 0;
	else {
		ftype.s_ptr--;
		*ftype.s_ptr = 0;
	}

	return count;
}

int snformatf(char *s, size_t size, const char *template, ...) {
	int count;
	va_list list;
	va_start(list, template);
	count = vsnformatf(s, size, template, list);
	va_end(list);
	return count;
}

int sformatf(char *s, const char *template, ...) {
	int count;
	va_list list;
	va_start(list, template);
	count = vsformatf(s, template, list);
	va_end(list);
	return count;
}

const char* getSubstringByIndex(uint32_t index, const char *hay, uint16_t *length) {
	if (hay == 0 || length == 0)
		return 0;
	for (uint32_t outindex = 0; outindex < index;) {
		if (*hay == '\n') {
			outindex++;
		}
		if (*hay == 0) {
			return 0;
		}
		hay++;
	}
	*length = strcspn(hay, "\n\r");
	return hay;
}
