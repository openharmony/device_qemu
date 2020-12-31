/*
 * Copyright (c) 2020 HiSilicon (Shanghai) Technologies CO., LIMITED.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __BOARD_CONFIG_H__
#define __BOARD_CONFIG_H__

#include "menuconfig.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

/* physical memory base and size */
#define DDR_MEM_ADDR            0x40000000
#define DDR_MEM_SIZE            0x40000000
/**
 * Memory map is as follows:
 * ADDR         SIZE
 * 0x00000000 ; 0x08000000	Flash
 * 0x08000000 ; 0x01000000	GIC (all variants' registers)
 * 0x09000000 ; 0x00001000	UART
 * 0x09010000 ; 0x00001000	RTC
 * 0x09020000 ; 0x00000018	FW Config
 * 0x09030000 ; 0x00001000	GPIO
 * 0x09040000 ; 0x00001000	Secure UART
 * 0x09050000 ; 0x00020000	SMMU
 * 0x09070000 ; 0x00000018	PCDIMM ACPI
 * 0x09080000 ; 0x00000004	ACPI GED
 * 0x09090000 ; 0x00000004	NVDIMM ACPI
 * 0x0a000000 ; 0x00000200	MMIO
 * 0x0c000000 ; 0x02000000	Platform Bus
 * 0x0e000000 ; 0x01000000	Secure Mem
 * 0x10000000 ; 0x2eff0000	PCIe MMIO
 * 0x3eff0000 ; 0x00010000	PCIe PIO
 * 0x3f000000 ; 0x01000000	PCIe ECAM
 * 0x40000000 ; 0xXXXXXXXX	RAM
 */

/* Peripheral register address base and size */
#define PERIPH_PMM_BASE         0x00000000
/* Note: Size ranges to the end of Secure Mem */
#define PERIPH_PMM_SIZE         0x0F000000

#define KERNEL_VADDR_BASE       0x40000000
#define KERNEL_VADDR_SIZE       DDR_MEM_SIZE

#define SYS_MEM_BASE            DDR_MEM_ADDR
#define SYS_MEM_SIZE_DEFAULT    0x02000000
#define SYS_MEM_END             (SYS_MEM_BASE + SYS_MEM_SIZE_DEFAULT)

#define EXC_INTERACT_MEM_SIZE        0x100000

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif
