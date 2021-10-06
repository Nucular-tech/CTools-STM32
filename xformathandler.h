/*
 * xformathandler.h
 *
 *  Created on: 6 апр. 2020 г.
 *      Author: VasiliSk
 */
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>

#pragma once

int vsnformatf(char *s, size_t size, const char *template, va_list ap);
int vsformatf(char *s, const char *template, va_list ap);
int snformatf(char *s, size_t size, const char *template, ...);
int sformatf(char *s, const char *template, ...);

const char* getSubstringByIndex(uint32_t index, const char *hay, uint16_t *length);
