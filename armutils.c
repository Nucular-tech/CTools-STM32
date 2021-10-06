/*
 * armutils.c
 *
 *  Created on: 16 oct. 2017
 *      Author: VasiliSk ©тырено
 */

#include "Resconfig.h"
#include "armutils.h"

ARM_DeviceID_t ARM_Device_ID;
ARM_Core_t ARM_Core;
ARM_FPU_t ARM_FPU;
STM_UID_t STM_UID;
// FPU Programmer Model
// http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0489b/Chdhfiah.html
void ARM_CheckAll(void) {
	ARM_FPUCheck();
	ARM_CORECheck();
	STM_GetUID();
}

ARM_FPU_t ARM_FPUCheck(void) {
	uint32_t mvfr0;

	/*	trace_printf("%08X %08X %08X\n%08X %08X %08X\n", *(volatile uint32_t *) 0xE000EF34,   // FPCCR  0xC0000000
	 *(volatile uint32_t *) 0xE000EF38,   // FPCAR
	 *(volatile uint32_t *) 0xE000EF3C,   // FPDSCR
	 *(volatile uint32_t *) 0xE000EF40,   // MVFR0  0x10110021 vs 0x10110221
	 *(volatile uint32_t *) 0xE000EF44,   // MVFR1  0x11000011 vs 0x12000011
	 *(volatile uint32_t *) 0xE000EF48);  // MVFR2  0x00000040
	 */
	mvfr0 = *(volatile uint32_t *) 0xE000EF40;

#ifdef TRACE
	switch (mvfr0) {
	case 0x10110021:
		trace_printf("FPU-S Single-precision only\n");
		break;
	case 0x10110221:
		trace_printf("FPU-D Single-precision and Double-precision\n");
		break;
	default:
		trace_printf("Unknown FPU\n");
		break;
	}
#endif
	ARM_FPU_t fpu;
	switch (mvfr0) {
	case 0x10110021:
		fpu = ARM_FPU_single_precision;
		break;
	case 0x10110221:
		fpu = ARM_FPU_double_precision;
		break;
	default:
		fpu = ARM_FPU_none;
		break;
	}
	ARM_FPU = fpu;
	return fpu;
}

ARM_Core_t ARM_CORECheck(void) {
	ARM_Core_t core;
	ARM_Device_ID = DBGMCU->IDCODE & 0xFFF;
	uint32_t cpuid = SCB->CPUID;
#ifdef TRACE
	trace_printf("CPUID %08X DEVID %03X\n", cpuid, ARM_Device_ID);
#endif
	core.Partno = (cpuid & 0x0000000F);
	core.Revision = (cpuid & 0x00F00000) >> 20;
	core.Core = ARM_CoreUnknown;
#ifdef TRACE
	if ((cpuid & 0xFF000000) == 0x41000000) // ARM
			{
		switch ((cpuid & 0x0000FFF0) >> 4) {
		case 0xC20:
			trace_printf("Cortex M0 r%dp%d\n", core.Revision, core.Partno);
			break;
		case 0xC60:
			trace_printf("Cortex M0+ r%dp%d\n", core.Revision, core.Partno);
			break;
		case 0xC21:
			trace_printf("Cortex M1 r%dp%d\n", core.Revision, core.Partno);
			break;
		case 0xC23:
			trace_printf("Cortex M3 r%dp%d\n", core.Revision, core.Partno);
			break;
		case 0xC24:
			trace_printf("Cortex M4 r%dp%d\n", core.Revision, core.Partno);
			break;
		case 0xC27:
			trace_printf("Cortex M7 r%dp%d\n", core.Revision, core.Partno);
			break;

		default:
			trace_printf("Unknown CORE\n");
		}
	} else
		trace_printf("Unknown CORE IMPLEMENTER\n");
#endif

	if ((cpuid & 0xFF000000) == 0x41000000) { // ARM
		switch ((cpuid & 0x0000FFF0) >> 4) {
		case 0xC20:
			core.Core = ARM_M0;
			break;
		case 0xC60:
			core.Core = ARM_M0_plus;
			break;
		case 0xC21:
			core.Core = ARM_M1;
			break;
		case 0xC23:
			core.Core = ARM_M3;
			break;
		case 0xC24:
			core.Core = ARM_M4;
			break;
		case 0xC27:
			core.Core = ARM_M7;
			break;
		}
	}
	ARM_Core = core;
	return core;
}

STM_UID_t STM_GetUID(void) {
	STM_UID_t uid;
	uint32_t *Unique_ID = (uint32_t *) 0x1FFFF7AC;
	// F0, F3 	0x1FFFF7AC
	if (ARM_Device_ID == ARM_STM32F04x || ARM_Device_ID == ARM_STM32F09x || ARM_Device_ID == ARM_STM32F0xx || ARM_Device_ID == ARM_STM32F0xx_CAN
			|| ARM_Device_ID == ARM_STM32F0xx_small)
		Unique_ID = (uint32_t *) 0x1FFFF7AC;
	// F1 		0x1FFFF7E8
	if (ARM_Device_ID == ARM_STM32F1xx_CL || ARM_Device_ID == ARM_STM32F1xx_HD || ARM_Device_ID == ARM_STM32F1xx_LD || ARM_Device_ID == ARM_STM32F1xx_MD
			|| ARM_Device_ID == ARM_STM32F1xx_VL || ARM_Device_ID == ARM_STM32F1xx_VL_HD || ARM_Device_ID == ARM_STM32F1xx_XL)
		Unique_ID = (uint32_t *) 0x1FFFF7E8;
	// F2, F4 	0x1FFF7A10
	if (ARM_Device_ID == ARM_STM32F2xx || ARM_Device_ID == ARM_STM32F405_407_415_417xx || ARM_Device_ID == ARM_STM32F410 || ARM_Device_ID == ARM_STM32F411re
			|| ARM_Device_ID == ARM_STM32F412 || ARM_Device_ID == ARM_STM32F413 || ARM_Device_ID == ARM_STM32F446xx || ARM_Device_ID == ARM_STM32F4xx_HD
			|| ARM_Device_ID == ARM_STM32F4xx_LP || ARM_Device_ID == ARM_STM32F4xx_de || ARM_Device_ID == ARM_STM32F4xx_dsi)
		Unique_ID = (uint32_t *) 0x1FFF7A10;
	// F7 		0x1FF0F420
	if (ARM_Device_ID == ARM_STM32F72_73xxx || ARM_Device_ID == ARM_STM32F74_75xxx || ARM_Device_ID == ARM_STM32F76_77xxx)
		Unique_ID = (uint32_t *) 0x1FF0F420;
	// L0 		0x1FF80050
	// L1 Cat.1,Cat.2 	0x1FF80050
	if (ARM_Device_ID == ARM_STM32L011 || ARM_Device_ID == ARM_STM32L0xx || ARM_Device_ID == ARM_STM32L0xx_C2 || ARM_Device_ID == ARM_STM32L0xx_C5
			|| ARM_Device_ID == ARM_STM32L1xx_C2 || ARM_Device_ID == ARM_STM32L152re)
		Unique_ID = (uint32_t *) 0x1FF80050;
	// L1 Cat.3,Cat.4,Cat.5,Cat.6 	0x1FF800D0
	//TODO ?? is it correct?
	if (ARM_Device_ID == ARM_STM32L1xx_HD || ARM_Device_ID == ARM_STM32L1xx_MD || ARM_Device_ID == ARM_STM32L1xx_MD_P)
		Unique_ID = (uint32_t *) 0x1FF800D0;

	if (ARM_Device_ID == ARM_STM32L43x || ARM_Device_ID == ARM_STM32L4xx)
		Unique_ID = (uint32_t *) 0x1FFF7590;

	uid.UID_0 = Unique_ID[0];
	uid.UID_1 = Unique_ID[1];
	uid.UID_2 = Unique_ID[2];
	STM_UID = uid;
#ifdef TRACE
	trace_printf("STM UID: %08X%08X%08X\n", uid.UID_0, uid.UID_1, uid.UID_2);
#endif
	return uid;
}
