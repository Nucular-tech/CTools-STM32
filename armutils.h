/*
 * armutils.h
 *
 *  Created on: 16 окт. 2017 г.
 *      Author: VasiliSk
 */

#pragma once

#include "stdint.h"

typedef enum {
	ARM_FPU_none, ARM_FPU_single_precision, ARM_FPU_double_precision
} ARM_FPU_t;

typedef enum {
	ARM_CoreUnknown, ARM_M0, ARM_M0_plus, ARM_M1, ARM_M3, ARM_M4, ARM_M7
} ARM_CoreName;

typedef enum {
	ARM_STM32F1xx_MD = 0x410,
	ARM_STM32F2xx = 0x411,
	ARM_STM32F1xx_LD = 0x412,
	ARM_STM32F405_407_415_417xx = 0x413,
	ARM_STM32F1xx_HD = 0x414,
	ARM_STM32L4xx = 0x415,
	ARM_STM32L1xx_MD = 0x416,
	ARM_STM32L0xx = 0x417,
	ARM_STM32F1xx_CL = 0x418,
	ARM_STM32F4xx_HD = 0x419,
	ARM_STM32F1xx_VL = 0x420,
	ARM_STM32F446xx = 0x421,
	ARM_STM32F3xx = 0x422,
	ARM_STM32F4xx_LP = 0x423,
	ARM_STM32L0xx_C2 = 0x425,
	ARM_STM32L1xx_MD_P = 0x427,
	ARM_STM32F1xx_VL_HD = 0x42,
	ARM_STM32L1xx_C2 = 0x429,
	ARM_STM32F1xx_XL = 0x430,
	ARM_STM32F411re = 0x431,
	ARM_STM32F37x = 0x432,
	ARM_STM32F4xx_de = 0x433,
	ARM_STM32F4xx_dsi = 0x434,
	ARM_STM32L43x = 0x435,
	ARM_STM32L1xx_HD = 0x436,
	ARM_STM32L152re = 0x437,
	ARM_STM32F334 = 0x438,
	ARM_STM32F3xx_small = 0x439,
	ARM_STM32F0xx = 0x440,
	ARM_STM32F412 = 0x441,
	ARM_STM32F09x = 0x442,
	ARM_STM32F0xx_small = 0x444,
	ARM_STM32F04x = 0x445,
	ARM_STM32F30_HD = 0x446,
	ARM_STM32L0xx_C5 = 0x447,
	ARM_STM32F0xx_CAN = 0x448,
	ARM_STM32F74_75xxx = 0x449,
	ARM_STM32_H7xxx = 0x450,
	ARM_STM32F76_77xxx = 0x451,
	ARM_STM32F72_73xxx = 0x452,
	ARM_STM32L011 = 0x457,
	ARM_STM32F410 = 0x458,
	ARM_STM32F413 = 0x463,
} ARM_DeviceID_t;

typedef struct {
	ARM_CoreName Core;
	uint32_t Revision;
	uint32_t Partno;
} ARM_Core_t;

typedef struct {
	union {
		struct {
			uint16_t UID_0L;
			uint16_t UID_0H;
		};
		uint32_t UID_0;
	};
	union {
		struct {
			uint16_t UID_1L;
			uint16_t UID_1H;
		};
		uint32_t UID_1;
	};
	union {
		struct {
			uint16_t UID_2L;
			uint16_t UID_2H;
		};
		uint32_t UID_2;
	};
} STM_UID_t;

extern ARM_DeviceID_t ARM_Device_ID;
extern ARM_Core_t ARM_Core;
extern ARM_FPU_t ARM_FPU;
extern STM_UID_t STM_UID;

void ARM_CheckAll(void);
ARM_FPU_t ARM_FPUCheck(void);
ARM_Core_t ARM_CORECheck(void);
STM_UID_t STM_GetUID(void);

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

/*
static inline float Utils_LP_fast(float value, float sample, float filter_const) {
	value -= (filter_const) * (value - (sample));
	return value;
}*/

