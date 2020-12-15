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

#ifndef PLATFORM_HAL_PLATFORM_INTS_H
#define PLATFORM_HAL_PLATFORM_INTS_H

#include"los_typedef.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

/**
 * Maximum number of supported hardware devices that generate hardware interrupts.
 * The maximum number of hardware devices that generate hardware interrupts is 128.
 */
#define OS_HWI_MAX_NUM                  96

/**
 * Maximum interrupt number.
 */

#define OS_HWI_MAX                      ((OS_HWI_MAX_NUM) - 1)

/**
 * Minimum interrupt number.
 */

#define OS_HWI_MIN                      0

/**
 * Maximum usable interrupt number.
 */

#define OS_USER_HWI_MAX                 OS_HWI_MAX

/**
 * Minimum usable interrupt number.
 */

#define OS_USER_HWI_MIN                 OS_HWI_MIN

#define NUM_HAL_INTERRUPT_CNTPSIRQ      29
#define NUM_HAL_INTERRUPT_CNTPNSIRQ     30
#define OS_TICK_INT_NUM                 NUM_HAL_INTERRUPT_CNTPSIRQ // use secure physical timer for now

#define NUM_HAL_INTERRUPT_TIMER0        37
#define NUM_HAL_INTERRUPT_TIMER1        37
#define NUM_HAL_INTERRUPT_TIMER2        38
#define NUM_HAL_INTERRUPT_TIMER3        38

#if 0
#define NUM_HAL_INTERRUPT_UART0         39
#else
#define NUM_HAL_INTERRUPT_UART0         (31 + 2)
#endif
#define NUM_HAL_INTERRUPT_UART1         40
#define NUM_HAL_INTERRUPT_UART2         41

#define NUM_HAL_INTERRUPT_TIMER         NUM_HAL_INTERRUPT_TIMER0
#define NUM_HAL_INTERRUPT_HRTIMER       NUM_HAL_INTERRUPT_TIMER3

#define NUM_HAL_INTERRUPT_NONE          -1

#define NUM_HAL_ISR_MIN                 OS_HWI_MIN
#define NUM_HAL_ISR_MAX                 1020
#define NUM_HAL_ISR_COUNT               (NUM_HAL_ISR_MAX - NUM_HAL_ISR_MIN + 1)

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */
#endif // PLATFORM_HAL_PLATFORM_INTS_H
