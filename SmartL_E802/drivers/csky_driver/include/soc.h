/*
 * Copyright (C) 2017 C-SKY Microsystems Co., Ltd. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**************************************************************************//**
 * @file     soc.h
 * @brief    CSI Core Peripheral Access Layer Header File for
 *           CSKYSOC Device Series
 * @version  V1.0
 * @date     02. June 2017
 ******************************************************************************/

#ifndef _SOC_H_
#define _SOC_H_

#include <stdint.h>
#include <csi_core.h>
#include <sys_freq.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef IHS_VALUE
#define  IHS_VALUE    (20000000)
#endif

#ifndef EHS_VALUE
#define  EHS_VALUE    (20000000)
#endif

/* -------------------------  Interrupt Number Definition  ------------------------ */

typedef enum IRQn {
    /* ----------------------  SmartL Specific Interrupt Numbers  --------------------- */
    UART_IRQn                       =   0,      /* uart Interrupt */
    CORET_IRQn                      =   1,      /* core Timer Interrupt */
    TIM0_IRQn                       =   2,      /* timer0 Interrupt */
    TIM1_IRQn                       =   3,      /* timer1 Interrupt */
    TIM2_IRQn                       =   4,      /* timer1 Interrupt */
    TIM3_IRQn                       =   5,      /* timer1 Interrupt */
    GPIO0_IRQn                      =   7,      /* gpio0 Interrupt */
    GPIO1_IRQn                      =   8,      /* gpio1 Interrupt */
    GPIO2_IRQn                      =   9,      /* gpio2 Interrupt */
    GPIO3_IRQn                      =   10,     /* gpio3 Interrupt */
    GPIO4_IRQn                      =   11,     /* gpio4 Interrupt */
    GPIO5_IRQn                      =   12,     /* gpio5 Interrupt */
    GPIO6_IRQn                      =   13,     /* gpio6 Interrupt */
    GPIO7_IRQn                      =   14,     /* gpio7 Interrupt */
    STIM0_IRQn                      =   16,     /* stimer0 Interrupt */
    STIM1_IRQn                      =   17,     /* stimer0 Interrupt */
    STIM2_IRQn                      =   18,     /* stimer0 Interrupt */
    STIM3_IRQn                      =   19,     /* stimer0 Interrupt */
    PAD_IRQn                        =   20,     /* pad Interrupt */
}
IRQn_Type;

/* ================================================================================ */
/* ================       Device Specific Peripheral Section       ================ */
/* ================================================================================ */

#define CONFIG_TIMER_NUM    4
#define CONFIG_USART_NUM    1
#define CONFIG_GPIO_NUM     8
#define CONFIG_GPIO_PIN_NUM 8

/* ================================================================================ */
/* ================              Peripheral memory map             ================ */
/* ================================================================================ */
/* --------------------------  CPU FPGA memory map  ------------------------------- */
#define CSKY_SRAM_BASE              (0x20000000UL)

#define CSKY_UART_BASE              (0x40015000UL)
#define CSKY_PMU_BASE               (0x40016000UL)
#define CSKY_TIMER0_BASE            (0x40011000UL)
#define CSKY_TIMER1_BASE            (0x40011014UL)
#define CSKY_TIMER2_BASE            (0x40011028UL)
#define CSKY_TIMER3_BASE            (0x4001103cUL)
#define CSKY_TIMER_CONTROL_BASE     (0x400110a0UL)
#define CSKY_CLK_GEN_BASE           (0x40017000UL)
#define CSKY_STIMER0_BASE           (0x40018000UL)
#define CSKY_STIMER1_BASE           (0x40018014UL)
#define CSKY_STIMER2_BASE           (0x40018028UL)
#define CSKY_STIMER3_BASE           (0x4001803cUL)
#define CSKY_STIMER_CONTROL_BASE    (0x400110a0UL)

#define CSKY_GPIOA_BASE             (0x40019000UL)
#define CSKY_GPIOA_CONTROL_BASE     (0x40019030UL)
#define CSKY_SMPU_BASE              (0x4001a000UL)

/* ================================================================================ */
/* ================             Peripheral declaration             ================ */
/* ================================================================================ */
#define CSKY_UART                  ((   CSKY_UART_TypeDef *)    CSKY_UART_BASE)

#ifdef __cplusplus
}
#endif

#endif  /* _SOC_H_ */
