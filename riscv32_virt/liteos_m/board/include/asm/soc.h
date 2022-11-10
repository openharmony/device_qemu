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

#ifndef _SOC_H
#define _SOC_H
#include "soc_common.h"

/*
 * Get the response interrupt number via mcause.
 * id = mcause & MCAUSE_INT_ID_MASK
 */
#define MSIP                                          0x2000000
#define MTIMERCMP                                     0x2004000
#define MTIMER                                        0x200BFF8
#define CLOCK_CONTRAL_REG                             0x10008000

/* interrupt base addr : 0xc000000 + 4 * interrupt ID
 * [2:0]   priority
 * [31:3]  reserved
 */
#define PLIC_PRIO_BASE                                 0xC000000
#define PLIC_PEND_BASE                                 0xC001000 /* interrupt 0-31 */
#define PLIC_PEND_REG2                                 0xC001004 /* interrupt 32-52 */.
#define PLIC_ENABLE_BASE                               0xC002000 /* interrupt 0-31 */
#define PLIC_ENABLE_REG2                               0xC002004 /* interrupt 32-52 */
#define PLIC_REG_BASE                                  0xC200000

#define UART0_BASE                                     0x10000000

#define UART0_CLK_FREQ                                 0x32000000
#define UART0_BAUDRAT                                  115200

#define RISCV_SYS_MAX_IRQ                              11
#define RISCV_WDOGCMP_IRQ                              (RISCV_SYS_MAX_IRQ + 1)
#define RISCV_RTCCMP_IRQ                               (RISCV_SYS_MAX_IRQ + 2)
#define RISCV_UART0_IRQ                                (RISCV_SYS_MAX_IRQ + 3)

#define RISCV_PLIC_VECTOR_CNT                          53

#endif
