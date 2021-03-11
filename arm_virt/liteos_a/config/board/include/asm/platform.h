/*
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd.
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

#ifndef __ASM_PLATFORM_H__
#define __ASM_PLATFORM_H__

#include "menuconfig.h"
#include "asm/hal_platform_ints.h"
#include "soc/timer.h"
#include "soc/uart.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

/*------------------------------------------------
 * GIC reg base address
 *------------------------------------------------*/
#define GIC_BASE_ADDR             IO_DEVICE_ADDR(0x08000000)
#define GICD_OFFSET               0x00000     /* interrupt distributor offset */
#define GICC_OFFSET               0x10000     /* CPU interface register offset */

#define CRG_REG_BASE              IO_DEVICE_ADDR(0x12010000)

#define UART0_REG_BASE            IO_DEVICE_ADDR(0x09000000)

#if (CONSOLE_UART == UART0)
    #define UART_BASE             UART0_REG_BASE
    #define UART0_INT_NUM         NUM_HAL_INTERRUPT_UART0
#endif

#define DDR_MEM_BASE              0x40000000

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif

